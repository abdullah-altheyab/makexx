#!/bin/sh
set -e

REPO="abdul900/makexx"
PREFIX="${PREFIX:-/usr/local}"
INSTALL_DIR="${PREFIX}/bin"

detect_platform() {
    OS="$(uname -s)"
    ARCH="$(uname -m)"

    case "$OS" in
        Linux)  OS_NAME="linux" ;;
        Darwin) OS_NAME="macos" ;;
        *)      echo "error: unsupported OS: $OS" >&2; exit 1 ;;
    esac

    case "$ARCH" in
        x86_64|amd64)  ARCH_NAME="x86_64" ;;
        aarch64|arm64) ARCH_NAME="aarch64" ;;
        *)             echo "error: unsupported architecture: $ARCH" >&2; exit 1 ;;
    esac

    if [ "$OS_NAME" = "macos" ] && [ "$ARCH_NAME" = "aarch64" ]; then
        ARCH_NAME="arm64"
    fi

    ARTIFACT="makexx-${OS_NAME}-${ARCH_NAME}"
}

get_latest_version() {
    if command -v curl >/dev/null 2>&1; then
        VERSION=$(curl -fsSL "https://api.github.com/repos/${REPO}/releases/latest" | grep '"tag_name"' | sed 's/.*"tag_name": *"//;s/".*//')
    elif command -v wget >/dev/null 2>&1; then
        VERSION=$(wget -qO- "https://api.github.com/repos/${REPO}/releases/latest" | grep '"tag_name"' | sed 's/.*"tag_name": *"//;s/".*//')
    else
        echo "error: curl or wget required" >&2; exit 1
    fi

    if [ -z "$VERSION" ]; then
        echo "error: could not determine latest version" >&2; exit 1
    fi
}

download_and_install() {
    URL="https://github.com/${REPO}/releases/download/${VERSION}/${ARTIFACT}.tar.gz"
    TMPDIR=$(mktemp -d)
    trap 'rm -rf "$TMPDIR"' EXIT

    echo "Downloading makexx ${VERSION} (${ARTIFACT})..."
    if command -v curl >/dev/null 2>&1; then
        curl -fsSL "$URL" -o "$TMPDIR/makexx.tar.gz"
    else
        wget -qO "$TMPDIR/makexx.tar.gz" "$URL"
    fi

    tar -xzf "$TMPDIR/makexx.tar.gz" -C "$TMPDIR"
    chmod +x "$TMPDIR/$ARTIFACT"

    if [ -w "$INSTALL_DIR" ]; then
        mv "$TMPDIR/$ARTIFACT" "$INSTALL_DIR/makexx"
    else
        echo "Installing to ${INSTALL_DIR} (requires sudo)..."
        sudo mv "$TMPDIR/$ARTIFACT" "$INSTALL_DIR/makexx"
    fi

    echo "makexx ${VERSION} installed to ${INSTALL_DIR}/makexx"
}

detect_platform
VERSION="${1:-}"
if [ -z "$VERSION" ]; then
    get_latest_version
fi
download_and_install
