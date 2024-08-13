#!/bin/bash

# Check if the --prod flag is provided
if [ "$1" == "--prod" ]; then
    echo "Using production configuration..."
    cp CMakeLists.prod.txt CMakeLists.txt
    cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=/home/amir_f/vcpkg/scripts/buildsystems/vcpkg.cmake
    cmake --build build
else
    echo "Using local configuration..."
    cp CMakeLists.local.txt CMakeLists.txt
    cmake -B build -S .
    cmake --build build -- -j8
fi

# Clean up by removing the temporary CMakeLists.txt
rm CMakeLists.txt

make

echo "Build process completed."

