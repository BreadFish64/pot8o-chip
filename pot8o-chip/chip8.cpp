#include "chip8.h"
#include <fstream>
#include <istream>
#include <vector>

void Chip8::initialize() {
  // Initialize registers and memory once
  pc = 0x200; // Program counter starts at 0x200
  opcode = 0; // Reset current opcode
  I = 0;      // Reset index register
  sp = 0;     // Reset stack pointer
  vf = 0;

  for (auto &row : gfx) {
    for (bool &column : row) {
      column = false;
    }
  } // Clear display
  for (auto &addr : stack) {
    addr = 0;
  } // Clear stack
  for (auto &reg : V) {
    reg = 0;
  } // Clear registers V0-VF
  for (auto &byte : memory) {
    byte = 0;
  } // Clear memory

  // Load fontset
  for (int i = 0; i < 0x50; ++i)
    memory[i] = font[i];

  // Reset timers
}

void Chip8::emulateCycle() {
  // Fetch Opcode
  opcode = memory[pc] << 8 | memory[pc + 1];

  // Decode Opcode
  // Execute Opcode
  switch (opcode & 0xF000) {
  case 0x0000:
    switch (opcode & 0x00FF) {
    case 0x00E0:
      for (auto &row : gfx) {
        for (bool &column : row) {
          column = false;
        }
      }
      break;
      // case 0x000E: // 0x00EE: Returns from subroutine
      // Execute opcode
      // break;
    default:
      printf("Unknown opcode [0x0000]: 0x%X\n", opcode);
    }
    break;
  case 0x1000:
    pc = (opcode & 0x0FFF) - 2;
    break;
  case 0x6000:
    V[(opcode & 0x0F00) >> 0xF] = opcode & 0x00FF;
    break;
  case 0x8000:
    switch (opcode & 0x000F) {
    case 0x0000:
      V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4];
      break;
    default:
      printf("Unknown opcode [0x0000]: 0x%X\n", opcode);
    }
  case 0x9000:
    if (V[(opcode & 0x0F00 >> 8)] != V[(opcode & 0x00F0 >> 4)])
      pc += 2;
    break;
  case 0xA000:
    I = opcode & 0x0FFF;
    break;
  case 0xD000: {
	  int x = opcode & 0x0F00;
	  int height = opcode & 0x000F;
	  vf = false;
	  for (int y = opcode & 0x00F0; y < height + (opcode & 0x00F0); y++) {
		  int byte = memory[I + y];
		  for (int i = 0; i < 8; i++) {
			  bool pixel = gfx[y][x + i];
			  gfx[y][x + i] = ((byte & power(2, i)) >> i) ^ pixel;
			  if (gfx[y][x + i] != pixel) vf = true;
		  }
	  }

	  break; }
  case 0xF000:
    switch (opcode & 0x00FF) {
    case 0x0015:
      delay_timer = V[(opcode & 0x0F00) >> 8];
      break;
    case 0x0055:
      for (int i = 0; i < V.size(); i++) {
        memory[I + i] = V[i];
      }
      break;
    case 0x0065:
      for (int i = 0; i < V.size(); i++) {
        V[i] = memory[I + i];
      }
      break;
    default:
      printf("Unknown opcode [0x0000]: 0x%X\n", opcode);
    }
    break;
  default:
    printf("Unknown opcode: 0x%X\n", opcode);
  }
  pc += 2;
  // Update timers
  if (delay_timer > 0)
    --delay_timer;

  if (sound_timer > 0) {
    if (sound_timer == 1)
      printf("BEEP!\n");
    --sound_timer;
  }
  if (pc >= 4096) {
    pc = 0;
  }
}

void Chip8::loadGame(std::string path) {
  path = "E:/git/chip8_ROMs/TICTAC";
  std::ifstream file(path, std::ios::binary);
  file.seekg(0, file.end);
  int length = file.tellg();
  file.seekg(0, file.beg);

  char *buffer = new char[length];
  file.read(buffer, length);

  for (int i = 0; i < length; ++i)
    memory[i + 512] = buffer[i];
}

const std::array<unsigned char, 0x50> Chip8::font = {
	//0
	0b11110000,
	0b10010000,
	0b10010000,
	0b10010000,
	0b11110000,
	//1
	0b00100000,
	0b01100000,
	0b00100000,
	0b00100000,
	0b01110000,
	//2
	0b11110000,
	0b00010000,
	0b11110000,
	0b10000000,
	0b11110000,
	//3
	0b11110000,
	0b00010000,
	0b11110000,
	0b00010000,
	0b11110000,
	//4
	0b10010000,
	0b10010000,
	0b11110000,
	0b00010000,
	0b00010000,
	//5
	0b11110000,
	0b10000000,
	0b11110000,
	0b00010000,
	0b11110000,
	//6
	0b11110000,
	0b10000000,
	0b11110000,
	0b10010000,
	0b11110000,
	//7
	0b11110000,
	0b00010000,
	0b00100000,
	0b01000000,
	0b01000000,
	//8
	0b11110000,
	0b10010000,
	0b11110000,
	0b10010000,
	0b11110000,
	//9
	0b11110000,
	0b10010000,
	0b11110000,
	0b00010000,
	0b11110000,
	//A
	0b11110000,
	0b10010000,
	0b11110000,
	0b10010000,
	0b10010000,
	//B
	0b11100000,
	0b10010000,
	0b11101111,
	0b10010000,
	0b11100000,
	//C
	0b11110000,
	0b10000000,
	0b10000000,
	0b10000000,
	0b11110000,
	//D
	0b11100000,
	0b10010000,
	0b10010000,
	0b10010000,
	0b11100000,
	//E
	0b11110000,
	0b10000000,
	0b11110000,
	0b10000000,
	0b11110000,
	//F
	0b11110000,
	0b10000000,
	0b11110000,
	0b10000000,
	0b10000000,
};

int power(int base, int exponent) {
	int result = 1;
	for (int i = 0; i < exponent; i++) {
		result = result * base;
	}
	return result;
}
