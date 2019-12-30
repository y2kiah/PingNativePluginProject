#!/bin/sh

#CommonCompilerFlags="-O3"
CommonCompilerFlags="-Og -g -g3 -gdwarf-4"
CommonCompilerFlags="$CommonCompilerFlags -v -std=c++14 -fno-rtti -fext-numeric-literals"

if ! [ -d ./build ]; then
    mkdir ./build
fi
cd ./build

/bin/g++ $CommonCompilerFlags -fPIC -shared -rdynamic -nostartfiles -o libunity-ping.so ../source/unity-ping.cpp

/bin/g++ $CommonCompilerFlags -o test.out ../source/test.cpp

#get disassembly
#/bin/g++ $CommonCompilerFlags -S -fverbose-asm -masm=intel -o unity-ping.s ../source/unity-ping.cpp
objdump -drwCS -Mintel --disassembler-options=intel unity-ping.so > unity-ping.s
objdump -drwCS -Mintel --disassembler-options=intel test.out > test.s

cd ..

cp -p ./build/test.out ./
cp -p ./build/libunity-ping.so ./
cp -p ./build/libunity-ping.so ../UnityProject/Assets/Plugins/Linux/x64/