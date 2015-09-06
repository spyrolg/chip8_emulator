#include "Chip8.h"
#include <fstream>
#include <vector>
#include <Windows.h>	/* Sleep */
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */

using namespace std;

Chip8::Chip8()
{
	srand((unsigned int)time(NULL));

	// Load fontset
	for (int i = 0; i < PIXEL_FONT_SET_4x5_LENGTH; ++i)
	{
		memory[FONTSET_ADDRESS + i] = chip8_fontset[i];
	}
}

Chip8::~Chip8()
{
}

bool Chip8::LoadGame(const char* path)
{
	ifstream stream(path, ios::in | ios::binary | ios::ate);

	char* gameData = NULL;
	try
	{
		if (stream.is_open())
		{
			streampos size = stream.tellg();
			gameData = new char[(unsigned int)size];
			stream.seekg(0, ios::beg);
			stream.read(gameData, size);
			stream.close();

			// Copy game data into Program/Ram memory
			for (int i = 0; i < size; i++)
			{
				memory[RAM_MEMORY_ADDRESS + i] = gameData[i];
			}

			delete[] gameData;
		}
		else
		{
			return false;
		}
	}
	catch (std::exception&)
	{
		if (gameData != NULL)
		{
			delete[] gameData;
		}

		return false;
	}

	Reinitialize();

	return true;
}

void Chip8::Reinitialize()
{
	for (int i = 0; i < 16; i++)
	{
		V[i] = 0;
		stack[0] = 0;
	}
	I = 0;
	sp = 0;
	pc = RAM_MEMORY_ADDRESS;
	delay_timer = 60;
	sound_timer = 60;
	for (int i = 0; i < 4096; i++)
	{
		memory[4096] = { 0 };
	}

	for (int i = 0; i < SCREEN_SIZE; i++)
	{
		gfx[i] = 0;
	}
}

/* CPU */
void Chip8::Decode()
{
	// Fetch opcode
	unsigned short opcode = memory[pc] << 8 | memory[pc + 1];

	// Decode opcode
	switch (opcode & 0xF000)
	{
	case 0x0000:
		DecodeOpcode0000(opcode);
		break;
	case 0x1000: // 1NNN: Jumps to address NNN.
		pc = opcode & 0x0FFF;
		return; // pc already updated
	case 0x2000: // 2NNN: Calls subroutine at NNN.
		// TODO: do I not need to trace the registers ?
		stack[sp++] = pc;
		pc = opcode & 0X0FFF;
		return; // pc already updated
	case 0x3000: // 3XNN	Skips the next instruction if VX equals NN.
		if (V[memory[pc] & 0x0F] == memory[pc + 1])
		{
			pc += 2;
		}
		break;
	case 0x4000: // 4XNN	Skips the next instruction if VX doesn't equal NN.
		if (V[memory[pc] & 0x0F] != memory[pc + 1])
		{
			pc += 2;
		}
		break;
	case 0x5000: // 5XY0	Skips the next instruction if VX equals VY.
		if (V[memory[pc] & 0x0F] == V[(memory[pc + 1] >> 4)])
		{
			pc += 2;
		}
		break;
	case 0x6000: // 6XNN: sets VX to NN
		V[memory[pc] & 0X0F] = memory[pc + 1];
		break;
	case 0x7000: // 7XNN	Adds NN to VX.
		V[memory[pc] & 0x0F] += memory[pc + 1];
		break;
	case 0x8000:
		DecodeOpcode8000(opcode);
		break;
	case 0x9000: // 9XY0	Skips the next instruction if VX doesn't equal VY.
		if (V[memory[pc] & 0x0F] != V[memory[pc + 1] >> 4])
		{
			pc += 2;
		}
		break;
	case 0xA000: // ANNN: sets I to the address NNN
		I = opcode & 0x0FFF;
		break;
	case 0xB000: // BNNN	Jumps to the address NNN plus V0.
		pc = (opcode & 0x0FFF) + V[0];
		return; // pc already upate
	case 0xC000: // CXNN	Sets VX to the result of a bitwise and operation on a random number and NN.
		V[memory[pc] & 0x0F] = (rand() % 0xFF) & memory[pc + 1];
		break;
	case 0xD000: // DXYN (gfx handling)
		DecodeOpcodeD000();
		break;
	case 0xE000:
		switch (opcode & 0x00FF)
		{
		case 0x9E:
			// EX9E	Skips the next instruction if the key stored in VX is pressed.
			if (keys[V[(opcode & 0x0F00) >> 8]] == 1)
			{
				pc += 2;
			}
			break;
		case 0xA1:
			// EXA1	Skips the next instruction if the key stored in VX isn't pressed.
			if (keys[V[(opcode & 0x0F00) >> 8]] == 0)
			{
				pc += 2;
			}
			break;
		}
		break;
	case 0xF000:
		DecodeOpcodeF000(opcode);
		break;
	default:
		throw std::invalid_argument("illegal opcode " + opcode);
	}

	pc += 2;
}

