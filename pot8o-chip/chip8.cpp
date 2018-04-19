#include "chip8.h"
#include <fstream>
#include <istream>
#include <vector>

void Chip8::initialize()
{
	// Initialize registers and memory once
	pc = 0x200;  // Program counter starts at 0x200
	opcode = 0;      // Reset current opcode	
	I = 0;      // Reset index register
	sp = 0;      // Reset stack pointer

				 // Clear display	
				 // Clear stack
				 // Clear registers V0-VF
				 // Clear memory

				 // Load fontset
	for (int i = 0; i < 80; ++i)
		memory[i] = font[i];

	// Reset timers
}

void Chip8::emulateCycle()
{
	// Fetch Opcode
	opcode = memory[pc] << 8 | memory[pc + 1];

	// Decode Opcode
	// Execute Opcode
	switch (opcode & 0xF000) {
	case 0x0000:
		switch (opcode & 0x000F)
		{
		case 0x0000: // 0x00E0: Clears the screen        
					 // Execute opcode
			break;

		case 0x000E: // 0x00EE: Returns from subroutine          
					 // Execute opcode
			break;

		default:
			printf("Unknown opcode [0x0000]: 0x%X\n", opcode);
		}
	case 0xA2F0:
		I = opcode & 0x0FFF;
		break;
	default:
		printf("Unknown opcode: 0x%X\n", opcode);
	}

	pc += 2;
	// Update timers
	if (delay_timer > 0)
		--delay_timer;

	if (sound_timer > 0)
	{
		if (sound_timer == 1)
			printf("BEEP!\n");
		--sound_timer;
	}
}

void Chip8::loadGame(std::string path) {
	std::ifstream file(path, std::ios::binary);
	file.seekg(0, file.end);
	int length = file.tellg();
	file.seekg(0, file.beg);

	char* buffer = new char[length];
	file.read(buffer, length);

	for (int i = 0; i < length; ++i)
		memory[i + 512] = buffer[i];
}
