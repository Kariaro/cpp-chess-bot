#ifndef PIECE_MANAGER_H
#define PIECE_MANAGER_H

#include "utils_type.h"
#include "utils.h"
#include "pieces.h"
#include "precomputed.h"
#include "chessboard.h"

namespace PieceManager {
	extern uint64_t piece_move(Chessboard& board, int piece, uint32_t idx);

	extern uint32_t special_piece_move(Chessboard& board, int piece, uint32_t idx);

	extern bool isAttacked(Chessboard& board, uint32_t idx);

	extern bool isKingAttacked(Chessboard& board, bool isWhite);
}

namespace PieceManager {
	template<bool White>
	_Inline uint64_t _Pawn_move(Chessboard& board, uint32_t idx) {
		uint64_t pawn = 1ull << idx;
		uint64_t result = 0;

		int ypos = idx >> 3;
		int xpos = idx & 7;

		if constexpr (White) {
			uint64_t step = pawn << 8;

			if (ypos < 6) {
				result |= step & ~board.pieceMask;
				if (result != 0 && ypos == 1) { // Pawn jump
					result |= (step << 8) & ~board.pieceMask;
				}

				if (xpos > 0) { // Takes
					result |= board.blackMask & (step >> 1);
				}

				if (xpos < 7) {
					result |= board.blackMask & (step << 1);
				}
			}
		} else {
			uint64_t step = pawn >> 8;

			if (ypos > 1) {
				result |= step & ~board.pieceMask;
				if (result != 0 && ypos == 6) { // Pawn jump
					result |= (step >> 8) & ~board.pieceMask;
				}

				if (xpos > 0) { // Takes
					result |= board.whiteMask & (step >> 1);
				}

				if (xpos < 7) {
					result |= board.whiteMask & (step << 1);
				}
			}
		}

		return result;
	}

	_Inline uint64_t _Bishop_move(uint64_t board_pieceMask, uint32_t idx) {
		uint64_t moveMask = PrecomputedTable::BISHOP_MOVES[idx];
		uint64_t checkMask = board_pieceMask & moveMask;

		const uint64_t* SHADOW = PrecomputedTable::BISHOP_SHADOW_MOVES[idx];
		while (checkMask != 0) {
			uint64_t pick = Utils::lowestOneBit(checkMask);
			checkMask &= ~pick;
			uint64_t shadowMask = SHADOW[Utils::numberOfTrailingZeros(pick)];
			moveMask &= shadowMask;
			checkMask &= shadowMask;
		}

		return moveMask;
	}

	_Inline uint64_t _Rook_move(uint64_t board_pieceMask, uint32_t idx) {
		uint64_t moveMask = PrecomputedTable::ROOK_MOVES[idx];
		uint64_t checkMask = board_pieceMask & moveMask;

		const uint64_t* SHADOW = PrecomputedTable::ROOK_SHADOW_MOVES[idx];
		while (checkMask != 0) {
			uint64_t pick = Utils::lowestOneBit(checkMask);
			checkMask &= ~pick;
			uint64_t shadowMask = SHADOW[Utils::numberOfTrailingZeros(pick)];
			moveMask &= shadowMask;
			checkMask &= shadowMask;
		}

		return moveMask;
	}

	_Inline uint64_t _Queen_move(uint64_t board_pieceMask, uint32_t idx) {
		uint64_t bishop_moveMask = PrecomputedTable::BISHOP_MOVES[idx];
		uint64_t bishop_checkMask = board_pieceMask & bishop_moveMask;

		uint64_t rook_moveMask = PrecomputedTable::ROOK_MOVES[idx];
		uint64_t rook_checkMask = board_pieceMask & rook_moveMask;

		const uint64_t* BISHOP_SHADOW = PrecomputedTable::BISHOP_SHADOW_MOVES[idx];
		const uint64_t* ROOK_SHADOW = PrecomputedTable::ROOK_SHADOW_MOVES[idx];

		// A rook and a bishop on the same square will never have the same positions
		// This is only true when both of them are zero.
		while (bishop_checkMask != rook_checkMask) {
			uint64_t pick;
			uint64_t shadowMask;

			if (bishop_checkMask != 0) {
				pick = Utils::lowestOneBit(bishop_checkMask);
				bishop_checkMask &= ~pick;
				shadowMask = BISHOP_SHADOW[Utils::numberOfTrailingZeros(pick)];
				bishop_moveMask &= shadowMask;
				bishop_checkMask &= shadowMask;
			}

			if (rook_checkMask != 0) {
				pick = Utils::lowestOneBit(rook_checkMask);
				rook_checkMask &= ~pick;
				shadowMask = ROOK_SHADOW[Utils::numberOfTrailingZeros(pick)];
				rook_moveMask &= shadowMask;
				rook_checkMask &= shadowMask;
			}
		}

		return bishop_moveMask | rook_moveMask;
	}

	_ForceInline uint64_t _King_move(uint32_t idx) {
		return PrecomputedTable::KING_MOVES[idx];
	}

	_ForceInline uint64_t _Knight_move(uint32_t idx) {
		return PrecomputedTable::KNIGHT_MOVES[idx];
	}

	_ForceInline uint64_t _White_pawn_attack(uint32_t idx) {
		return PrecomputedTable::PAWN_ATTACK_WHITE[idx];
	}

