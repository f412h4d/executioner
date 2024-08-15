cmake_minimum_required(VERSION 3.25)
project(executioner)

set(CMAKE_CXX_STANDARD 23)

# Prefer static libraries
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_CXX_FLAGS "-static-libstdc++ -static-libgcc -static")

# Set the path to the locally built OpenSSL with static libraries
set(OPENSSL_ROOT_DIR /home/f4r/openssl-3.3.1)
set(OPENSSL_USE_STATIC_LIBS TRUE)

# Include directories
include_directories(modules/News/headers)
include_directories(modules/Utils/headers)
include_directories(modules/Margin/headers)
include_directories(modules/Signaling/headers)
include_directories(modules/Order/headers)
include_directories(modules/Order/models/APIParams)
include_directories(modules/Order/models/OrderInput)
include_directories(modules/Order/models/TriggerOrderInput)

# Source files
set(SOURCES
    main.cpp
    modules/News/src/news.cpp
    modules/Utils/src/utils.cpp
    modules/Margin/src/margin.cpp
    modules/Signaling/src/signaling.cpp
    modules/Order/models/APIParams/APIParams.cpp
    modules/Order/models/OrderInput/OrderInput.cpp
    modules/Order/models/TriggerOrderInput/TriggerOrderInput.cpp
    modules/Order/src/OrderService.cpp
)

# Add executable
add_executable(executioner ${SOURCES})

include(FetchContent)

# Fetch and build CPR as a static library
FetchContent_Declare(
    cpr
    GIT_REPOSITORY https://github.com/libcpr/cpr.git
    GIT_TAG 1.10.5
)
FetchContent_MakeAvailable(cpr)
target_link_libraries(executioner PRIVATE cpr::cpr)

# Fetch and build nlohmann_json as a static library
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)
target_link_libraries(executioner PRIVATE nlohmann_json::nlohmann_json)

# Find and link OpenSSL as a static library
find_package(OpenSSL REQUIRED)

# Link the libraries statically
target_link_libraries(executioner PRIVATE OpenSSL::SSL OpenSSL::Crypto)

# Link the static zlib library
target_link_libraries(executioner PRIVATE /home/f4r/zlib-1.3.1/libz.a)
