#ifndef CHESSBOARD_H
#define CHESSBOARD_H

#include "utils_type.h"

namespace Board {
	_ForceInline bool isWhite(Chessboard& board) {
		return (board.halfMove & 1) == 0;
	}

	_ForceInline bool hasFlags(Chessboard& board, int flags) {
		return (board.flags & flags) != 0;
	}

	template <int Piece>
	_ForceInline void setPiece(Chessboard& board, uint32_t idx) {
		int old = board.pieces[idx];
		board.pieces[idx] = Piece;

		uint64_t mask = (uint64_t)(1ull) << idx;
		if (old < 0) {
			if constexpr (Piece >= 0) board.blackMask &= ~mask;
		}
		else {
			// if old is zero this still work
			if constexpr (Piece <= 0) board.whiteMask &= ~mask;
		}

		if constexpr (Piece > 0) board.whiteMask |= mask;
		if constexpr (Piece < 0) board.blackMask |= mask;

		board.pieceMask = board.blackMask | board.whiteMask;
	}

	_ForceInline void setPiece(Chessboard& board, uint32_t idx, int piece) {
		int old = board.pieces[idx];
		board.pieces[idx] = piece;

		uint64_t mask = (uint64_t)(1ull) << idx;
		if (old < 0) {
			if (piece >= 0) board.blackMask &= ~mask;
		} else {
			// if old is zero this still work
			if (piece <= 0) board.whiteMask &= ~mask;
		}

		if (piece > 0) board.whiteMask |= mask;
		if (piece < 0) board.blackMask |= mask;

		board.pieceMask = board.blackMask | board.whiteMask;

		/*
		int old = board.pieces[idx];
		board.pieces[idx] = piece;
		
		uint64_t mask = (uint64_t)(1ull) << idx;
		if (old < 0 && piece >= 0) board.blackMask &= ~mask;
		if (old > 0 && piece <= 0) board.whiteMask &= ~mask;
		if (piece > 0) board.whiteMask |= mask;
		if (piece < 0) board.blackMask |= mask;

		board.pieceMask = board.blackMask | board.whiteMask;
		*/
	}
}

#endif // CHESSBOARD_H
