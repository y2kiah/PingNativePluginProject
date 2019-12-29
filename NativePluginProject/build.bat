@echo off

rem -Od or -O2 here
set CommonCompilerFlags=-Od
set CommonCompilerFlags=-MT -Oi -Ot -WL -nologo -fp:fast -fp:except- -GF -Gm- -GR- -EHa- -Zo -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4505 -wd4127 -FC -Z7 -GS- -Gs9999999 -Zc:inline -Zc:ternary -diagnostics:caret %CommonCompilerFlags%
set CommonLinkerFlags=-machine:X64 -stack:0x100000,0x100000 -incremental:no -opt:ref -ignore:4099

if not exist .\build mkdir .\build
pushd .\build

cl %CommonCompilerFlags% ../source/unity-ping.cpp -Fmunity-ping.map -LD -link -out:unity-ping.dll -pdb:unity-ping_%random%.pdb %CommonLinkerFlags% ws2_32.lib

cl %CommonCompilerFlags% ../source/test.cpp -Fmtest.map -link -out:test.exe -pdb:test_%random%.pdb -subsystem:console %CommonLinkerFlags% ws2_32.lib

popd

copy .\build\test.exe .\
copy .\build\unity-ping.dll .\
