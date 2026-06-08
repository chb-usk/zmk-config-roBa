#include <zmk/keymap.h>

/* Weak stubs so the PMW3610 driver (kumamuk-git) links on Peripheral builds.
 * keymap.c is only compiled for Central; these stubs satisfy the linker on
 * Peripheral builds.  Central builds use the strong definitions from keymap.c. */

__attribute__((weak)) zmk_keymap_layer_index_t zmk_keymap_highest_layer_active(void) {
    return 0;
}

__attribute__((weak)) int zmk_keymap_layer_activate(zmk_keymap_layer_id_t layer) {
    return 0;
}

__attribute__((weak)) int zmk_keymap_layer_deactivate(zmk_keymap_layer_id_t layer) {
    return 0;
}
