# Anvil 核心与界面目标

set(THIRD_PARTY "${CMAKE_SOURCE_DIR}/third-party/rmlui")
set(RMLUI_DEPS "${THIRD_PARTY}/Dependencies/Bin-Static")

# ------------------------------------------------------------------ core
add_library(anvil_core STATIC
    core/md5.cpp
    core/hex.cpp
    core/lic_codec.cpp
    core/machine_id.cpp
    core/serial.cpp
    core/license.cpp
)

target_include_directories(anvil_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(anvil_core PUBLIC iphlpapi)

# ------------------------------------------------------------------ rmlui
# RmlUi 6.3 预编译包：先手动定位 Freetype，满足其 Dependencies.cmake 的检查
set(Freetype_DIR "${RMLUI_DEPS}/share/freetype" CACHE PATH "" FORCE)
find_package(Freetype CONFIG REQUIRED)

set(RmlUi_DIR "${THIRD_PARTY}/Bin-Static/CMake" CACHE PATH "" FORCE)
find_package(RmlUi CONFIG REQUIRED)

# ------------------------------------------------------------------ app
set(RMLUI_BACKENDS "${THIRD_PARTY}/Backends")

add_executable(Anvil
    main/main.cpp
    ui/app.cpp
    ${RMLUI_BACKENDS}/RmlUi_Backend_Win32_DX11.cpp
    ${RMLUI_BACKENDS}/RmlUi_Platform_Win32.cpp
    ${RMLUI_BACKENDS}/RmlUi_Renderer_DX11.cpp
)

target_include_directories(Anvil PRIVATE "${RMLUI_BACKENDS}")
target_link_libraries(Anvil PRIVATE anvil_core RmlUi::Core d3d11 dxgi d3dcompiler)

# 资源目录随可执行文件部署
add_custom_command(TARGET Anvil POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:Anvil>/assets"
    COMMENT "部署 assets")