void Chip8::DecodeOpcode0000(unsigned short opcode)
{
	if (opcode == 0x00E0) // clears the screen
	{
		for (unsigned int i = 0; i < SCREEN_SIZE; i++)
		{
			gfx[i] = 0;
		}
	}
	else if (opcode == 0x00EE) // returns from subroutine
	{
		pc = stack[--sp];
		stack[sp] = 0;
	}
	else
	{
		// opcode should be 0NNN: Calls RCA 1802 program at address NNN. Not necessary for most ROMs.
		// will not be supported for now
		throw std::invalid_argument("RCA 1802 program is unsupported by this emulator");
	}
}

void Chip8::DecodeOpcode8000(unsigned short opcode)
{
	// 8XYN

	unsigned char subOpCode = opcode & 0x000F;
	unsigned char x = (opcode >> 8) & 0x0F;
	unsigned char y = (opcode >> 4) & 0X0F;

	switch (subOpCode)
	{
	case (0x0) : // 8XY0	Sets VX to the value of VY.
		V[x] = V[y];
		break;
	case (0x1) : // 8XY1	Sets VX to VX or VY.
		V[x] |= V[y];
		break;
	case (0x2) : // 8XY2	Sets VX to VX and VY.
		V[x] &= V[y];
		break;
	case (0x3) : // 8XY3	Sets VX to VX xor VY.
		V[x] ^= V[y];
		break;
	case (0x4) : // 8XY4	Adds VY to VX. VF is set to 1 when there's a carry, and to 0 when there isn't.
		V[F] = (V[y] > (0xFF - V[x])) ? 1 : 0;
		V[x] += V[y];
		break;
	case (0x5) : // 8XY5	VY is subtracted from VX. VF is set to 0 when there's a borrow, and 1 when there isn't.
		V[F] = (V[y] > V[x]) ? 0 : 1;
		V[x] -= V[y];
		break;
	case (0x6) : // 8XY6	Shifts VX right by one. VF is set to the value of the least significant bit of VX before the shift.[2]
		V[F] = V[x] & 0x01;
		V[x] >>= 1;
		break;
	case (0x7) : // 8XY7	Sets VX to VY minus VX. VF is set to 0 when there's a borrow, and 1 when there isn't.
		V[F] = (V[x] > V[y]) ? 0 : 1;
		V[x] = V[y] - V[x];
		break;
	case (0xE) : // 8XYE	Shifts VX left by one. VF is set to the value of the most significant bit of VX before the shift.[2]
		V[F] = V[x] >> 7;
		V[x] <<= 1;
		break;
	default:
		throw std::invalid_argument("illegal opcode " + opcode);
	}
}

void Chip8::DecodeOpcodeD000()
{
	// DXYN
	// Sprites stored in memory at location in index register (I), 8bits wide. 
	// Wraps around the screen. If when drawn, clears a pixel, register VF is set to 1 otherwise it is zero. 
	// All drawing is XOR drawing (i.e. it toggles the screen pixels). Sprites are drawn starting at position VX, VY. 
	// N is the number of 8bit rows that need to be drawn. If N is greater than 1, second line continues at position VX, VY+1, and so on.

	unsigned short x = V[memory[pc] & 0x0F];
	unsigned short y = V[(memory[pc + 1] & 0xF0) >> 4];
	unsigned short height = memory[pc + 1] & 0x0F;
	unsigned short pixel;

	V[F] = 0;
	for (int yline = 0; yline < height; yline++)
	{
		pixel = memory[I + yline];
		for (int xline = 0; xline < 8; xline++)
		{
			if ((pixel & (0x80 >> xline)) != 0)
			{
				if (gfx[(x + xline + ((y + yline) * 64))] == 1)
				{
					V[F] = 1;
				}
				gfx[x + xline + ((y + yline) * 64)] ^= 1;
			}
		}
	}
}

