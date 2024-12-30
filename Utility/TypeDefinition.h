#pragma once
#include <Unknwnbase.h>
#include <combaseapi.h>
#include <iostream>
#include <string>

typedef unsigned char byte;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;
typedef long long int64;
typedef uint32 bool32;
using Entity = uint32;

enum class MouseKey : int
{
	LEFT = 0,
	RIGHT = 1,
	MIDDLE = 2,
	MOUSE_KEY_MAX = 3
};

