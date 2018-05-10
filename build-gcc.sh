#!/bin/bash

# Get git submodules
git submodule update --init --recursive

# (This is a script for Linux)
# Place all build related files in a specific directory.
# Whenever you'd like to clean the build and restart it from scratch, you can
# delete this directory without worrying about deleting important files.
mkdir build-gcc
cd build-gcc

# Call cmake to generate the Makefile. You can then build with 'make' and
# install with 'make install'
cmake ..

# Check that it run all right
if [ $? -eq 0 ]
then
	echo [92mSuccessful[0m
	echo You can now cd to build-gcc and run 'make'
else
	echo [91mUnsuccessful[0m
fi

if [ -z "$NONINTERACTIVE" ]; then read -p "Press enter to continue..."; fi
