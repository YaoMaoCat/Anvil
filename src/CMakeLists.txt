# Anvil 核心与界面目标

# ------------------------------------------------------------------ core
add_library(anvil_core STATIC
    core/md5.cpp
    core/hex.cpp
    core/lic_codec.cpp
    core/machine_id.cpp
    core/serial.cpp
    core/license.cpp
    core/client_probe.cpp
    core/feature_dat.cpp
)

target_include_directories(anvil_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(anvil_core PUBLIC iphlpapi advapi32 crypt32)

# ------------------------------------------------------------------ imgui
add_library(anvil_imgui STATIC
    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp"
    "${IMGUI_DIR}/imgui_tables.cpp"
    "${IMGUI_DIR}/imgui_widgets.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_win32.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_dx11.cpp"
)

target_include_directories(anvil_imgui PUBLIC "${IMGUI_DIR}" "${IMGUI_DIR}/backends")
target_link_libraries(anvil_imgui PUBLIC d3d11 dxgi)

# ------------------------------------------------------------------ app
add_executable(Anvil
    main/main.cpp
    ui/gfx.cpp
    ui/theme.cpp
    ui/xml_pretty.cpp
    ui/version_picker.cpp
    ui/app.cpp
)

target_link_libraries(Anvil PRIVATE anvil_core anvil_imgui d3d11 dxgi)

# 声明 DPI 感知，否则窗口坐标被系统虚拟化，与物理像素对不上
set_target_properties(Anvil PROPERTIES VS_DPI_AWARE "PerMonitor")
