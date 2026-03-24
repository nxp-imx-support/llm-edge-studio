#!/usr/bin/env bash
#
# Copyright 2025-2026 NXP
#
# NXP Proprietary. This software is owned or controlled by NXP and may only be
# used strictly in accordance with the applicable license terms.  By expressly
# accepting such terms or by downloading, installing, activating and/or
# otherwise using the software, you are agreeing that you have read, and that
# you agree to comply with and are bound by, such license terms.  If you do
# not agree to be bound by the applicable license terms, then you may not
# retain, install, activate or otherwise use the software.

set -e # Exit on any error

TOOLCHAIN_PATH="$1"
BUILD_DIR="build"
PACKAGE_DIR="llm-edge-studio"
BIN_NAME="llm_edge_studio"

RED='\033[1;31m'
NC='\033[0m' # No Color

PACKAGE="📦"
WRENCH="🔧"

if [ -z "$TOOLCHAIN_PATH" ]; then
	echo "Usage: $0 <toolchain_path>"
	exit 1
fi
if [ ! -e "$TOOLCHAIN_PATH" ]; then
	echo "Error: Path '$TOOLCHAIN_PATH' does not exist."
	echo -e "${RED}ERROR:${NC} Path '$TOOLCHAIN_PATH' does not exist."
	exit 1
fi

echo "Toolchain path '$TOOLCHAIN_PATH' exists. Sourcing for cross-compilation."

# Source your toolchain
source "$TOOLCHAIN_PATH"

# Create or reuse build directory
if [ ! -d "$BUILD_DIR" ]; then
	echo "Creating build directory for $PACKAGE_DIR..."
	mkdir "$BUILD_DIR"
else
	echo "Reusing existing build directory for $PACKAGE_DIR..."
fi

# Navigate into build
cd "$BUILD_DIR"

# Run CMake
echo " $WRENCH Running CMake..."
cmake -DCMAKE_BUILD_TYPE=Release .. || {
	echo -e "${RED}ERROR:${NC} CMake failed for $PACKAGE_DIR"
	exit 1
}

# Compile with make
echo "$WRENCH Compiling..."
make -j$(nproc) || {
	echo -e "${RED}ERROR:${NC} Compilation failed for $PACKAGE_DIR"
	exit 1
}

echo "Build for $PACKAGE_DIR completed successfully."

# Move the binary to the correct path
cp $BIN_NAME ../$PACKAGE_DIR/usr/share/$PACKAGE_DIR/

# Move to root path of repository
cd ../

# Build deb package
if [ ! -d "$PACKAGE_DIR" ]; then
	echo -e "${RED}ERROR:${NC} Package directory '$PACKAGE_DIR' not found."
	exit 1
fi

echo "$PACKAGE Creating DEB package..."
dpkg-deb --build "$PACKAGE_DIR" || {
	echo "DEB package creation failed"
	exit 1
}

echo "Build and packaging completed successfully."
