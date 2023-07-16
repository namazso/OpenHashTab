//    Copyright 2019-2023 namazso <admin@namazso.eu>
//    This file is part of OpenHashTab.
//
//    OpenHashTab is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    OpenHashTab is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with OpenHashTab.  If not, see <https://www.gnu.org/licenses/>.
#include "FileHashTask.h"

#include "Coordinator.h"
#include "Queues.h"
#include "utl.h"

std::atomic<intptr_t> FileHashTask::s_allocations_remaining = k_max_allocations;

uint8_t* FileHashTask::BlockTryAllocate() {
  if (--s_allocations_remaining >= 0) {
    const auto p = VirtualAlloc(
      nullptr,
      k_block_size,
      MEM_RESERVE | MEM_COMMIT,
      PAGE_READWRITE
    );

    if (p)
      return static_cast<uint8_t*>(p);
  }

  ++s_allocations_remaining;
  return nullptr;
}

void FileHashTask::BlockReset(uint8_t* p) {
  VirtualAlloc(
    p,
    k_block_size,
    MEM_RESET,
    PAGE_READWRITE
  );
  // We don't care about errors
}

void FileHashTask::BlockFree(uint8_t* p) {
  const auto ret = VirtualFree(p, 0, MEM_RELEASE);
  (void)ret;
  assert(ret);
  ++s_allocations_remaining;
}

VOID NTAPI FileHashTask::HashWorkCallback(
  _Inout_ PTP_CALLBACK_INSTANCE instance,
  _Inout_opt_ PVOID ctx,
  _Inout_ PTP_WORK work
) {
  UNREFERENCED_PARAMETER(instance);
  UNREFERENCED_PARAMETER(work);
  static_cast<FileHashTask*>(ctx)->DoHashRound();
}

VOID WINAPI FileHashTask::IoCallback(
  _Inout_ PTP_CALLBACK_INSTANCE instance,
  _Inout_opt_ PVOID ctx,
  _Inout_opt_ PVOID overlapped,
  _In_ ULONG result,
  _In_ ULONG_PTR bytes_transferred,
  _Inout_ PTP_IO io
) {
  UNREFERENCED_PARAMETER(instance);
  UNREFERENCED_PARAMETER(overlapped);
  UNREFERENCED_PARAMETER(io);
  static_cast<FileHashTask*>(ctx)->OverlappedCompletionRoutine(result, bytes_transferred);
}

void FileHashTask::ProcessReadQueue(uint8_t* reuse_block) {
  FileHashTask* waiting_for_read = nullptr;
  do {
    auto ret = g_read_queue.try_dequeue(waiting_for_read);
    if (!ret)
      break;
    ret = waiting_for_read->ReadBlockAsync(reuse_block);
    reuse_block = nullptr;
    if (!ret)
      break;
  } while (true);
  if (reuse_block)
    BlockFree(reuse_block);
}

FileHashTask::FileHashTask(Coordinator* prop_page, const std::wstring& path, ProcessedFileList::FileInfo file_info)
    : _hash_contexts{}
    , _prop_page{prop_page}
    , _file_info{std::move(file_info)} {
  // Instead of exception, set _error because a failed file is still a finished
  // file task. Finish mechanism will trigger on first block read

  for (auto i = 0u; i < LegacyHashAlgorithm::k_count; ++i) {
    _lparam_idx[i] = static_cast<uint8_t>(i);
    if (_prop_page->settings.algorithms[i])
      _hash_contexts[i] = LegacyHashAlgorithm::Algorithms()[i].MakeContext();
  }

  _handle = utl::OpenForRead(path, true);

  if (_handle == INVALID_HANDLE_VALUE) {
    _error = GetLastError();
    return;
  }

  BY_HANDLE_FILE_INFORMATION fi;
  if (!GetFileInformationByHandle(_handle, &fi)) {
    _error = GetLastError();
    return;
  }

  _file_size = static_cast<uint64_t>(fi.nFileSizeHigh) << 32 | fi.nFileSizeLow;
  _file_index = static_cast<uint64_t>(fi.nFileIndexHigh) << 32 | fi.nFileIndexLow;

  // TODO: use this in queue so a lot of files from a slower device can't slow down another faster device
  _volume_serial = fi.dwVolumeSerialNumber;

  _threadpool_hash_work = CreateThreadpoolWork(
    HashWorkCallback,
    this,
    nullptr
  );

  if (!_threadpool_hash_work) {
    _error = GetLastError();
    return;
  }

  _threadpool_io = CreateThreadpoolIo(
    _handle,
    IoCallback,
    this,
    nullptr
  );

  if (!_threadpool_io) {
    _error = GetLastError();
    return;
  }
}

FileHashTask::~FileHashTask() {
  assert(_block == nullptr);

  if (_handle != INVALID_HANDLE_VALUE)
    CloseHandle(_handle);
  if (_threadpool_hash_work)
    CloseThreadpoolWork(_threadpool_hash_work);
  if (_threadpool_io)
    CloseThreadpoolIo(_threadpool_io);
}

