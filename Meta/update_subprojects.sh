#!/usr/bin/env zsh

rm -rf subprojects/*/
meson subprojects update --reset
meson subprojects packagefiles --apply
