#!/bin/bash
# ASM64 Install Script
# Builds and installs asm64 to /usr/local/bin

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

INSTALL_DIR="${INSTALL_DIR:-/usr/local/bin}"

echo "ASM64 Installer"
echo "==============="
echo ""

# Check for C compiler
if ! command -v cc &> /dev/null && ! command -v gcc &> /dev/null; then
    echo -e "${RED}Error: No C compiler found. Please install gcc or clang.${NC}"
    exit 1
fi

# Build
echo "Building asm64..."
make clean 2>/dev/null || true
make

if [ ! -f "build/asm64" ]; then
    echo -e "${RED}Error: Build failed.${NC}"
    exit 1
fi

echo ""
echo "Installing to $INSTALL_DIR..."

# Check if we need sudo
if [ -w "$INSTALL_DIR" ]; then
    cp build/asm64 "$INSTALL_DIR/asm64"
    chmod +x "$INSTALL_DIR/asm64"
else
    echo "Requires sudo to install to $INSTALL_DIR"
    sudo cp build/asm64 "$INSTALL_DIR/asm64"
    sudo chmod +x "$INSTALL_DIR/asm64"
fi

echo ""
echo -e "${GREEN}Installation complete!${NC}"
echo ""
echo "Verify installation:"
echo "  asm64 --version"
echo ""
echo "To uninstall:"
echo "  sudo rm $INSTALL_DIR/asm64"
