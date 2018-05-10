cmake_minimum_required(VERSION 3.0)
project(Paint)

add_subdirectory(src)

# Copy share/ directory as is to the installation folder. The share directory
# contains external data required by the executables, like images or shaders.
file(COPY share DESTINATION .)
