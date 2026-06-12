# This file is intentionally empty.
#
# The prospector_scanner shield is fully defined in prospector-zmk-module.
# ZMK does not include this file during the build because the shield definition
# (and its CMakeLists.txt) comes from the module, not from this overlay directory.
#
# LVGL v8 / ZMK v0.3-branch compatibility patches live in the repository root
# CMakeLists.txt, which IS processed as a Zephyr module cmake entry point
# (zephyr/module.yml → cmake: .).
