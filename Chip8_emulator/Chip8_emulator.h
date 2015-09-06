#pragma once

#include "resource.h"
#include "SDL.h"
#include "Chip8.h"
#include <mutex>

// Chip8 engine
Chip8 chip8;

// SDL
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Texture* texture = NULL;
SDL_Surface* windowSurface = NULL;

std::mutex textureMutex;