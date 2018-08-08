add_executable(pot8o-chip
	main.h
	main.cpp
    chip8.h
	chip8.cpp
	interpreter.h
	interpreter.cpp
	dynarec/dynarec.h
	dynarec/dynarec.cpp
	dynarec/enums.h
	frontend.h
	frontend.cpp
)

create_target_directory_groups(pot8o-chip)

target_link_libraries(pot8o-chip PRIVATE SDL2)

###########################################
# Copyright 2016 Citra Emulator Project
# Licensed under GPLv2 or any later version

# Include guard
if(__windows_copy_files)
	return()
endif()
set(__windows_copy_files YES)

# Any number of files to copy from SOURCE_DIR to DEST_DIR can be specified after DEST_DIR.
# This copying happens post-build.
function(windows_copy_files TARGET SOURCE_DIR DEST_DIR)
    # windows commandline expects the / to be \ so switch them
    string(REPLACE "/" "\\\\" SOURCE_DIR ${SOURCE_DIR})
    string(REPLACE "/" "\\\\" DEST_DIR ${DEST_DIR})

    # /NJH /NJS /NDL /NFL /NC /NS /NP - Silence any output
    # cmake adds an extra check for command success which doesn't work too well with robocopy
    # so trick it into thinking the command was successful with the || cmd /c "exit /b 0"
    add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND if not exist ${DEST_DIR} mkdir ${DEST_DIR} 2> nul
        COMMAND robocopy ${SOURCE_DIR} ${DEST_DIR} ${ARGN} /NJH /NJS /NDL /NFL /NC /NS /NP || cmd /c "exit /b 0"
    )
endfunction()
###########################################

function(copy_SDL_deps target_dir)
    set(DLL_DEST "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/")
    windows_copy_files(${target_dir} ${SDL2_DLL_DIR} ${DLL_DEST} SDL2.dll)
endfunction(copy_SDL_deps)

if (MSVC)
    copy_SDL_deps(pot8o-chip)
endif()
