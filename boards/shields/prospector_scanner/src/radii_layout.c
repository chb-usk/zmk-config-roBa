/*
 * Copyright (c) 2024 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * Radii Layout for Scanner Mode
 * Design and layout based on carrefinho's prospector-zmk-module:
 * https://github.com/carrefinho/prospector-zmk-module
 *
 * Display: 240x280 with hardware y-offset=20
 * Layout designed for: 240x240 visible area (no software offset needed)
 *
 * PATCHED for LVGL v8 (Zephyr 4.1.0 / ZMK v0.3-branch):
 *   - LV_CANVAS_BUF_SIZE_TRUE_COLOR_ALPHA instead of LV_CANVAS_BUF_SIZE
 *   - LV_IMG_CF_TRUE_COLOR_ALPHA instead of LV_COLOR_FORMAT_ARGB8888
 *   - lv_img_* instead of lv_image_* (renamed in LVGL v9)
 *   - lv_canvas_draw_line instead of lv_canvas_init_layer / lv_draw_line
 */

#include "radii_layout.h"
#include "fonts_carrefinho.h"
#include "fonts.h"
#include <zephyr/logging/log.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

LOG_MODULE_REGISTER(radii_layout, CONFIG_ZMK_LOG_LEVEL);

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

/* Wheel dimensions (from original layer_indicator.c) */
#define WHEEL_SIZE         48
#define WHEEL_CENTER       (WHEEL_SIZE / 2)
#define WHEEL_INNER_RADIUS 12
#define WHEEL_OUTER_RADIUS 20

/* ========== Color Palette Structure ========== */
typedef struct {
    uint32_t left_panel_bg;
    uint32_t mod_panel_bg;
    uint32_t battery_panel_bg;
    uint32_t arc_bg;
    uint32_t arc_indicator;
    uint32_t layer_wheel;
    uint32_t layer_text;
    uint32_t mod_active;
    uint32_t mod_inactive;
    const char *name;
} radii_color_palette_t;

/* ========== 4 Color Palettes (from carrefinho's device tree themes) ========== */
static const radii_color_palette_t color_palettes[4] = {
    /* Blue Theme (default) */
    {
        .left_panel_bg = 0xACB9D3,
        .mod_panel_bg = 0x1448AA,
        .battery_panel_bg = 0xE2FF61,
        .arc_bg = 0xA8BF41,
        .arc_indicator = 0x576610,
        .layer_wheel = 0x000000,
        .layer_text = 0x000000,
        .mod_active = 0x61E7FF,
        .mod_inactive = 0x0C2B65,
        .name = "Blue"
    },
    /* Green Theme */
    {
        .left_panel_bg = 0x0D2C26,
        .mod_panel_bg = 0x708066,
        .battery_panel_bg = 0x2D373D,
        .arc_bg = 0x445544,
        .arc_indicator = 0x88AA88,
        .layer_wheel = 0x00FF90,
        .layer_text = 0x00FF90,
        .mod_active = 0xE2FF61,
        .mod_inactive = 0x3A4A3A,
        .name = "Green"
    },
    /* Red Theme */
    {
        .left_panel_bg = 0xD77B7A,
        .mod_panel_bg = 0x7B4B5C,
        .battery_panel_bg = 0xC7BFAD,
        .arc_bg = 0x9A8A7A,
        .arc_indicator = 0x5A4A3A,
        .layer_wheel = 0x000000,
        .layer_text = 0x000000,
        .mod_active = 0xFFAAAB,
        .mod_inactive = 0x4A2A3A,
        .name = "Red"
    },
    /* Purple Theme */
    {
        .left_panel_bg = 0x212121,
        .mod_panel_bg = 0x858585,
        .battery_panel_bg = 0x8774B4,
        .arc_bg = 0x6654A4,
        .arc_indicator = 0xAA99CC,
        .layer_wheel = 0xFFFFFF,
        .layer_text = 0xFFFFFF,
        .mod_active = 0x38FFB3,
        .mod_inactive = 0x444444,
        .name = "Purple"
    }
};

#define PALETTE_COUNT 4
static uint8_t current_palette = 0;

