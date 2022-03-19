#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sstream>

#include "pieces.h"
#include "serial.h"

// TODO: Remove
static void appendChars(char* ptr, const char* val) {
	for (int i = 0; ; i++) {
		char c = *(val + i);
		if (c == '\0') {
			break;
		}

		*(ptr + i) = c;
	}
}

const int VALUES[13] = { -100, -500, -300, -300, -900, 0, 0, 0, 900, 300, 300, 500, 100 };

namespace Serial {
	inline int get_piece_value(int i) {
		return (i > -7 && i < 7) ? (VALUES[i + 6]) : (0);
	}

	inline char get_piece_character(int i) {
		return (i > -7 && i < 7) ? ("prnbqk\0KQBNRP"[i + 6]) : ('\0');
	}

	inline int get_piece_from_character(char c) {
		switch (c) {
			case 'r': return Pieces::B_ROOK;
			case 'n': return Pieces::B_KNIGHT;
			case 'b': return Pieces::B_BISHOP;
			case 'q': return Pieces::B_QUEEN;
			case 'k': return Pieces::B_KING;
			case 'p': return Pieces::B_PAWN;
			case 'R': return Pieces::W_ROOK;
			case 'N': return Pieces::W_KNIGHT;
			case 'B': return Pieces::W_BISHOP;
			case 'Q': return Pieces::W_QUEEN;
			case 'K': return Pieces::W_KING;
			case 'P': return Pieces::W_PAWN;
			default: return Pieces::NONE;
		}
	}

	char* get_square_string(int square) {
		char* result = (char*)calloc(3, sizeof(char));
		if (!result) return 0;

		*(result    ) = 'h' - (square & 7);
		*(result + 1) = '1' + ((square >> 3) & 7);
		*(result + 2) = '\0';
		return result;
	}
	
	// TODO: Remove
	char* getFancyMoveString(int piece, uint32_t from, uint32_t to, uint32_t special) {
		char* buffer = (char*)calloc(32, sizeof(char));
		if (!buffer) {
			return 0;
		}
		char* ptr = buffer;

		uint32_t type = special & 0b11000000;

		switch (type) {
			case NORMAL: {
				char pieceChar = get_piece_character(piece);
				if (pieceChar != '\0') {
					*ptr = pieceChar;
					ptr++;
				}
				
				*(ptr    ) = 'h' - (from & 7);
				*(ptr + 1) = '1' + ((from >> 3) & 7);
				*(ptr + 2) = 'h' - (to & 7);
				*(ptr + 3) = '1' + ((to >> 3) & 7);
				*(ptr + 4) = '\0';
				break;
			}
			case CASTLING: {
				appendChars(ptr, (special & ANY_CASTLE_K) ? "O-O\0" : "O-O-O\0");
				break;
			}
			case EN_PASSANT: {
				*(ptr    ) = 'h' - (from & 7);
				*(ptr + 1) = '1' + ((from >> 3) & 7);
				*(ptr + 2) = 'h' - (to & 7);
				*(ptr + 3) = '1' + ((to >> 3) & 7);
				appendChars(ptr + 4, " (en passant)\0");
				break;
			}
			case PROMOTION: {
				*(ptr + 0) = 'h' - (to & 7);
				*(ptr + 1) = '1' + ((to >> 3) & 7);
				*(ptr + 2) = '=';
				*(ptr + 3) = get_piece_character((special >> 3) & 0b111);
				*(ptr + 4) = '\0';
				break;
			}
		}

		return buffer;
	}

	/*
	char* getMoveString(uint32_t from, uint32_t to, uint32_t special) {
		char* buffer = (char*)calloc(8, sizeof(char));
		if (!buffer) {
			return 0;
		}

		*(buffer    ) = 'a' + (from & 7);
		*(buffer + 1) = '1' + ((from >> 3) & 7);
		*(buffer + 2) = 'a' + (to & 7);
		*(buffer + 3) = '1' + ((to >> 3) & 7);

		if ((special & 0b11000000) == SM::PROMOTION) {
			*(buffer + 4) = get_piece_character(-(int)((special >> 3) & 0b111));
		}

		return buffer;
	}
	*/

	std::string get_move_string(Move& move) {
		// null moves
		uint8_t move_from = get_move_from(move);
		uint8_t move_to = get_move_to(move);
		uint8_t move_special = get_move_special(move);

		if ((move_from == move_to) && move_from == 0) {
			return "0000";
		}

		std::stringstream ss;
		ss << (char)('a' + (move_from & 7))
		   << (char)('1' + ((move_from >> 3) & 7))
		   << (char)('a' + (move_to & 7))
		   << (char)('1' + ((move_to >> 3) & 7));

		if ((move_special & 0b11000000) == SM::PROMOTION) {
			ss << (char)get_piece_character(-(int)((move_special >> 3) & 0b111));
		}

		return ss.str();
	}

	std::string get_board_string(Chessboard& board) {
		std::stringstream ss;

		for (int i = 0; i < 64; i++) {
			int x = 7 - (i & 7);
			int y = i >> 3;

			int idx = x + (y << 3);
			char c = get_piece_character(board.pieces[idx]);

			if (i > 0) {
				ss << ((i & 7) == 0) ? '\n' : ' ';
			}

			ss << (c == '\0') ? '.' : c;
		}

		return ss.str();
	}
}
