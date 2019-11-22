// Public domain
// Based on public domain 7zip implementation by Igor Pavlov and Samuel Neves
#include "blake2sp.h"

#ifdef LITTLE_ENDIAN_UNALIGNED

#define GetUi32(p) (*(const uint32_t *)(const void *)(p))
#define SetUi32(p, v) { *(uint32_t *)(p) = (v); }

#else

#define GetUi32(p) ( \
             ((const uint8_t *)(p))[0]        | \
    ((uint32_t)((const uint8_t *)(p))[1] <<  8) | \
    ((uint32_t)((const uint8_t *)(p))[2] << 16) | \
    ((uint32_t)((const uint8_t *)(p))[3] << 24))

#define SetUi32(p, v) { uint8_t *_ppp_ = (uint8_t *)(p); uint32_t _vvv_ = (v); \
    _ppp_[0] = (uint8_t)_vvv_; \
    _ppp_[1] = (uint8_t)(_vvv_ >> 8); \
    _ppp_[2] = (uint8_t)(_vvv_ >> 16); \
    _ppp_[3] = (uint8_t)(_vvv_ >> 24); }

#endif

#ifdef _MSC_VER

/* don't use _rotl with MINGW. It can insert slow call to function. */

/* #if (_MSC_VER >= 1200) */
#pragma intrinsic(_rotl)
#pragma intrinsic(_rotr)
/* #endif */

#define rotlFixed(x, n) _rotl((x), (n))
#define rotrFixed(x, n) _rotr((x), (n))

#else

/* new compilers can translate these macros to fast commands. */

#define rotlFixed(x, n) (((x) << (n)) | ((x) >> (32 - (n))))
#define rotrFixed(x, n) (((x) >> (n)) | ((x) << (32 - (n))))

#endif

#define rotr32 rotrFixed

#define BLAKE2S_NUM_ROUNDS 10
#define BLAKE2S_FINAL_FLAG (~(uint32_t)0)

static const uint32_t k_Blake2s_IV[8] =
{
  0x6A09E667UL, 0xBB67AE85UL, 0x3C6EF372UL, 0xA54FF53AUL,
  0x510E527FUL, 0x9B05688CUL, 0x1F83D9ABUL, 0x5BE0CD19UL
};

static const uint8_t k_Blake2s_Sigma[BLAKE2S_NUM_ROUNDS][16] =
{
  {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 } ,
  { 14, 10,  4,  8,  9, 15, 13,  6,  1, 12,  0,  2, 11,  7,  5,  3 } ,
  { 11,  8, 12,  0,  5,  2, 15, 13, 10, 14,  3,  6,  7,  1,  9,  4 } ,
  {  7,  9,  3,  1, 13, 12, 11, 14,  2,  6,  5, 10,  4,  0, 15,  8 } ,
  {  9,  0,  5,  7,  2,  4, 10, 15, 14,  1, 11, 12,  6,  8,  3, 13 } ,
  {  2, 12,  6, 10,  0, 11,  8,  3,  4, 13,  7,  5, 15, 14,  1,  9 } ,
  { 12,  5,  1, 15, 14, 13,  4, 10,  0,  7,  6,  3,  9,  2,  8, 11 } ,
  { 13, 11,  7, 14, 12,  1,  3,  9,  5,  0, 15,  4,  8,  6,  2, 10 } ,
  {  6, 15, 14,  9, 11,  3,  0,  8, 12,  2, 13,  7,  1,  4, 10,  5 } ,
  { 10,  2,  8,  4,  7,  6,  1,  5, 15, 11,  9, 14,  3, 12, 13 , 0 } ,
};


void Blake2s_Init0(CBlake2s *p)
{
  unsigned i;
  for (i = 0; i < 8; i++)
    p->h[i] = k_Blake2s_IV[i];
  p->t[0] = 0;
  p->t[1] = 0;
  p->f[0] = 0;
  p->f[1] = 0;
  p->bufPos = 0;
  p->lastNode_f1 = 0;
}


static void Blake2s_Compress(CBlake2s *p)
{
  uint32_t m[16];
  uint32_t v[16];

  {
    unsigned i;

    for (i = 0; i < 16; i++)
      m[i] = *(const uint32_t*)(p->buf + i * sizeof(m[i]));

    for (i = 0; i < 8; i++)
      v[i] = p->h[i];
  }

  v[8] = k_Blake2s_IV[0];
  v[9] = k_Blake2s_IV[1];
  v[10] = k_Blake2s_IV[2];
  v[11] = k_Blake2s_IV[3];

  v[12] = p->t[0] ^ k_Blake2s_IV[4];
  v[13] = p->t[1] ^ k_Blake2s_IV[5];
  v[14] = p->f[0] ^ k_Blake2s_IV[6];
  v[15] = p->f[1] ^ k_Blake2s_IV[7];

#define G(r,i,a,b,c,d) \
    a += b + m[sigma[2*i+0]];  d ^= a; d = rotr32(d, 16);  c += d;  b ^= c; b = rotr32(b, 12); \
    a += b + m[sigma[2*i+1]];  d ^= a; d = rotr32(d,  8);  c += d;  b ^= c; b = rotr32(b,  7); \

#define R(r) \
    G(r,0,v[ 0],v[ 4],v[ 8],v[12]); \
    G(r,1,v[ 1],v[ 5],v[ 9],v[13]); \
    G(r,2,v[ 2],v[ 6],v[10],v[14]); \
    G(r,3,v[ 3],v[ 7],v[11],v[15]); \
    G(r,4,v[ 0],v[ 5],v[10],v[15]); \
    G(r,5,v[ 1],v[ 6],v[11],v[12]); \
    G(r,6,v[ 2],v[ 7],v[ 8],v[13]); \
    G(r,7,v[ 3],v[ 4],v[ 9],v[14]); \

  {
    unsigned r;
    for (r = 0; r < BLAKE2S_NUM_ROUNDS; r++)
    {
      const uint8_t *sigma = k_Blake2s_Sigma[r];
      R(r);
    }
    /* R(0); R(1); R(2); R(3); R(4); R(5); R(6); R(7); R(8); R(9); */
  }

#undef G
#undef R

  {
    unsigned i;
    for (i = 0; i < 8; i++)
      p->h[i] ^= v[i] ^ v[i + 8];
  }
}


