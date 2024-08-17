#!/bin/bash

# Check if the --prod flag is provided
if [ "$1" == "--prod" ]; then
    echo "Using production configuration..."
    cp -rf CMakeLists.prod.txt CMakeLists.txt
    cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=/home/amir_f/vcpkg/scripts/buildsystems/vcpkg.cmake
    cmake --build build -- -j4
else
    echo "Using local configuration..."
    cp -rf CMakeLists.local.txt CMakeLists.txt
    cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=/home/f4r/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    cmake --build build -- -j8
fi

echo "Build process completed."

