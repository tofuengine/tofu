#!/bin/bash

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

# docker entry point script used for allowing executables in the docker
# container to manipulate files in shared volumes owned by the USER_ID:GROUP_ID
# (currently open docker issue #7198). It creates a user named `builder:builders`
# with selected USER_ID and GROUP_ID, or 1000 if not specified.

# example:
#
#  docker run -ti -e USER_ID=$(id -u) -e GROUP_ID=$(id -g) imagename bash
#

USERNAME=builder
GROUPNAME=builders
HOME="/home/$USERNAME"
CREATE_HOME_PARAM="--no-create-home"

# the persistent data will be in these directories, everything else is considered to be ephemeral
PERSISTENT_DIRS="/tmp/ccache /tofu"

# set default USER_ID/GROUP_ID if no USER_ID/GROUP_ID environment variables are set
if [ -z ${USER_ID+x} ]; then USER_ID=1000; fi
if [ -z ${GROUP_ID+x} ]; then GROUP_ID=1000; fi

msg="docker_entrypoint: creating user UID/GID [$USER_ID/$GROUP_ID]" && echo $msg
if [ ! -d "$HOME" ]; then
    CREATE_HOME_PARAM="--create-home"
fi
groupadd -g $GROUP_ID -r $GROUPNAME && \
useradd -u $USER_ID $CREATE_HOME_PARAM -r -g $GROUPNAME $USERNAME

msg="docker_entrypoint: adding $USERNAME to container sudoers" && echo $msg
adduser --quiet $USERNAME sudo > /dev/null
echo "$msg - done"

# adding default .gitconfig and .ssh/config only if not already present
if [ ! -f "$HOME/.gitconfig" ]; then
    msg="docker_entrypoint: Adding a default .gitconfig to new user home" && echo $msg
    cp /root/.gitconfig "$HOME/.gitconfig" && \
    chown $USERNAME:$GROUPNAME "$HOME/.gitconfig"
    echo "$msg - done"
fi

if [ ! -f "$HOME/.ssh/config" ]; then
    msg="docker_entrypoint: Adding a default .ssh/config to new user home" && echo $msg
    mkdir -p "$HOME/.ssh" && \
    cp /root/.ssh/config "$HOME/.ssh/config" && \
    chown $USERNAME:$GROUPNAME -R "$HOME/.ssh"
    echo "$msg - done"
fi

msg="docker_entrypoint: creating persistent directories" && echo $msg
for DIR in $PERSISTENT_DIRS;
do
    mkdir -p "$DIR"
    chown $USERNAME:$GROUPNAME "$DIR"
done
echo "$msg - done"

echo ""

# default to 'bash' if no arguments are provided
args="$@"
if [ -z "$args" ]; then
    args="bash"
fi

# execute command as `$USERNAME` user
export HOME="$HOME"
exec sudo -u $USERNAME $args
