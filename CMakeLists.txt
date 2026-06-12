if(CONFIG_PMW3610 AND CONFIG_ZMK_SPLIT AND NOT CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    target_sources(app PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/boards/shields/roBa/zmk_keymap_peripheral_stub.c
    )
endif()

# LVGL v8 / ZMK v0.3-branch compatibility patches for prospector_scanner.
#
# This file IS processed by Zephyr (cmake: . in zephyr/module.yml).
# boards/shields/prospector_scanner/CMakeLists.txt is NOT processed by ZMK
# because the shield definition lives entirely in prospector-zmk-module.
#
# Strategy: generate patched source files into CMAKE_BINARY_DIR, add them to
# app immediately, and defer removal of the upstream originals to the very end
# of CMAKE_SOURCE_DIR processing (after the prospector shield cmake has run).
if(CONFIG_SHIELD_PROSPECTOR_SCANNER AND DEFINED ZEPHYR_PROSPECTOR_ZMK_MODULE_MODULE_DIR)
    set(_PSRC "${ZEPHYR_PROSPECTOR_ZMK_MODULE_MODULE_DIR}/boards/shields/prospector_scanner/src")
    set(_PBIN "${CMAKE_BINARY_DIR}/prospector_patches")
    set(_LSRC "${CMAKE_CURRENT_LIST_DIR}/boards/shields/prospector_scanner/src")

    # ── radii_layout.c: local v8-compatible replacement ───────────────────────
    if(CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM)
        target_sources(app PRIVATE "${_LSRC}/radii_layout.c")
    endif()
    set_property(GLOBAL APPEND PROPERTY _PROSPECTOR_SRCS_TO_REMOVE "${_PSRC}/radii_layout.c")

    # ── touch_handler.c: INPUT_CALLBACK_DEFINE 3-arg → 2-arg ─────────────────
    file(READ "${_PSRC}/touch_handler.c" _TC)
    string(REPLACE
        "INPUT_CALLBACK_DEFINE(DEVICE_DT_GET(TOUCH_NODE), touch_input_callback, NULL)"
        "INPUT_CALLBACK_DEFINE(DEVICE_DT_GET(TOUCH_NODE), touch_input_callback)"
        _TC "${_TC}"
    )
    file(WRITE "${_PBIN}/touch_handler.c" "${_TC}")
    target_sources(app PRIVATE "${_PBIN}/touch_handler.c")
    set_property(GLOBAL APPEND PROPERTY _PROSPECTOR_SRCS_TO_REMOVE "${_PSRC}/touch_handler.c")

    # ── custom_status_screen.c: LVGL v9 → v8 API renames ─────────────────────
    file(READ "${_PSRC}/custom_status_screen.c" _CSS)
    string(REPLACE "lv_obj_set_style_transform_scale(" "lv_obj_set_style_transform_zoom(" _CSS "${_CSS}")
    string(REPLACE "lv_indev_active()" "lv_indev_get_act()" _CSS "${_CSS}")
    string(REPLACE "lv_obj_send_event(" "lv_event_send(" _CSS "${_CSS}")
    string(REPLACE
        "#include <zephyr/retention/bootmode.h>  /* For bootmode_set() - Zephyr 4.x bootloader entry */"
        "/* bootmode stub: retention not configured for ZMK v0.3 */\nstatic inline int bootmode_set(uint8_t m) { ARG_UNUSED(m); return -ENOTSUP; }\n#define BOOT_MODE_TYPE_BOOTLOADER 1"
        _CSS "${_CSS}"
    )
    file(WRITE "${_PBIN}/custom_status_screen.c" "${_CSS}")
    target_sources(app PRIVATE "${_PBIN}/custom_status_screen.c")
    set_property(GLOBAL APPEND PROPERTY _PROSPECTOR_SRCS_TO_REMOVE "${_PSRC}/custom_status_screen.c")

    # ── hello_widget.c: lv_malloc/lv_free → lv_mem_alloc/lv_mem_free ─────────
    file(READ "${_PSRC}/hello_widget.c" _HW)
    string(REPLACE "lv_malloc(" "lv_mem_alloc(" _HW "${_HW}")
    string(REPLACE "lv_free(" "lv_mem_free(" _HW "${_HW}")
    file(WRITE "${_PBIN}/hello_widget.c" "${_HW}")
    target_sources(app PRIVATE "${_PBIN}/hello_widget.c")
    set_property(GLOBAL APPEND PROPERTY _PROSPECTOR_SRCS_TO_REMOVE "${_PSRC}/hello_widget.c")

    # ── system_settings_widget.c: lv_malloc/lv_free → lv_mem_alloc/lv_mem_free
    file(READ "${_PSRC}/system_settings_widget.c" _SSW)
    string(REPLACE "lv_malloc(" "lv_mem_alloc(" _SSW "${_SSW}")
    string(REPLACE "lv_free(" "lv_mem_free(" _SSW "${_SSW}")
    file(WRITE "${_PBIN}/system_settings_widget.c" "${_SSW}")
    target_sources(app PRIVATE "${_PBIN}/system_settings_widget.c")
    set_property(GLOBAL APPEND PROPERTY _PROSPECTOR_SRCS_TO_REMOVE "${_PSRC}/system_settings_widget.c")

    # ── field_layout.c: LVGL v9 draw layer API → v8 draw context API ─────────
    file(READ "${_PSRC}/field_layout.c" _FL)
    string(REPLACE "lv_layer_t *layer = lv_event_get_layer(e);" "lv_draw_ctx_t *draw_ctx = lv_event_get_draw_ctx(e);" _FL "${_FL}")
    string(REPLACE "line_dsc.p1.x = obj_x1 + cx - dx;" "lv_point_t _p1 = {0}, _p2 = {0}; _p1.x = obj_x1 + cx - dx;" _FL "${_FL}")
    string(REPLACE "line_dsc.p1.y = obj_y1 + cy - dy;" "_p1.y = obj_y1 + cy - dy;" _FL "${_FL}")
    string(REPLACE "line_dsc.p2.x = obj_x1 + cx + dx;" "_p2.x = obj_x1 + cx + dx;" _FL "${_FL}")
    string(REPLACE "line_dsc.p2.y = obj_y1 + cy + dy;" "_p2.y = obj_y1 + cy + dy;" _FL "${_FL}")
    string(REPLACE "lv_draw_line(layer, &line_dsc);" "lv_draw_line(draw_ctx, &line_dsc, &_p1, &_p2);" _FL "${_FL}")
    file(WRITE "${_PBIN}/field_layout.c" "${_FL}")
    target_sources(app PRIVATE "${_PBIN}/field_layout.c")
    set_property(GLOBAL APPEND PROPERTY _PROSPECTOR_SRCS_TO_REMOVE "${_PSRC}/field_layout.c")

    # Defer removal of upstream originals until after the prospector shield
    # cmake has added them via target_sources_ifdef().
    cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}" CALL
        cmake_language EVAL CODE [=[
            get_property(_prm GLOBAL PROPERTY _PROSPECTOR_SRCS_TO_REMOVE)
            if(_prm)
                get_target_property(_srcs app SOURCES)
                list(REMOVE_ITEM _srcs ${_prm})
                set_target_properties(app PROPERTIES SOURCES "${_srcs}")
            endif()
        ]=]
    )
endif()
