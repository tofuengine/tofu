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

FROM ubuntu:22.04

RUN apt-get update && \
        apt-get upgrade -y && \
        apt-get install -y \
        git \
        build-essential \
        mingw-w64 \
        xorg-dev \
        libx11-dev \
        libwayland-dev \
        libxkbcommon-dev \
        wayland-protocols \
        mesa-common-dev \
        libgles2-mesa-dev \
        cppcheck \
        valgrind

RUN apt-get install -y \
        ccache \
        sudo

RUN apt-get install -y \
        python3

RUN apt-get install -y \
        lua5.4 \
        liblua5.4-dev \
        luarocks

RUN apt-get -f install -y

# cleanup
RUN apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

# luarocks dependencies
RUN luarocks --lua-version=5.4 install argparse
RUN luarocks --lua-version=5.4 install luafilesystem
RUN luarocks --lua-version=5.4 install luacheck
RUN luarocks --lua-version=5.4 install luazen

# the persistent data will be in these directories, everything else is considered to be ephemeral
VOLUME ["/tmp/ccache", "/tofu"]

# improve rebuild performance by enabling compiler cache
ENV USE_CCACHE 1
ENV CCACHE_DIR /tmp/ccache

# work in the build directory, repo is expected to be init'd here
WORKDIR /tofu
COPY docker_entrypoint.sh /root/docker_entrypoint.sh
ENTRYPOINT ["/root/docker_entrypoint.sh"]
