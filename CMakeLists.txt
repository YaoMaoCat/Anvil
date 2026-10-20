cmake_minimum_required(VERSION 3.20)

project(Anvil VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 静态链接 CRT：发布的单个 exe 不依赖 VC++ 运行库（/MT）
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(MSVC)
    add_compile_options(/utf-8 /W4 /permissive-)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS NOMINMAX)
endif()

# 三方依赖：本地放了 third-party/imgui 就用它（离线可编）；
# 没有则从 GitHub 拉取，保证 clone 下来就能构建。
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/third-party/imgui" CACHE PATH "Dear ImGui 源码目录")
if(NOT EXISTS "${IMGUI_DIR}/imgui.cpp")
    include(FetchContent)
    message(STATUS "third-party/imgui 不存在，改为从 GitHub 拉取 Dear ImGui")
    FetchContent_Declare(imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.93.0
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(imgui)
    set(IMGUI_DIR "${imgui_SOURCE_DIR}" CACHE PATH "Dear ImGui 源码目录" FORCE)
endif()

# 可执行文件统一输出到 out/bin，VS 里直接 F5 即可
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/out/bin")
foreach(cfg ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${cfg} CFG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CFG} "${CMAKE_SOURCE_DIR}/out/bin/${cfg}")
endforeach()

add_subdirectory(src)
