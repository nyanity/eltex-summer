#!/bin/bash

set -e

KERNEL_RELEASE=$(uname -r)
echo "Kernel version: $KERNEL_RELEASE"

if grep -qi microsoft /proc/version; then
    IS_WSL=true
    echo "System type: WSL2"
else
    IS_WSL=false
    echo "System type: Linux"
fi

if [ "$IS_WSL" = false ]; then
    echo "=== Installing kernel module headers ==="
    sudo apt-get update
    sudo apt-get install -y build-essential linux-headers-$(uname -r)
    echo "Done."
else
    echo "=== Donwloading and building WSL2 kernel module headers ==="
    
    KERNEL_VERSION=$(echo "$KERNEL_RELEASE" | cut -d'-' -f1)
    echo "Clean version WSL2: $KERNEL_VERSION"
    
    echo "Installing tools..."
    sudo apt-get update
    sudo apt-get install -y build-essential flex bison libssl-dev libelf-dev \
                            libncurses-dev autoconf libtool cpio dwarves wget tar

    SRC_DIR="/home/$USER/WSL2-Linux-Kernel"
    TAG_NAME="linux-msft-wsl-${KERNEL_VERSION}"
    TAR_FILE="linux-msft-wsl-${KERNEL_VERSION}.tar.gz"
    
    echo "Downloading $KERNEL_VERSION..."
    cd "$HOME"
    
    if [ ! -f "$TAR_FILE" ]; then
        wget "https://github.com/microsoft/WSL2-Linux-Kernel/archive/refs/tags/${TAG_NAME}.tar.gz" -O "$TAR_FILE"
    else
        echo "File $TAR_FILE is downloaded."
    fi

    # Распаковка архива
    echo "Unzip..."
    if [ -d "WSL2-Linux-Kernel-${TAG_NAME}" ]; then
        rm -rf "WSL2-Linux-Kernel-${TAG_NAME}"
    fi
    tar -zxf "$TAR_FILE"

    echo "Mkdir..."
    rm -rf "$SRC_DIR"
    mv "WSL2-Linux-Kernel-${TAG_NAME}" "$SRC_DIR"

    cd "$SRC_DIR"
    echo "zcat config..."
    zcat /proc/config.gz > .config

    echo "module_prerape..."
    make modules_prepare

    echo "Build..."
    make modules -j$(nproc)

    echo "Creating symbolic link..."
    sudo mkdir -p "/lib/modules/$KERNEL_RELEASE"
    sudo ln -nsf "$SRC_DIR" "/lib/modules/$KERNEL_RELEASE/build"

    echo "=========================================================="
    echo "Done."
    echo "Source: $SRC_DIR"
    echo "Link: /lib/modules/$KERNEL_RELEASE/build"
    echo "=========================================================="
fi