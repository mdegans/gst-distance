# This Dockerfile is for production
# it clones from master and deletes the source
# use this when size matters most

# Copyright (C) 2020  Michael de Gans

# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.

# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.

# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
# USA

# TODO(mdegans): find way to get gstreamer plugin path and just install/link
#  the plugin there rather than installing everything to /usr prefix

ARG DSFILTER_TAG="UNSET (use docker_build.sh to build)"
ARG AUTHOR="UNSET (use docker_build.sh to build)"
FROM registry.hub.docker.com/${AUTHOR}/libdsfilter:${DSFILTER_TAG}

ARG PREFIX="/usr"
ARG SRCDIR="${PREFIX}/src/gst-cuda-plugin"

WORKDIR ${SRCDIR}
COPY meson.build COPYING VERSION README.md ./
COPY gst-cuda-plugin ./gst-cuda-plugin/

# install build deps, build, clean up
RUN apt-get update && apt-get install -y --no-install-recommends \
        cuda-compiler-10-2 \
        cuda-cudart-dev-10-2 \
        ninja-build \
        python3-pip \
        python3-setuptools \
        python3-wheel \
    && pip3 install meson \
    && mkdir build \
    && cd build \
    && meson --prefix=${PREFIX} .. \
    && ninja \
    && ninja install \
    && cd .. && rm -rf build \
    && ldconfig \
    && pip3 uninstall -y meson \
    && apt-get purge -y --autoremove \
        ninja-build \
        python3-pip \
        python3-setuptools \
        python3-wheel \
    && rm -rf /var/lib/apt/lists/*
