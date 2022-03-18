# CMakeList.txt : CMake project for rplni, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)


if(MSVC)
	# C4255: use f(void) styled decl of >=C99 or C++
	# C4996: do not use strcpy/strncpy
	# C5045: notification of Spectre mitigation
	# C4090: tried to modify const
	# C4028: incompatible decl
	add_compile_options("/Wall" "/wd4255" "/wd4996" "/wd5045" "/we4090" "/we4028")
else()
	add_compile_options("-Wall")
endif()

# Add source to this project's executable.
add_executable (rplni "rplni.c" "values.c" "objects.c" "misc.c" "loader.c" "tmpstr.c")
add_executable (test_loader "test_loader.c" "values.c" "objects.c" "misc.c" "loader.c" "tmpstr.c")

# TODO: Add tests and install targets if needed.
