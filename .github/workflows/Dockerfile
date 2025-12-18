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
        cmake \
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
RUN pip install --no-cache-dir "conan>=2"

RUN if ! getent group "${USERNAME}" >/dev/null 2>&1; then \
        groupadd --gid "${GID}" -o "${USERNAME}"; \
    fi && \
    if ! id -u "${USERNAME}" >/dev/null 2>&1; then \
        useradd --uid "${UID}" --gid "${GID}" --non-unique -m "${USERNAME}"; \
    fi

WORKDIR /workspace
COPY . /workspace
RUN conan profile detect
RUN conan install . --output-folder /conan_build --build=missing -c tools.system.package_manager:mode=install
RUN chown -R ${USERNAME}:${USERNAME} /workspace /conan_build

USER ${USERNAME}
ENV HOME=/home/${USERNAME}
WORKDIR /workspace
