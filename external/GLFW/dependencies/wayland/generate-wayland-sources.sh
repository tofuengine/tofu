#!/bin/bash
#
# MIT License
#
# Copyright (c) 2019-2024 Marco Lizza
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

wayland-scanner client-header ./wayland.xml ../wayland-client-protocol.h
wayland-scanner client-header ./viewporter.xml ../viewporter-client-protocol.h
wayland-scanner client-header ./xdg-shell.xml ../xdg-shell-client-protocol.h
wayland-scanner client-header ./idle-inhibit-unstable-v1.xml ../idle-inhibit-unstable-v1-client-protocol.h
wayland-scanner client-header ./pointer-constraints-unstable-v1.xml ../pointer-constraints-unstable-v1-client-protocol.h
wayland-scanner client-header ./relative-pointer-unstable-v1.xml ../relative-pointer-unstable-v1-client-protocol.h
wayland-scanner client-header ./fractional-scale-v1.xml ../fractional-scale-v1-client-protocol.h
wayland-scanner client-header ./xdg-activation-v1.xml ../xdg-activation-v1-client-protocol.h
wayland-scanner client-header ./xdg-decoration-unstable-v1.xml ../xdg-decoration-unstable-v1-client-protocol.h

wayland-scanner private-code ./wayland.xml ../wayland-client-protocol-code.h
wayland-scanner private-code ./viewporter.xml ../viewporter-client-protocol-code.h
wayland-scanner private-code ./xdg-shell.xml ../xdg-shell-client-protocol-code.h
wayland-scanner private-code ./idle-inhibit-unstable-v1.xml ../idle-inhibit-unstable-v1-client-protocol-code.h
wayland-scanner private-code ./pointer-constraints-unstable-v1.xml ../pointer-constraints-unstable-v1-client-protocol-code.h
wayland-scanner private-code ./relative-pointer-unstable-v1.xml ../relative-pointer-unstable-v1-client-protocol-code.h
wayland-scanner private-code ./fractional-scale-v1.xml ../fractional-scale-v1-client-protocol-code.h
wayland-scanner private-code ./xdg-activation-v1.xml ../xdg-activation-v1-client-protocol-code.h
wayland-scanner private-code ./xdg-decoration-unstable-v1.xml ../xdg-decoration-unstable-v1-client-protocol-code.h