/* ========== Static storage ========== */
static lv_obj_t *parent_screen = NULL;
static bool layout_created = false;

/* Left panel */
static lv_obj_t *left_panel = NULL;
static lv_obj_t *wheel_canvas = NULL;
static lv_obj_t *wheel_image = NULL;
static lv_obj_t *layer_label = NULL;
static uint8_t current_layer = 0;
static uint8_t current_layer_count = 6;

/* Modifier panel */
static lv_obj_t *mod_panel = NULL;
static lv_obj_t *mod_labels[4] = {NULL};

/* Battery panel */
static lv_obj_t *bat_panel = NULL;
static lv_obj_t *bat_arc_left = NULL;
static lv_obj_t *bat_arc_right = NULL;

/* Canvas buffer — v8: LV_CANVAS_BUF_SIZE_TRUE_COLOR_ALPHA (was LV_CANVAS_BUF_SIZE in v9) */
static uint8_t wheel_canvas_buf[LV_CANVAS_BUF_SIZE_TRUE_COLOR_ALPHA(WHEEL_SIZE, WHEEL_SIZE)];

/* ========== Wheel Drawing ========== */

static void draw_wheel(uint8_t layer_count) {
    if (!wheel_canvas) return;

    const radii_color_palette_t *p = &color_palettes[current_palette];
    int num_ticks = (layer_count > 0 && layer_count <= 16) ? layer_count : 6;

    lv_canvas_fill_bg(wheel_canvas, lv_color_hex(0x000000), LV_OPA_TRANSP);

    /* v8: draw each tick line directly with lv_canvas_draw_line.
     * v9 used lv_canvas_init_layer / lv_draw_line / lv_canvas_finish_layer. */
    lv_draw_line_dsc_t line_dsc;
    lv_draw_line_dsc_init(&line_dsc);
    line_dsc.color = lv_color_hex(p->layer_wheel);
    line_dsc.width = 4;
    line_dsc.opa = LV_OPA_COVER;
    line_dsc.round_start = 1;
    line_dsc.round_end = 1;

    for (int i = 0; i < num_ticks; i++) {
        float angle = ((float)i * 360.0f / num_ticks - 90.0f) * M_PI / 180.0f;

        lv_point_t pts[2];
        pts[0].x = WHEEL_CENTER + (int)(WHEEL_INNER_RADIUS * cosf(angle));
        pts[0].y = WHEEL_CENTER + (int)(WHEEL_INNER_RADIUS * sinf(angle));
        pts[1].x = WHEEL_CENTER + (int)(WHEEL_OUTER_RADIUS * cosf(angle));
        pts[1].y = WHEEL_CENTER + (int)(WHEEL_OUTER_RADIUS * sinf(angle));

        lv_canvas_draw_line(wheel_canvas, pts, 2, &line_dsc);
    }
}

static void rotate_wheel(uint8_t target_layer, uint8_t layer_count) {
    if (!wheel_image) return;

    int num_layers = (layer_count > 0 && layer_count <= 16) ? layer_count : 6;
    int32_t angle_per_layer = 3600 / num_layers;
    int32_t target_angle = -(target_layer * angle_per_layer);

    /* v8: lv_img_get_angle (was lv_image_get_rotation in v9) */
    int32_t current_angle = lv_img_get_angle(wheel_image);

    int32_t diff = target_angle - current_angle;
    while (diff > 1800) { target_angle -= 3600; diff = target_angle - current_angle; }
    while (diff < -1800) { target_angle += 3600; diff = target_angle - current_angle; }

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, wheel_image);
    lv_anim_set_values(&a, current_angle, target_angle);
    lv_anim_set_time(&a, 150);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    /* v8: lv_img_set_angle (was lv_image_set_rotation in v9) */
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_img_set_angle);
    lv_anim_start(&a);
}

/* ========== Create Functions ========== */

