#!/bin/bash

set -e

WORKDIR=$(pwd)
PROJECT_TOP=$(cd $(dirname $0); pwd)

TARGET=debfile-master

BUILD_DIR=$WORKDIR/.build_deb
BUILD_DEBIAN_DIR=$BUILD_DIR/DEBIAN
INSTALL_BASE=$BUILD_DIR/opt/kylin-utils
INSTALL_BIN=$INSTALL_BASE/bin
INSTALL_LIB=$INSTALL_BASE/lib
INSTALL_DATA=$INSTALL_BASE/share/$TARGET
INSTALL_DESKTOP=$INSTALL_DATA/applications
INSTALL_SYS_DESKTOP=$BUILD_DIR/usr/share/applications

# Construct Build Entry
mkdir -p $BUILD_DIR/DEBIAN
cp -at $BUILD_DEBIAN_DIR $PROJECT_TOP/debian/*

mkdir -p $INSTALL_BIN
cp -at $INSTALL_BIN $PROJECT_TOP/debfile-master

mkdir -p $INSTALL_DATA
cp -at $INSTALL_DATA $PROJECT_TOP/resources/logo.png

mkdir -p $INSTALL_SYS_DESKTOP
cp -at $INSTALL_SYS_DESKTOP $PROJECT_TOP/debfile-master.desktop

# Update Control and Desktop file
DESKTOP_FILE=$INSTALL_SYS_DESKTOP/debfile-master.desktop
CONTROL_FILE=$BUILD_DEBIAN_DIR/control

# Usage: UpdateValue <File> <Separator> <Key> <Value>
function UpdateValue()
{
    local File=$1
    local Separator=$2
    local Key=$3
    local Value=$4
    if [ $# -ne 4 ]; then
        echo "Usage: UpdateValue <File> <Separator> <Key> <Value>"
        return 1
    fi

    local Line=$(grep -n "^${Key}${Separator}" $File | awk -F: '{print $1}' | sed -n 1p)
    if [ -n "${Line}" ]; then
        if ! sed -i "${Line}c${Key}${Separator}${Value}" $File; then
            echo "Update Value of Key: $Key in file $File failed!"
            return 1;
        fi
    fi
}

# Update control file
PKG_NAME=$(grep "Package" $CONTROL_FILE | awk '{print $2}')
PKG_VER=$(grep '^VERSION' $TARGET.pro | awk '{print $3}')
PKG_ARCH=$(dpkg --print-architecture)

UpdateValue $DESKTOP_FILE "=" "Version" "$PKG_VER"
UpdateValue $CONTROL_FILE ":\ " "Version" "$PKG_VER"
UpdateValue $CONTROL_FILE ":\ " "Architecture" "$PKG_ARCH"

# Build Package
fakeroot dpkg-deb -b $BUILD_DIR ${PKG_NAME}_${PKG_VER}_${PKG_ARCH}.deb || exit $?
rm -rf $BUILD_DIR