#define Blake2s_Increment_Counter(S, inc) \
  { p->t[0] += (inc); p->t[1] += (p->t[0] < (inc)); }

#define Blake2s_Set_LastBlock(p) \
  { p->f[0] = BLAKE2S_FINAL_FLAG; p->f[1] = p->lastNode_f1; }


static void Blake2s_Update(CBlake2s *p, const uint8_t *data, size_t size)
{
  while (size != 0)
  {
    unsigned pos = (unsigned)p->bufPos;
    unsigned rem = BLAKE2S_BLOCK_SIZE - pos;

    if (size <= rem)
    {
      memcpy(p->buf + pos, data, size);
      p->bufPos += (uint32_t)size;
      return;
    }

    memcpy(p->buf + pos, data, rem);
    Blake2s_Increment_Counter(S, BLAKE2S_BLOCK_SIZE);
    Blake2s_Compress(p);
    p->bufPos = 0;
    data += rem;
    size -= rem;
  }
}


static void Blake2s_Final(CBlake2s *p, uint8_t *digest)
{
  unsigned i;

  Blake2s_Increment_Counter(S, (uint32_t)p->bufPos);
  Blake2s_Set_LastBlock(p);
  memset(p->buf + p->bufPos, 0, BLAKE2S_BLOCK_SIZE - p->bufPos);
  Blake2s_Compress(p);

  for (i = 0; i < 8; i++)
    SetUi32(digest + sizeof(p->h[i]) * i, p->h[i]);
}


/* ---------- BLAKE2s ---------- */

/* we need to xor CBlake2s::h[i] with input parameter block after Blake2s_Init0() */
/*
typedef struct
{
  uint8_t  digest_length;
  uint8_t  key_length;
  uint8_t  fanout;
  uint8_t  depth;
  uint32_t leaf_length;
  uint8_t  node_offset[6];
  uint8_t  node_depth;
  uint8_t  inner_length;
  uint8_t  salt[BLAKE2S_SALTuint8_tS];
  uint8_t  personal[BLAKE2S_PERSONALuint8_tS];
} CBlake2sParam;
*/


static void Blake2sp_Init_Spec(CBlake2s *p, unsigned node_offset, unsigned node_depth)
{
  Blake2s_Init0(p);

  p->h[0] ^= (BLAKE2S_DIGEST_SIZE | ((uint32_t)BLAKE2SP_PARALLEL_DEGREE << 16) | ((uint32_t)2 << 24));
  p->h[2] ^= ((uint32_t)node_offset);
  p->h[3] ^= ((uint32_t)node_depth << 16) | ((uint32_t)BLAKE2S_DIGEST_SIZE << 24);
  /*
  P->digest_length = BLAKE2S_DIGEST_SIZE;
  P->key_length = 0;
  P->fanout = BLAKE2SP_PARALLEL_DEGREE;
  P->depth = 2;
  P->leaf_length = 0;
  store48(P->node_offset, node_offset);
  P->node_depth = node_depth;
  P->inner_length = BLAKE2S_DIGEST_SIZE;
  */
}


void Blake2sp_Init(CBlake2sp *p)
{
  unsigned i;

  p->bufPos = 0;

  for (i = 0; i < BLAKE2SP_PARALLEL_DEGREE; i++)
    Blake2sp_Init_Spec(&p->S[i], i, 0);

  p->S[BLAKE2SP_PARALLEL_DEGREE - 1].lastNode_f1 = BLAKE2S_FINAL_FLAG;
}


void Blake2sp_Update(CBlake2sp *p, const uint8_t *data, size_t size)
{
  unsigned pos = p->bufPos;
  while (size != 0)
  {
    unsigned index = pos / BLAKE2S_BLOCK_SIZE;
    unsigned rem = BLAKE2S_BLOCK_SIZE - (pos & (BLAKE2S_BLOCK_SIZE - 1));
    if (rem > size)
      rem = (unsigned)size;
    Blake2s_Update(&p->S[index], data, rem);
    size -= rem;
    data += rem;
    pos += rem;
    pos &= (BLAKE2S_BLOCK_SIZE * BLAKE2SP_PARALLEL_DEGREE - 1);
  }
  p->bufPos = pos;
}


void Blake2sp_Final(CBlake2sp *p, uint8_t *digest)
{
  CBlake2s R;
  unsigned i;

  Blake2sp_Init_Spec(&R, 0, 1);
  R.lastNode_f1 = BLAKE2S_FINAL_FLAG;

  for (i = 0; i < BLAKE2SP_PARALLEL_DEGREE; i++)
  {
    uint8_t hash[BLAKE2S_DIGEST_SIZE];
    Blake2s_Final(&p->S[i], hash);
    Blake2s_Update(&R, hash, BLAKE2S_DIGEST_SIZE);
  }

  Blake2s_Final(&R, digest);
}
