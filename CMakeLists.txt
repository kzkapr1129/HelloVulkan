cmake_minimum_required(VERSION 3.22)

project(vulkan-test)

set(CMAKE_CXX_STANDARD 17)

set(BUILD_SHARED_LIBS ON)
add_subdirectory(glfw)
set(BUILD_SHARED_LIBS OFF)

file(GLOB SRC_FILES cmd/*.cpp)
add_executable(app main.cpp ${SRC_FILES})

# https://github.com/KhronosGroup/MoltenVKのREADMEの手順に従い、/Users/nakayama/workspace直下にMoltenVKをcloneしてビルドを行う
set(VULKAN_ROOT "./MoltenVK/Package/Latest/MoltenVK")
find_library(VULKAN_LIBRARY NAMES MoltenVK PATHS "${VULKAN_ROOT}/dylib/macOS")

set(CXXOPTS_ROOT "./cxxopts")

include_directories(
    ${VULKAN_ROOT}/include
    ${CXXOPTS_ROOT}/include
    ${GLFW_ROOT}/include
    common
    cmd)

target_link_libraries(app PRIVATE ${VULKAN_LIBRARY})
target_link_libraries(app PRIVATE glfw)