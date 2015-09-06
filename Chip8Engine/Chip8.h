#pragma once

#ifdef CHIP8ENGINE_EXPORTS
#define CHIP8ENGINE_API __declspec(dllexport) 
#else
#define CHIP8ENGINE_API __declspec(dllimport) 
#endif

class Chip8
{
public:
	static __declspec(dllexport) const unsigned int SCREEN_WIDTH = 64;
	static __declspec(dllexport) const unsigned int SCREEN_HEIGHT = 32;
	static __declspec(dllexport) const unsigned int SCREEN_SIZE = SCREEN_WIDTH * SCREEN_HEIGHT;

private:
	// system memory map
	// 0x000 - 0x1FF - Chip 8 interpreter(contains font set in emu)
	// 0x050 - 0x0A0 - Used for the built in 4x5 pixel font set(0 - F)
	// 0x200 - 0xFFF - Program ROM and work RAM
	const static unsigned short FONTSET_ADDRESS = 0x000;
	const static unsigned short PIXEL_FONT_SET_4x5_LENGTH = 0x050;
	const static unsigned short RAM_MEMORY_ADDRESS = 0x200;

	// Font set (0-F, 4x5 size, first 4 MSB bits are used for drawing character from font set)
	unsigned char chip8_fontset[PIXEL_FONT_SET_4x5_LENGTH] =
	{
		0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
		0x20, 0x60, 0x20, 0x20, 0x70, // 1
		0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
		0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
		0x90, 0x90, 0xF0, 0x10, 0x10, // 4
		0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
		0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
		0xF0, 0x10, 0x20, 0x40, 0x40, // 7
		0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
		0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
		0xF0, 0x90, 0xF0, 0x90, 0x90, // A
		0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
		0xF0, 0x80, 0x80, 0x80, 0xF0, // C
		0xE0, 0x90, 0x90, 0x90, 0xE0, // D
		0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
		0xF0, 0x80, 0xF0, 0x80, 0x80  // F
	};

public:
	__declspec(dllexport) Chip8();
	__declspec(dllexport) ~Chip8();
	__declspec(dllexport) bool LoadGame(const char* path);
	__declspec(dllexport) void EmulateCycle();
	__declspec(dllexport) bool DrawGraphics(unsigned char* pixels);
	__declspec(dllexport) void SetKey(unsigned char key, unsigned char state);
private:
	// System memory
	unsigned char memory[4096] = { 0 };		// 4k of memory

	// Timers
	unsigned char delay_timer = 60;			// 60 hz timer
	unsigned char sound_timer = 60;			// 60 hz timer

	// Screen buffer
	unsigned char gfx[SCREEN_SIZE] = { 0 }; // monochrome 64x32 pixel screen (0 is off, 1 is on)

	// Keypad (16 keys: 0-F)
	// 1 2 3 C
	// 4 5 6 D
	// 7 8 9 E
	// A 0 B F
	unsigned char keys[16] = { 0 };

	// CPU
	
	unsigned char V[16] = { 0 };			// 15 generic purpose registers and the last one is the carry flag
	unsigned char F = 15;					// carry flag index on V
	unsigned short I = 0;					// index register 2 bytes
	unsigned short pc = RAM_MEMORY_ADDRESS;	// two byte program counter
	unsigned short stack[16] = { 0 };		// program stack (max depth subroutines is 16)
	unsigned short sp = 0;					// stack pointer

	void Decode();
	void DecodeOpcode0000(unsigned short opcode);
	void DecodeOpcode8000(unsigned short opcode);
	void DecodeOpcodeD000();
	void DecodeOpcodeF000(unsigned short opcode);

	void Reinitialize();
};