void Chip8::DecodeOpcodeF000(unsigned short opcode)
{
	unsigned char subOpCode = opcode & 0xFF;
	unsigned char x = (opcode >> 8) & 0x0F;

	switch (subOpCode)
	{
	case (0x07) : // FX07	Sets VX to the value of the delay timer.
		V[x] = delay_timer;
		break;
	case (0x0A) : // FX0A	A key press is awaited, and then stored in VX.
		for (int i = 0; i < 16; i++)
		{
			if (keys[i] == 1)
			{
				V[x] = i;
				return;
			}
		}
		pc -= 2; // compensate for the outer loop that increments the program counter cause we need to repeat the opcode (no key was pressed)
		break;
	case (0x15) : // FX15	Sets the delay timer to VX.
		delay_timer = V[x];
		break;
	case (0x18) : // FX18	Sets the sound timer to VX.
		sound_timer = V[x];
		break;
	case (0x1E) : // FX1E	Adds VX to I.  VF is set to 1 when range overflow (I+VX>0xFFF), and 0 when there isn't. This is undocumented feature of the CHIP-8 and used by Spacefight 2091! game.
		V[F] = (I + V[x] > 0xFFF) ? 1 : 0;
		I += V[x];
		break;
	case (0x29) : // FX29	Sets I to the location of the sprite for the character in VX. Characters 0 - F(in hexadecimal) are represented by a 4x5 font.
		I = V[x] * 0x05;
		break;
	case (0x33) : // FX33	
		// Stores the Binary - coded decimal representation of VX, 
		// with the most significant of three digits at the address in I, 
		// the middle digit at I plus 1, 
		// and the least significant digit at I plus 2. 
		// (In other words, take the decimal representation of VX, place the hundreds digit in memory at location in I, the tens digit at location I + 1, and the ones digit at location I + 2.)
		memory[I] = V[x] / 100;
		memory[I + 1] = (V[x] / 10) % 10;
		memory[I + 2] = (V[x] % 100) % 10;
		break;
	case (0x55) : // FX55	Stores V0 to VX in memory starting at address I
		for (unsigned char i = 0; i <= x; i++)
			memory[I + i] = V[i];

		// On the original interpreter, when the operation is done, I = I + X + 1.
		I += x + 1;
		break;
	case (0x65) : // FX65	Fills V0 to VX with values from memory starting at address I.
		for (unsigned char i = 0; i <= x; i++)
			V[i] = memory[I + i];

		I += x + 1;
		break;
	}
}

/* Public */

void Chip8::EmulateCycle()
{
	Decode();

	// Update timers TODO
	if (delay_timer > 0)
		--delay_timer;

	if (sound_timer > 0)
	{
		if (sound_timer == 1)
			printf("BEEP!\n");
		--sound_timer;
	}

	Sleep(2);
}

bool Chip8::DrawGraphics(unsigned char* pixels)
{
	for (int i = 0; i < SCREEN_SIZE; i++)
	{
		if (gfx[i] == 0)
		{
			pixels[(3 * i) + 0] = 0;
			pixels[(3 * i) + 1] = 0;
			pixels[(3 * i) + 2] = 0;
		}
		else if (gfx[i] == 1)
		{
			pixels[(3 * i) + 0] = 255;
			pixels[(3 * i) + 1] = 255;
			pixels[(3 * i) + 2] = 255;
		}
		else
		{
			throw std::invalid_argument("monochrome gfx");
		}
	}

	return true;
}

void Chip8::SetKey(unsigned char key, unsigned char state)
{
	keys[key] = state;
}
