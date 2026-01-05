cmake_minimum_required(VERSION 3.18...4.0)

project(Hyperoid VERSION 1.4.0)

set(CMAKE_C_STANDARD 99)

add_executable(sdlroids
	src/config.h
	src/getargs.c
	src/getargs.h
	src/graphics.h
	src/hyperoid.c
	src/hyperoid.h
	src/misc.h
	src/petopt.c
	src/petopt.h
	src/rand.c
	src/rand.h
	src/roidsupp.c
	src/roidsupp.h
	src/sdl.c
	src/sdlsound.c
	src/sdlsound.h)

target_compile_options(sdlroids PRIVATE -Wall -Wunused)
target_compile_definitions(sdlroids PRIVATE DATADIR="/usr/share/sdlroids") # TODO

find_package(SDL2 REQUIRED)
find_package(SDL2_mixer REQUIRED)
target_link_libraries(sdlroids PRIVATE SDL2::SDL2 SDL2_mixer::SDL2_mixer m)