	_ForceInline uint64_t _Black_pawn_attack(uint32_t idx) {
		return PrecomputedTable::PAWN_ATTACK_BLACK[idx];
	}

	template <int PieceA, int PieceB>
	_Inline bool _Has_two_piece(Chessboard& board, uint64_t mask) {
		if constexpr (PieceA < 0) {
			mask &= board.blackMask;
		} else {
			mask &= board.whiteMask;
		}

		while (mask != 0) {
			uint64_t pick = Utils::lowestOneBit(mask);
			mask &= ~pick;
			uint32_t idx = Utils::numberOfTrailingZeros(pick);

			int piece = board.pieces[idx];
			if (piece == PieceA || piece == PieceB) {
				return true;
			}
		}

		return false;
	}

	template <int Piece>
	_Inline bool _Has_piece(Chessboard& board, uint64_t mask) {
		if constexpr (Piece < 0) {
			mask &= board.blackMask;
		} else {
			mask &= board.whiteMask;
		}

		while (mask != 0) {
			uint64_t pick = Utils::lowestOneBit(mask);
			mask &= ~pick;
			uint32_t idx = Utils::numberOfTrailingZeros(pick);

			if (board.pieces[idx] == Piece) {
				return true;
			}
		}

		return false;
	}

	template <int Piece>
	_Inline uint32_t _Get_first(Chessboard& board) {
		uint64_t mask;
		if constexpr (Piece < 0) {
			mask = board.blackMask;
		} else {
			mask = board.whiteMask;
		}

		while (mask != 0) {
			uint64_t pick = Utils::lowestOneBit(mask);
			mask &= ~pick;
			uint32_t idx = Utils::numberOfTrailingZeros(pick);

			if (board.pieces[idx] == Piece) {
				return idx;
			}
		}

		return -1;
	}

	template <bool White>
	_Inline bool _Is_attacked(Chessboard& board, uint32_t idx) {
		uint64_t pieceMask = board.pieceMask;

		if constexpr (White) {
			uint64_t _mask = (~board.whiteMask) & board.blackMask;
			uint64_t _rook_move = _Rook_move(pieceMask, idx) & _mask;
			if (_Has_two_piece<Pieces::B_ROOK, Pieces::B_QUEEN>(board, _rook_move)) {
				return true;
			}

			uint64_t _bishop_move = _Bishop_move(pieceMask, idx) & _mask;
			if (_Has_two_piece<Pieces::B_BISHOP, Pieces::B_QUEEN>(board, _bishop_move)) {
				return true;
			}

			uint64_t _knight_move = _Knight_move(idx) & _mask;
			if (_Has_piece<Pieces::B_KNIGHT>(board, _knight_move)) {
				return true;
			}

			uint64_t _king_move = _King_move(idx) & _mask;
			if (_Has_piece<Pieces::B_KING>(board, _king_move)) {
				return true;
			}

			uint64_t _pawn_move = _White_pawn_attack(idx) & _mask;
			return _Has_piece<Pieces::B_PAWN>(board, _pawn_move);
		} else {
			uint64_t _mask = (~board.blackMask) & board.whiteMask;
			uint64_t _rook_move = _Rook_move(pieceMask, idx) & _mask;
			if (_Has_two_piece<Pieces::W_ROOK, Pieces::W_QUEEN>(board, _rook_move)) {
				return true;
			}

			uint64_t _bishop_move = _Bishop_move(pieceMask, idx) & _mask;
			if (_Has_two_piece<Pieces::W_BISHOP, Pieces::W_QUEEN>(board, _bishop_move)) {
				return true;
			}

			uint64_t _knight_move = _Knight_move(idx) & _mask;
			if (_Has_piece<Pieces::W_KNIGHT>(board, _knight_move)) {
				return true;
			}

			uint64_t _king_move = _King_move(idx) & _mask;
			if (_Has_piece<Pieces::W_KING>(board, _king_move)) {
				return true;
			}

			uint64_t _pawn_move = _Black_pawn_attack(idx) & _mask;
			return _Has_piece<Pieces::W_PAWN>(board, _pawn_move);
		}
	}

	template <bool White>
	_Inline bool _Is_king_attacked(Chessboard& board) {
		uint32_t idx;

		// Find the king
		if constexpr (White) {
			idx = _Get_first<Pieces::W_KING>(board);
		} else {
			idx = _Get_first<Pieces::B_KING>(board);
		}

		return idx != -1 && _Is_attacked<White>(board, idx);
	}

	template <bool White>
	_ForceInline uint64_t _Get_attack_square(Chessboard& a_board) {
		uint32_t idx;
		uint64_t mask;

		// Find the king
		if constexpr (White) {
			idx = _Get_first<Pieces::W_KING>(a_board);
			mask = a_board.whiteMask;
		} else {
			idx = _Get_first<Pieces::B_KING>(a_board);
			mask = a_board.blackMask;
		}

		// Add the kings position
		//return (PrecomputedTable::KNIGHT_MOVES[idx] | PrecomputedTable::ROOK_MOVES[idx] | PrecomputedTable::BISHOP_MOVES[idx]) | (1ull << idx);
		return PrecomputedTable::KNIGHT_MOVES[idx] | _Queen_move(mask, idx) | (1ull << idx);
	}
}

#endif // PIECE_MANAGER_H
