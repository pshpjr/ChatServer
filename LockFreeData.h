#pragma once

namespace
{
	const unsigned long long pointerMask = 0x000'7FFF'FFFF'FFFF;
	const unsigned long long indexInc = 0x000'8000'0000'0000;
	const unsigned long long indexMask = 0xFFFF'8000'0000'0000;
}