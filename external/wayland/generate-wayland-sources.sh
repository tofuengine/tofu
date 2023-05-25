#!/bin/bash
#
# MIT License
#
# Copyright (c) 2019-2023 Marco Lizza
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

wayland-scanner client-header /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml xdg-shell-client-protocol.h
wayland-scanner client-header /usr/share/wayland-protocols/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml xdg-decoration-client-protocol.h
wayland-scanner client-header /usr/share/wayland-protocols/stable/viewporter/viewporter.xml viewporter-client-protocol.h
wayland-scanner client-header /usr/share/wayland-protocols/unstable/relative-pointer/relative-pointer-unstable-v1.xml relative-pointer-unstable-v1-client-protocol.h
wayland-scanner client-header /usr/share/wayland-protocols/unstable/pointer-constraints/pointer-constraints-unstable-v1.xml pointer-constraints-unstable-v1-client-protocol.h
wayland-scanner client-header /usr/share/wayland-protocols/unstable/idle-inhibit/idle-inhibit-unstable-v1.xml idle-inhibit-unstable-v1-client-protocol.h

wayland-scanner private-code /usr/share/wayland-protocols//stable/xdg-shell/xdg-shell.xml xdg-shell-client-protocol.c
wayland-scanner private-code /usr/share/wayland-protocols/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml xdg-decoration-client-protocol.c
wayland-scanner private-code /usr/share/wayland-protocols/stable/viewporter/viewporter.xml viewporter-client-protocol.c
wayland-scanner private-code /usr/share/wayland-protocols/unstable/relative-pointer/relative-pointer-unstable-v1.xml relative-pointer-unstable-v1-client-protocol.c
wayland-scanner private-code /usr/share/wayland-protocols/unstable/pointer-constraints/pointer-constraints-unstable-v1.xml pointer-constraints-unstable-v1-client-protocol.c
wayland-scanner private-code /usr/share/wayland-protocols/unstable/idle-inhibit/idle-inhibit-unstable-v1.xml idle-inhibit-unstable-v1-client-protocol.c