void FileHashTask::StartProcessing() {
  _prop_page->Reference();
  ReadBlockAsync();
}

bool FileHashTask::ReadBlockAsync(uint8_t* reuse_block) {
  if (_error != ERROR_SUCCESS) {
    if (reuse_block)
      BlockFree(reuse_block);
    Finish();
    return true;
  }

  // Set up OVERLAPPED fields for either reading or enqueueing for read
  _overlapped.Internal = 0;     // reserved
  _overlapped.InternalHigh = 0; // reserved
  _overlapped.Offset = static_cast<DWORD>(_current_offset);
  _overlapped.OffsetHigh = static_cast<DWORD>(_current_offset >> 32);
  //_overlapped.hEvent = this; // for caller use

  if (const auto block = reuse_block ? reuse_block : BlockTryAllocate()) {
    const auto read_size = static_cast<DWORD>(GetCurrentBlockSize());

    StartThreadpoolIo(_threadpool_io);

    _block = block;

    const auto ret = ReadFile(
      _handle,
      block,
      read_size,
      nullptr,
      &_overlapped
    );

    const auto error = GetLastError();

    if (ret || error == ERROR_IO_PENDING) // succeeded
      return true;

    _block = nullptr;

    CancelThreadpoolIo(_threadpool_io);

    // We failed to start the async operation, free block - cant give it back
    BlockFree(block);

    // If we got some unknown error don't reschedule, fail instead
    if (error != ERROR_INVALID_USER_BUFFER && error != ERROR_NOT_ENOUGH_MEMORY) {
      assert(_error);
      _error = error;
      Finish();
      return true;
    }
  }

  // If we just ran out of memory or outstanding async ios, requeue
  g_read_queue.enqueue(this);
  return false;
}

void FileHashTask::OverlappedCompletionRoutine(ULONG error_code, ULONG_PTR bytes_transferred) {
  UNREFERENCED_PARAMETER(bytes_transferred);

  uint8_t* reuse_block = nullptr;

  if (_cancelled)
    error_code = ERROR_CANCELLED;

  if (error_code != ERROR_SUCCESS) {
    _error = error_code;
    reuse_block = _block;
    _block = nullptr;
    BlockReset(reuse_block);
    Finish();
  } else {
    AddToHashQueue();
  }

  ProcessReadQueue(reuse_block);
}

void FileHashTask::AddToHashQueue() {
  assert(_block);

  _hash_start_counter.store(LegacyHashAlgorithm::k_count, std::memory_order_relaxed);
  _hash_finish_counter.store(LegacyHashAlgorithm::k_count, std::memory_order_relaxed);

  for (auto i = 0u; i < LegacyHashAlgorithm::k_count; ++i)
    SubmitThreadpoolWork(_threadpool_hash_work);
}

void FileHashTask::DoHashRound() {
  const auto ctx_index = --_hash_start_counter;
  auto& ctx = _hash_contexts[ctx_index];
  const auto block_size = GetCurrentBlockSize();
  if (ctx.IsInitialized())
    ctx.Update(_block, block_size);
  const auto locks_on_this = --_hash_finish_counter;
  if (locks_on_this == 0)
    FinishedBlock();
}

void FileHashTask::FinishedBlock() {
  const auto block_size = GetCurrentBlockSize();
  _prop_page->FileProgressCallback(block_size);
  _current_offset += block_size;
  auto reuse_block = _block;
  _block = nullptr;
  BlockReset(reuse_block);

  if (GetCurrentBlockSize() > 0 && !_cancelled) {
    ReadBlockAsync(reuse_block);
    reuse_block = nullptr;
  } else {
    if (_cancelled)
      _error = ERROR_CANCELLED;
    Finish();
  }

  ProcessReadQueue(reuse_block);
}

void FileHashTask::Finish() {
  if (!_error) {
    // If we expect a hash but none match, write no match to all algos
    _match_state = _file_info.expected_hashes.empty() ? MatchState_None : MatchState_Mismatch;

    for (auto i = 0u; i < LegacyHashAlgorithm::k_count; ++i) {
      auto& it_result = _hash_results[i];
      auto& it_ctx = _hash_contexts[i];
      if (it_ctx.IsInitialized()) {
        it_result.resize(it_ctx.GetOutputSize());
        it_ctx.Finish(it_result.data());
      }

      // TODO: O(n^2) BABY HERE WE GO
      for (const auto& expected : _file_info.expected_hashes)
        if (_match_state != MatchState_None && it_result == expected) {
          // secure algorithms trump insecure ones
          if (_match_state == MatchState_Mismatch || !LegacyHashAlgorithm::Algorithms()[_match_state].IsSecure())
            _match_state = (int)i; // TODO: store all matches somehow
        }
    }
  }

  _prop_page->FileCompletionCallback(this);
  _prop_page->Dereference();
}
