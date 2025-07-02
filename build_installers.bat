set Platform=x86
dotnet build -c Machine OpenHashTab.sln
dotnet build -c User OpenHashTab.sln
set Platform=x64
dotnet build -c Machine OpenHashTab.sln
dotnet build -c User OpenHashTab.sln
set Platform=ARM64
dotnet build -c Machine OpenHashTab.sln
dotnet build -c User OpenHashTab.sln

move .\bin\x86\Machine\OpenHashTab.msi .\bin\OpenHashTab_Machine_x86.msi
move .\bin\x86\User\OpenHashTab.msi .\bin\OpenHashTab_User_x86.msi
move .\bin\x64\Machine\OpenHashTab.msi .\bin\OpenHashTab_Machine_x64.msi
move .\bin\x64\User\OpenHashTab.msi .\bin\OpenHashTab_User_x64.msi
move .\bin\ARM64\Machine\OpenHashTab.msi .\bin\OpenHashTab_Machine_ARM64.msi
move .\bin\ARM64\User\OpenHashTab.msi .\bin\OpenHashTab_User_ARM64.msi
