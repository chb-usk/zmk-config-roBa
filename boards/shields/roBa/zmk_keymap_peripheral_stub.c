#include <zmk/keymap.h>
#include <zmk/behavior_queue.h>

/* Weak stubs so the kumamuk PMW3610 driver links on Peripheral builds.
 * keymap.c and behavior_queue.c are only compiled for Central; these stubs
 * satisfy the linker on Peripheral.  Central builds use the strong definitions. */

__attribute__((weak)) zmk_keymap_layer_index_t zmk_keymap_highest_layer_active(void) {
    return 0;
}

__attribute__((weak)) int zmk_keymap_layer_activate(zmk_keymap_layer_id_t layer) {
    return 0;
}

__attribute__((weak)) int zmk_keymap_layer_deactivate(zmk_keymap_layer_id_t layer) {
    return 0;
}

__attribute__((weak)) int zmk_behavior_queue_add(const struct zmk_behavior_binding_event *event,
                                                  const struct zmk_behavior_binding behavior,
                                                  bool press, uint32_t wait) {
    return 0;
}
