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

// Chess move hash table
struct HashTable {

};

typedef uint32_t Move;

/*
struct Move {
	uint8_t from;    //  0 -  7 bit
	uint8_t to;      //  8 - 15 bit
	uint8_t special; // 16 - 23 bit
	bool valid;      // 24 - 31 bit
};
*/

#define _ForceInline __forceinline
#define _Inline inline

constexpr bool WHITE = true;
constexpr bool BLACK = false;

constexpr Move create_move(uint8_t from, uint8_t to, uint8_t special, bool valid) {
	return (Move)((valid << 24) | (special << 16) | (to << 8) | (from));
}

// Getters
constexpr uint8_t get_move_from(const Move m) {
	return (uint8_t)(m & 0xff);
}

constexpr uint8_t get_move_to(const Move m) {
	return (uint8_t)((m >> 8) & 0xff);
}

constexpr uint8_t get_move_special(const Move m) {
	return (uint8_t)((m >> 16) & 0xff);
}

constexpr uint8_t get_move_valid(const Move m) {
	return (uint8_t)((m >> 24) & 0xff);
}

// Setters
constexpr Move set_move_valid(const Move m, bool valid) {
	return (Move)((m & (0x00ffffff)) | (valid * 0x01000000));
}

#endif // UTILS_TYPE_H
