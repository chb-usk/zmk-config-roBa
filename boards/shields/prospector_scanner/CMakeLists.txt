# LVGL v8 and ZMK v0.3-branch compatibility patches for prospector_scanner.
#
# prospector-zmk-module v2.2.1 targets LVGL v9 (ZMK main / Zephyr 4.2+).
# ZMK v0.3-branch ships Zephyr 4.1.0 / LVGL v8.3.
#
# Strategy: read upstream source files, apply string replacements, write
# patched copies to CMAKE_CURRENT_BINARY_DIR, mark originals HEADER_FILE_ONLY,
# and compile the patched copies instead.  This avoids CMake directory-scope
# issues that prevent set_source_files_properties(COMPILE_OPTIONS) from working
# on files owned by a different module's cmake scope.

if(CONFIG_SHIELD_PROSPECTOR_SCANNER AND DEFINED ZEPHYR_PROSPECTOR_ZMK_MODULE_MODULE_DIR)
    set(_PSRC
        "${ZEPHYR_PROSPECTOR_ZMK_MODULE_MODULE_DIR}/boards/shields/prospector_scanner/src"
    )

    # ── radii_layout.c: replace with v8-compatible version ────────────────────
    # The upstream file uses LVGL v9 canvas layer API (lv_canvas_init_layer,
    # lv_draw_line on a layer, lv_image_* widget names).  Our replacement
    # rewrites these with the LVGL v8 equivalents.
    set_source_files_properties(
        "${_PSRC}/radii_layout.c"
        PROPERTIES HEADER_FILE_ONLY TRUE
    )
    if(CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM)
        target_sources(app PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src/radii_layout.c")
    endif()

    # ── touch_handler.c: INPUT_CALLBACK_DEFINE 3-arg → 2-arg ─────────────────
    # ZMK main added a user_data parameter; ZMK v0.3-branch macro takes only
    # (dev, callback).  Patch the one call site that passes the extra NULL.
    file(READ "${_PSRC}/touch_handler.c" _TC)
    string(REPLACE
        "INPUT_CALLBACK_DEFINE(DEVICE_DT_GET(TOUCH_NODE), touch_input_callback, NULL)"
        "INPUT_CALLBACK_DEFINE(DEVICE_DT_GET(TOUCH_NODE), touch_input_callback)"
        _TC "${_TC}"
    )
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/touch_handler.c" "${_TC}")
    set_source_files_properties(
        "${_PSRC}/touch_handler.c"
        PROPERTIES HEADER_FILE_ONLY TRUE
    )
    target_sources(app PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/touch_handler.c")

    # ── custom_status_screen.c: LVGL v9 → v8 API renames ─────────────────────
    file(READ "${_PSRC}/custom_status_screen.c" _CSS)
    # lv_obj_set_style_transform_scale (v9) → lv_obj_set_style_transform_zoom (v8)
    string(REPLACE
        "lv_obj_set_style_transform_scale("
        "lv_obj_set_style_transform_zoom("
        _CSS "${_CSS}"
    )
    # lv_indev_active() (v9) → lv_indev_get_act() (v8)
    string(REPLACE "lv_indev_active()" "lv_indev_get_act()" _CSS "${_CSS}")
    # lv_obj_send_event (v9) → lv_event_send (v8)
    string(REPLACE "lv_obj_send_event(" "lv_event_send(" _CSS "${_CSS}")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/custom_status_screen.c" "${_CSS}")
    set_source_files_properties(
        "${_PSRC}/custom_status_screen.c"
        PROPERTIES HEADER_FILE_ONLY TRUE
    )
    target_sources(app PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/custom_status_screen.c")

    # ── hello_widget.c: lv_malloc/lv_free → lv_mem_alloc/lv_mem_free ─────────
    # LVGL v9 introduced lv_malloc/lv_free; v8 uses lv_mem_alloc/lv_mem_free.
    file(READ "${_PSRC}/hello_widget.c" _HW)
    string(REPLACE "lv_malloc(" "lv_mem_alloc(" _HW "${_HW}")
    string(REPLACE "lv_free(" "lv_mem_free(" _HW "${_HW}")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/hello_widget.c" "${_HW}")
    set_source_files_properties(
        "${_PSRC}/hello_widget.c"
        PROPERTIES HEADER_FILE_ONLY TRUE
    )
    target_sources(app PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/hello_widget.c")

endif()
