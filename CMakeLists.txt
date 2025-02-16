cmake_minimum_required(VERSION 3.22)

project(vulkan-test)

set(CMAKE_CXX_STANDARD 17)

file(GLOB SRC_FILES cmd/*.cpp)
add_executable(app main.cpp ${SRC_FILES})

# https://github.com/KhronosGroup/MoltenVKのREADMEの手順に従い、/Users/nakayama/workspace直下にMoltenVKをcloneしてビルドを行う
set(VULKAN_ROOT "./MoltenVK/Package/Latest/MoltenVK")
find_library(VULKAN_LIBRARY NAMES MoltenVK PATHS "${VULKAN_ROOT}/dylib/macOS")

set(CXXOPTS_ROOT "./cxxopts")

include_directories(${VULKAN_ROOT}/include)
include_directories(${CXXOPTS_ROOT}/include)
include_directories(common)
include_directories(cmd)
target_link_libraries(app PRIVATE ${VULKAN_LIBRARY})