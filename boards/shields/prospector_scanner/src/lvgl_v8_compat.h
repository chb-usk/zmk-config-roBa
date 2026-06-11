/*
 * LVGL v9 → v8 API compatibility shims for ZMK v0.3-branch (Zephyr 4.1.0).
 *
 * prospector-zmk-module uses a small number of LVGL v9 APIs that were renamed
 * from their v8 counterparts.  This header is injected via a per-file
 * COMPILE_OPTIONS -include flag so it only affects the files that need it.
 */
#pragma once

/* lv_obj_set_style_transform_scale (v9) → lv_obj_set_style_transform_zoom (v8) */
#define lv_obj_set_style_transform_scale(obj, v, sel) \
    lv_obj_set_style_transform_zoom(obj, v, sel)

/* lv_indev_active (v9) → lv_indev_get_act (v8) */
#define lv_indev_active() lv_indev_get_act()
