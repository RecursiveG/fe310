#!/bin/bash

ver=$(clang --version | grep version | awk '{print $3}')
echo Clang version is $ver
wget "https://github.com/llvm/llvm-project/releases/download/llvmorg-$ver/llvm-project-$ver.src.tar.xz"
tar -xaf "llvm-project-$ver.src.tar.xz"

cmake -B llvm-project-$ver.src/compiler-rt-build -G 'Unix Makefiles' \
-DCOMPILER_RT_BUILD_BUILTINS=ON \
-DCOMPILER_RT_BUILD_CRT=OFF \
-DCOMPILER_RT_BUILD_SANITIZERS=OFF \
-DCOMPILER_RT_BUILD_XRAY=OFF \
-DCOMPILER_RT_BUILD_LIBFUZZER=OFF \
-DCOMPILER_RT_BUILD_PROFILE=OFF \
-DCOMPILER_RT_BUILD_MEMPROF=OFF \
-DCOMPILER_RT_BUILD_XRAY_NO_PREINIT=OFF \
-DCOMPILER_RT_BUILD_ORC=OFF \
-DCOMPILER_RT_BUILD_GWP_ASAN=OFF \
-DCOMPILER_RT_ENABLE_CET=OFF \
-DCMAKE_AR=/usr/bin/llvm-ar \
-DCMAKE_ASM_COMPILER_TARGET="riscv32" \
-DCMAKE_ASM_FLAGS="--target=riscv32 -mcpu=sifive-e31 -nostdlib" \
-DCMAKE_C_COMPILER=/usr/bin/clang \
-DCMAKE_C_COMPILER_TARGET="riscv32" \
-DCMAKE_C_FLAGS="--target=riscv32 -mcpu=sifive-e31 -nostdlib" \
-DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=lld" \
-DCMAKE_NM=/usr/bin/llvm-nm \
-DCMAKE_RANLIB=/usr/bin/llvm-ranlib \
-DCOMPILER_RT_DEFAULT_TARGET_ONLY=ON \
-DLLVM_CONFIG_PATH=/usr/bin/llvm-config \
-DCOMPILER_RT_BAREMETAL_BUILD=ON \
"llvm-project-$ver.src/compiler-rt"

cmake --build "llvm-project-$ver.src/compiler-rt-build"

cp -v "llvm-project-$ver.src/compiler-rt-build/lib/linux/libclang_rt.builtins-riscv32.a" .
