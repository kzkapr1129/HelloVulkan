cmake_minimum_required(VERSION 3.22)

project(vulkan-test)

set(CMAKE_CXX_STANDARD 17)

add_executable(app main.cpp)

# https://github.com/KhronosGroup/MoltenVKのREADMEの手順に従い、/Users/nakayama/workspace直下にMoltenVKをcloneしてビルドを行う
set(VULKAN_ROOT "/Users/nakayama/workspace/MoltenVK/Package/Latest/MoltenVK")
find_library(VULKAN_LIBRARY NAMES MoltenVK PATHS "${VULKAN_ROOT}/dylib/macOS")

include_directories(${VULKAN_ROOT}/include)
target_link_libraries(app PRIVATE ${VULKAN_LIBRARY})