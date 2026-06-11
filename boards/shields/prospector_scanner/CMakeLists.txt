# LVGL v8 compatibility patches for prospector_scanner on ZMK v0.3-branch.
#
# The upstream module was written for LVGL v9 (ZMK main / Zephyr 4.2+).
# Zephyr 4.1.0 (ZMK v0.3-branch) ships LVGL v8.  Two kinds of fixes:
#
#  1. radii_layout.c  — uses v9-only canvas layer API and lv_image_* names.
#     The upstream file is excluded from compilation; a v8-compatible
#     replacement lives next to this CMakeLists.txt.
#
#  2. custom_status_screen.c — two simple renames: lv_obj_set_style_transform_scale
#     and lv_indev_active.  Fixed by force-including lvgl_v8_compat.h via a
#     per-file COMPILE_OPTIONS flag (no file duplication needed).

if(CONFIG_SHIELD_PROSPECTOR_SCANNER AND DEFINED ZEPHYR_PROSPECTOR_ZMK_MODULE_MODULE_DIR)
    set(_PSRC
        "${ZEPHYR_PROSPECTOR_ZMK_MODULE_MODULE_DIR}/boards/shields/prospector_scanner/src"
    )
    set(_COMPAT "${CMAKE_CURRENT_LIST_DIR}/src/lvgl_v8_compat.h")

    # --- radii_layout.c: replace with v8-compatible version ---
    # Mark the upstream file as header-only so it is never compiled,
    # regardless of whether this cmake runs before or after the module cmake.
    set_source_files_properties(
        "${_PSRC}/radii_layout.c"
        PROPERTIES HEADER_FILE_ONLY TRUE
    )
    if(CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM)
        target_sources(app PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src/radii_layout.c")
    endif()

    # --- custom_status_screen.c: inject compat header ---
    # Only two v9 API renames; no need to duplicate the whole file.
    set_source_files_properties(
        "${_PSRC}/custom_status_screen.c"
        PROPERTIES COMPILE_OPTIONS "-include${_COMPAT}"
    )
endif()
