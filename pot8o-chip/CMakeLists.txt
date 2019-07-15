add_executable(pot8o-chip
	main.h
	main.cpp
    chip8.h
	chip8.cpp
	frontend.h
	frontend.cpp
)

find_package(SDL2 CONFIG REQUIRED)
target_link_libraries(pot8o-chip PRIVATE SDL2::SDL2)

