#!/bin/bash

# Check if the --prod flag is provided
if [ "$1" == "--prod" ]; then
    echo "Using production configuration..."
    cp CMakeLists.prod.txt CMakeLists.txt
else
    echo "Using local configuration..."
    cp CMakeLists.local.txt CMakeLists.txt
fi

# Run cmake
cmake -B build -S .

# Clean up by removing the temporary CMakeLists.txt
rm CMakeLists.txt

echo "Build process completed."

