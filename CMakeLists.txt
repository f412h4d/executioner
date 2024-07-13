cmake_minimum_required(VERSION 3.25)
project(executioner)

set(CMAKE_CXX_STANDARD 23)


# Include directories
include_directories(modules/Utils/headers)
include_directories(modules/Margin/headers)
include_directories(modules/Account/headers)
include_directories(modules/Signaling/headers)

include_directories(modules/Order/headers)
include_directories(modules/Order/models/APIParams)
include_directories(modules/Order/models/OrderInput)
include_directories(modules/Order/models/TriggerOrderInput)

# Source files
set(SOURCES
        main.cpp
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

FetchContent_Declare(
        cpr
        GIT_REPOSITORY https://github.com/libcpr/cpr.git
        GIT_TAG 1.10.5
)
FetchContent_MakeAvailable(cpr)
target_link_libraries(executioner PRIVATE cpr::cpr)


FetchContent_Declare(json GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3)
FetchContent_MakeAvailable(json)
target_link_libraries(executioner PRIVATE nlohmann_json::nlohmann_json)


# Fetch OpenSSL
FetchContent_Declare(
        openssl
        GIT_REPOSITORY https://github.com/openssl/openssl.git
        GIT_TAG openssl-3.3.1
)
FetchContent_MakeAvailable(openssl)


# Find the OpenSSL package
find_package(OpenSSL REQUIRED)

# Link the libraries
target_link_libraries(executioner PRIVATE OpenSSL::SSL OpenSSL::Crypto)
