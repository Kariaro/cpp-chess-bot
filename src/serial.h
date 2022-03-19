#ifndef SERIAL_H
#define SERIAL_H

#include <string>
#include "utils_type.h"

namespace Serial {
	extern inline int get_piece_value(int piece);
	extern inline char get_piece_character(int piece);
	extern inline int get_piece_from_character(char c);

	extern char* get_square_string(int square);
	
	char* getFancyMoveString(int piece, uint32_t from, uint32_t to, uint32_t special);

	extern std::string get_board_string(Chessboard& board);
	extern std::string get_move_string(Move& move);
}

#endif // SERIAL_H
