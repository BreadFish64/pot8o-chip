add_executable(pot8o-chip
	main.cpp
    chip8.hpp
	frontend.hpp
	frontend.cpp
	llvm_aot.hpp
	llvm_aot.cpp
	aot_ops.hpp
	interpreter.hpp
	interpreter.cpp
	font.hpp
)

find_package(SDL2 CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(clang CONFIG REQUIRED)
find_package(llvm CONFIG REQUIRED)
target_link_libraries(pot8o-chip PRIVATE SDL2::SDL2 fmt::fmt libclang clangCodeGen LLVMCore LLVMCodeGen LLVMX86AsmParser LLVMX86CodeGen LLVMExecutionEngine LLVMMCJIT)
