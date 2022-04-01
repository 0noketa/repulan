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
	# C4477: printf/fprintf type check
	# C4774: printf/fprintf without string literal
	# C4047: maybe incompatible type
	# C4133: incompatible type
	# C4020: incompatible type
	# C4029: incompatible func decl
	add_compile_options(
		"/Wall"
		"/wd4255" "/wd4710" "/wd4711" "/wd4820" "/wd4996" "/wd5045"
		"/we4090" "/we4028" "/we4477" "/we4774" "/we4047" "/we4133" "/we4020" "/we4029")
else()
	add_compile_options("-Wall")
endif()

# Add source to this project's executable.
add_executable (rplni "rplni.c" "values.c" "objects.c" "misc.c" "loader.c" "tmpstr.c")

# TODO: Add tests and install targets if needed.