static void create_left_panel(lv_obj_t *parent) {
    const radii_color_palette_t *p = &color_palettes[current_palette];

    left_panel = lv_obj_create(parent);
    lv_obj_set_size(left_panel, 172, 240);
    lv_obj_set_pos(left_panel, 0, 0);
    lv_obj_set_style_bg_color(left_panel, lv_color_hex(p->left_panel_bg), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(left_panel, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(left_panel, 24, LV_PART_MAIN);
    lv_obj_set_style_border_width(left_panel, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(left_panel, 0, LV_PART_MAIN);
    lv_obj_clear_flag(left_panel, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *layer_container = lv_obj_create(left_panel);
    lv_obj_set_size(layer_container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_pos(layer_container, 14, 20);
    lv_obj_set_style_bg_opa(layer_container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(layer_container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(layer_container, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(layer_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(layer_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(layer_container, 12, LV_PART_MAIN);
    lv_obj_clear_flag(layer_container, LV_OBJ_FLAG_SCROLLABLE);

    /* Canvas for wheel drawing */
    wheel_canvas = lv_canvas_create(layer_container);
    /* v8: LV_IMG_CF_TRUE_COLOR_ALPHA (was LV_COLOR_FORMAT_ARGB8888 in v9) */
    lv_canvas_set_buffer(wheel_canvas, wheel_canvas_buf, WHEEL_SIZE, WHEEL_SIZE,
                         LV_IMG_CF_TRUE_COLOR_ALPHA);
    lv_obj_add_flag(wheel_canvas, LV_OBJ_FLAG_HIDDEN);
    draw_wheel(6);

    /* Rotatable image displaying the canvas contents */
    /* v8: lv_img_create (was lv_image_create in v9) */
    wheel_image = lv_img_create(layer_container);
    /* v8: lv_img_set_src (was lv_image_set_src in v9) */
    lv_img_set_src(wheel_image, lv_canvas_get_image(wheel_canvas));
    /* v8: lv_img_set_pivot (was lv_image_set_pivot in v9) */
    lv_img_set_pivot(wheel_image, WHEEL_CENTER, WHEEL_CENTER);
    /* v8: lv_img_set_angle (was lv_image_set_rotation in v9) */
    lv_img_set_angle(wheel_image, 0);

    layer_label = lv_label_create(layer_container);
    lv_obj_set_style_text_font(layer_label, &DINishExpanded_Light_36, LV_PART_MAIN);
    lv_obj_set_style_text_color(layer_label, lv_color_hex(p->layer_text), LV_PART_MAIN);
    lv_obj_set_width(layer_label, 148);
    lv_label_set_long_mode(layer_label, LV_LABEL_LONG_WRAP);
    lv_label_set_text(layer_label, "BASE");

    current_layer = 0;
    current_layer_count = 6;
}

static void create_modifier_panel(lv_obj_t *parent) {
    const radii_color_palette_t *p = &color_palettes[current_palette];

    mod_panel = lv_obj_create(parent);
    lv_obj_set_size(mod_panel, 108, 178);
    lv_obj_align(mod_panel, LV_ALIGN_BOTTOM_RIGHT, 0, -62);
    lv_obj_set_style_bg_color(mod_panel, lv_color_hex(p->mod_panel_bg), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(mod_panel, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(mod_panel, 24, LV_PART_MAIN);
    lv_obj_set_style_border_width(mod_panel, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(mod_panel, 0, LV_PART_MAIN);
    lv_obj_clear_flag(mod_panel, LV_OBJ_FLAG_SCROLLABLE);

    static const int32_t mod_positions[4][2] = {
        {18, 24}, {56, 24},
        {18, 62}, {56, 62}
    };
    static const char *mod_symbols[] = {
        SYMBOL_COMMAND,
        SYMBOL_OPTION,
        SYMBOL_CONTROL,
        SYMBOL_SHIFT
    };

    for (int i = 0; i < 4; i++) {
        mod_labels[i] = lv_label_create(mod_panel);
        lv_label_set_text(mod_labels[i], mod_symbols[i]);
        lv_obj_set_style_text_font(mod_labels[i], &Symbols_Semibold_32, LV_PART_MAIN);
        lv_obj_set_style_text_color(mod_labels[i], lv_color_hex(p->mod_inactive), LV_PART_MAIN);
        lv_obj_set_pos(mod_labels[i], mod_positions[i][0], mod_positions[i][1]);
    }
}

static lv_obj_t *create_arc(lv_obj_t *parent, int size, int x, int y, int width) {
    const radii_color_palette_t *p = &color_palettes[current_palette];

    lv_obj_t *arc = lv_arc_create(parent);
    lv_obj_set_size(arc, size, size);
    lv_obj_set_pos(arc, x, y);
    lv_arc_set_range(arc, 0, 100);
    lv_arc_set_value(arc, 0);
    lv_arc_set_bg_angles(arc, 0, 360);
    lv_arc_set_rotation(arc, 270);
    lv_obj_set_style_arc_width(arc, width, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, lv_color_hex(p->arc_bg), LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(arc, true, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, width, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, lv_color_hex(p->arc_bg), LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(arc, true, LV_PART_INDICATOR);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    return arc;
}

static void create_battery_panel(lv_obj_t *parent) {
    const radii_color_palette_t *p = &color_palettes[current_palette];

    bat_panel = lv_obj_create(parent);
    lv_obj_set_size(bat_panel, 108, 62);
    lv_obj_set_pos(bat_panel, 172, 178);
    lv_obj_set_style_bg_color(bat_panel, lv_color_hex(p->battery_panel_bg), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bat_panel, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(bat_panel, 24, LV_PART_MAIN);
    lv_obj_set_style_border_width(bat_panel, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(bat_panel, 0, LV_PART_MAIN);
    lv_obj_clear_flag(bat_panel, LV_OBJ_FLAG_SCROLLABLE);

    int arc_size = 30;
    int left_pad = 19;
    int arc_gap = 10;
    int y_center = (62 - arc_size) / 2;

    bat_arc_left = create_arc(bat_panel, arc_size, left_pad, y_center, 6);
    bat_arc_right = create_arc(bat_panel, arc_size, left_pad + arc_size + arc_gap, y_center, 6);
}

/* ========== Update Functions ========== */

static void update_modifiers(uint8_t flags) {
    const radii_color_palette_t *p = &color_palettes[current_palette];

    bool mods[4] = {
        (flags & 0x88) != 0,
        (flags & 0x44) != 0,
        (flags & 0x11) != 0,
        (flags & 0x22) != 0,
    };

    for (int i = 0; i < 4; i++) {
        if (mod_labels[i]) {
            lv_color_t color = mods[i] ? lv_color_hex(p->mod_active)
                                       : lv_color_hex(p->mod_inactive);
            lv_obj_set_style_text_color(mod_labels[i], color, LV_PART_MAIN);
        }
    }
}

static void update_battery(lv_obj_t *arc, uint8_t level, bool connected) {
    const radii_color_palette_t *p = &color_palettes[current_palette];

    if (!arc) return;
    if (connected && level > 0) {
        lv_arc_set_value(arc, level);
        lv_obj_set_style_arc_color(arc, lv_color_hex(p->arc_indicator), LV_PART_INDICATOR);
    } else {
        lv_arc_set_value(arc, 0);
        lv_obj_set_style_arc_color(arc, lv_color_hex(p->arc_bg), LV_PART_INDICATOR);
    }
}

/* ========== Palette Application ========== */

static void apply_palette(void) {
    if (!layout_created) return;

    const radii_color_palette_t *p = &color_palettes[current_palette];

    if (left_panel) {
        lv_obj_set_style_bg_color(left_panel, lv_color_hex(p->left_panel_bg), LV_PART_MAIN);
    }
    if (layer_label) {
        lv_obj_set_style_text_color(layer_label, lv_color_hex(p->layer_text), LV_PART_MAIN);
    }
    if (wheel_canvas && wheel_image) {
        draw_wheel(current_layer_count);
        lv_img_set_src(wheel_image, lv_canvas_get_image(wheel_canvas));
    }
    if (mod_panel) {
        lv_obj_set_style_bg_color(mod_panel, lv_color_hex(p->mod_panel_bg), LV_PART_MAIN);
    }
    for (int i = 0; i < 4; i++) {
        if (mod_labels[i]) {
            lv_obj_set_style_text_color(mod_labels[i], lv_color_hex(p->mod_inactive), LV_PART_MAIN);
        }
    }
    if (bat_panel) {
        lv_obj_set_style_bg_color(bat_panel, lv_color_hex(p->battery_panel_bg), LV_PART_MAIN);
    }
    if (bat_arc_left) {
        lv_obj_set_style_arc_color(bat_arc_left, lv_color_hex(p->arc_bg), LV_PART_MAIN);
    }
    if (bat_arc_right) {
        lv_obj_set_style_arc_color(bat_arc_right, lv_color_hex(p->arc_bg), LV_PART_MAIN);
    }

    LOG_INF("Applied palette: %s", p->name);
}

/* ========== Public API ========== */

lv_obj_t *radii_layout_create(lv_obj_t *parent) {
    if (layout_created) {
        LOG_WRN("Radii layout already created");
        return parent;
    }

    parent_screen = parent;

    lv_obj_set_style_bg_color(parent, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, LV_PART_MAIN);

    create_left_panel(parent);
    create_modifier_panel(parent);
    create_battery_panel(parent);

    layout_created = true;
    LOG_INF("Radii layout created (%s theme)", color_palettes[current_palette].name);
    return parent;
}

void radii_layout_update(uint8_t active_layer, const char *layer_name,
                        uint8_t battery_level, bool battery_connected,
                        uint8_t peripheral_battery, bool peripheral_connected,
                        uint8_t modifier_flags, bool usb_connected, uint8_t ble_profile) {
    if (!layout_created) return;

    uint8_t layer_count = 6;
#ifdef CONFIG_PROSPECTOR_MAX_LAYERS
    layer_count = CONFIG_PROSPECTOR_MAX_LAYERS;
#endif

    if (layer_count != current_layer_count && wheel_canvas && wheel_image) {
        draw_wheel(layer_count);
        lv_img_set_src(wheel_image, lv_canvas_get_image(wheel_canvas));
        current_layer_count = layer_count;
    }

    if (active_layer != current_layer) {
        rotate_wheel(active_layer, layer_count);
        current_layer = active_layer;
    }

    if (layer_label) {
        if (layer_name && layer_name[0]) {
            static char upper_name[32];
            int i;
            for (i = 0; layer_name[i] && i < 31; i++) {
                upper_name[i] = (layer_name[i] >= 'a' && layer_name[i] <= 'z')
                                ? layer_name[i] - 32 : layer_name[i];
            }
            upper_name[i] = '\0';
            lv_label_set_text(layer_label, upper_name);
        } else {
            char text[16];
            snprintf(text, sizeof(text), "%d", active_layer);
            lv_label_set_text(layer_label, text);
        }
    }

    update_battery(bat_arc_left, battery_level, battery_connected);
    update_battery(bat_arc_right, peripheral_battery, peripheral_connected);

    update_modifiers(modifier_flags);

    if (parent_screen) {
        lv_obj_invalidate(parent_screen);
    }
}

void radii_layout_destroy(void) {
    if (!layout_created) return;

    if (left_panel) { lv_obj_del(left_panel); left_panel = NULL; }
    if (mod_panel) { lv_obj_del(mod_panel); mod_panel = NULL; }
    if (bat_panel) { lv_obj_del(bat_panel); bat_panel = NULL; }

    wheel_canvas = NULL;
    wheel_image = NULL;
    layer_label = NULL;
    for (int i = 0; i < 4; i++) mod_labels[i] = NULL;
    bat_arc_left = NULL;
    bat_arc_right = NULL;

    parent_screen = NULL;
    layout_created = false;
    current_layer = 0;
    current_layer_count = 6;

    LOG_INF("Radii layout destroyed");
}

void radii_layout_cycle_palette(void) {
    if (!layout_created) return;
    current_palette = (current_palette + 1) % PALETTE_COUNT;
    apply_palette();
}

uint8_t radii_layout_get_palette(void) {
    return current_palette;
}

const char *radii_layout_get_palette_name(void) {
    return color_palettes[current_palette].name;
}
