#ifndef UTILS_TYPE_H
#define UTILS_TYPE_H

#include <cinttypes>

struct Chessboard {
	int8_t pieces[64];
	uint64_t pieceMask;
	uint64_t whiteMask;
	uint64_t blackMask;
	int lastCapture;
	int lastPawn;
	int halfMove;
	int flags;
};

struct Move {
	uint32_t from;
	uint32_t to;
	uint32_t special;
	bool valid;
};

#define _ForceInline __forceinline
#define _Inline inline

constexpr bool WHITE = true;
constexpr bool BLACK = false;

#endif // UTILS_TYPE_H
