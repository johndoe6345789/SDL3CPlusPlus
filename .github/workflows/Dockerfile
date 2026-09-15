# Build environment for the CI job: the system libraries, Conan 2, a pinned
# CMake, Ninja, and a Conan cache with every dependency in conanfile.py
# already compiled. Published per architecture by publish-container.yml so
# that build.yml only has to compile this repository.
ARG USERNAME=builder
ARG UID=1000
ARG GID=1000
FROM ubuntu:24.04
ARG USERNAME
ARG UID
ARG GID

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        ninja-build \
        libegl-dev \
        libegl1-mesa-dev \
        libgl-dev \
        libgl1-mesa-dev \
        libudev-dev \
        libwayland-dev \
        libxcb-util-dev \
        libxcb-util0-dev \
        libxkbcommon-dev \
        libxrandr-dev \
        libx11-dev \
        pkg-config \
        python3 \
        python3-pip \
        python3-venv \
        ca-certificates \
        git \
    && rm -rf /var/lib/apt/lists/*

RUN python3 -m venv /opt/venv
ENV PATH="/opt/venv/bin:${PATH}"
RUN pip install --upgrade pip
RUN pip install --no-cache-dir "conan>=2" "cmake==3.31.6"

# The warm cache has to be readable by whichever account ends up running the
# build - root under GitHub Actions, the builder user locally - so keep it out
# of any one home directory instead of letting it land in ~/.conan2.
ENV CONAN_HOME=/opt/conan2

# ConanCenter publishes no Linux ARM binaries, so on arm64 Conan would build
# its own CMake package, which means downloading it from cmake.org - and
# cmake.org/files now answers 403. Bring our own; 3.31.6 satisfies every cmake
# range in the graph. build.yml relies on this profile, so the two stay in
# step by virtue of the job running inside this image.
RUN conan profile detect \
    && { \
        echo ""; \
        echo "[platform_tool_requires]"; \
        echo "cmake/3.31.6"; \
    } >> "${CONAN_HOME}/profiles/default" \
    && cat "${CONAN_HOME}/profiles/default"

# Only the recipe is copied, so editing application sources never invalidates
# the layer that spends an hour building the dependency tree.
COPY conanfile.py /tmp/recipe/conanfile.py
RUN conan install /tmp/recipe \
        --output-folder /tmp/recipe/build \
        --build=missing \
        -s build_type=Release \
        -c tools.system.package_manager:mode=install \
        -c '&:tools.build:cxxflags=["-include","cstdint"]' \
    && conan cache clean \
    && rm -rf /tmp/recipe \
    && chmod -R a+rwX "${CONAN_HOME}"

RUN if ! getent group "${USERNAME}" >/dev/null 2>&1; then \
        groupadd --gid "${GID}" -o "${USERNAME}"; \
    fi && \
    if ! id -u "${USERNAME}" >/dev/null 2>&1; then \
        useradd --uid "${UID}" --gid "${GID}" --non-unique -m "${USERNAME}"; \
    fi

# The default user stays root: GitHub Actions mounts the workspace into a
# container job owned by root, and a non-root default breaks checkout. Local
# use can still pass `docker run --user builder`.
WORKDIR /workspace
