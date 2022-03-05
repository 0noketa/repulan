# CMakeList.txt : CMake project for rul0i, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)


# Add source to this project's executable.
add_executable (rul0i "rul0i.c" "values.c" "objects.c" "misc.c" "rul0i.h")

# TODO: Add tests and install targets if needed.
