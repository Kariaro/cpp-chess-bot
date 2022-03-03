#include "piece_manager.h"

uint64_t black_pawn_move(Chessboard& board, uint32_t idx) {
	uint64_t pawn = (uint64_t)(1) << idx;
	uint64_t step = pawn >> 8;
	uint64_t result = 0;
	
	int ypos = idx >> 3;
	int xpos = idx & 7;
	
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
	
	return result;
}

uint64_t white_pawn_move(Chessboard& board, uint32_t idx) {
	uint64_t pawn = (uint64_t)(1) << idx;
	uint64_t step = pawn << 8;
	uint64_t result = 0;
	
	int ypos = idx >> 3;
	int xpos = idx & 7;
	
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
	
	return result;
}

uint64_t bishop_move(uint64_t board_pieceMask, uint32_t idx) {
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

uint64_t rook_move(uint64_t board_pieceMask, uint32_t idx) {
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

uint64_t queen_move(uint64_t board_pieceMask, uint32_t idx) {
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

inline uint64_t king_move(uint32_t idx) {
	return PrecomputedTable::KING_MOVES[idx];
}

inline uint64_t knight_move(uint32_t idx) {
	return PrecomputedTable::KNIGHT_MOVES[idx];
}

inline uint64_t white_pawn_attack(uint32_t idx) {
	return PrecomputedTable::PAWN_ATTACK_WHITE[idx];
}

inline uint64_t black_pawn_attack(uint32_t idx) {
	return PrecomputedTable::PAWN_ATTACK_BLACK[idx];
}

template <int A, int B>
bool _hasTwoPiece(Chessboard& board, uint64_t mask) {
	if constexpr (A < 0) {
		mask &= board.blackMask;
	} else {
		mask &= board.whiteMask;
	}
		
	while (mask != 0) {
		uint64_t pick = Utils::lowestOneBit(mask);
		mask &= ~pick;
		uint32_t idx = Utils::numberOfTrailingZeros(pick);
		
		int piece = board.pieces[idx];
		if (piece == A || piece == B) {
			return true;
		}
	}
		
	return false;
}

template <int A>
bool _hasPiece(Chessboard& board, uint64_t mask) {
	if constexpr (A < 0) {
		mask &= board.blackMask;
	} else {
		mask &= board.whiteMask;
	}
	
	while (mask != 0) {
		uint64_t pick = Utils::lowestOneBit(mask);
		mask &= ~pick;
		uint32_t idx = Utils::numberOfTrailingZeros(pick);
		
		if (board.pieces[idx] == A) {
			return true;
		}
	}
	
	return false;
}

template <int A>
uint32_t _getFirst(Chessboard& board) {
	uint64_t mask;
	if constexpr (A < 0) {
		mask = board.blackMask;
	} else {
		mask = board.whiteMask;
	}
	
	while (mask != 0) {
		uint64_t pick = Utils::lowestOneBit(mask);
		mask &= ~pick;
		uint32_t idx = Utils::numberOfTrailingZeros(pick);
		
		if (board.pieces[idx] == A) {
			return idx;
		}
	}
	
	return -1;
}

namespace PieceManager {
	uint64_t piece_move(Chessboard& board, int piece, uint32_t idx) {
		switch (piece) {
			case Pieces::W_KNIGHT: return knight_move(idx) & ~board.whiteMask;
			case Pieces::W_BISHOP: return bishop_move(board.pieceMask, idx) & ~board.whiteMask;
			case Pieces::W_ROOK:   return rook_move(board.pieceMask, idx) & ~board.whiteMask;
			case Pieces::W_QUEEN:  return queen_move(board.pieceMask, idx) & ~board.whiteMask;
			case Pieces::W_PAWN:   return white_pawn_move(board, idx);
			case Pieces::W_KING:   return king_move(idx) & ~board.whiteMask;

			case Pieces::B_KNIGHT: return knight_move(idx) & ~board.blackMask;
			case Pieces::B_BISHOP: return bishop_move(board.pieceMask, idx) & ~board.blackMask;
			case Pieces::B_ROOK:   return rook_move(board.pieceMask, idx) & ~board.blackMask;
			case Pieces::B_QUEEN:  return queen_move(board.pieceMask, idx) & ~board.blackMask;
			case Pieces::B_PAWN:   return black_pawn_move(board, idx);
			case Pieces::B_KING:   return king_move(idx) & ~board.blackMask;
			default: return 0;
		}
	}

	uint32_t white_pawn_special_move(Chessboard& board, uint32_t idx) {
		int ypos = idx >> 3;
		int xpos = idx & 7;
		
		// En passant
		if (ypos == 4 && board.lastPawn != 0) {
			int lyp = board.lastPawn >> 3;
			if (lyp == 5) {
				int lxp = board.lastPawn & 7;
				if (xpos - 1 == lxp) {
					return (idx + 7) | SM::EN_PASSANT;
				}
				
				if (xpos + 1 == lxp) {
					return (idx + 9) | SM::EN_PASSANT;
				}
			}
		}
		
		// Promotion
		if (ypos == 6) {
			int result = 0;
			
			if (xpos > 0) {
				uint64_t mask = (uint64_t)(1) << (idx + 7);
				if ((board.blackMask & mask) != 0) {
					result |= Promotion::LEFT;
				}
			}
			
			if (xpos < 7) {
				uint64_t mask = (uint64_t)(1) << (idx + 9);
				if ((board.blackMask & mask) != 0) {
					result |= Promotion::RIGHT;
				}
			}
			
			{
				uint64_t mask = (uint64_t)(1) << (idx + 8);
				if (((board.pieceMask) & mask) == 0) {
					result |= Promotion::MIDDLE;
				}
			}
			
			return result == 0 ? 0 : (result | SM::PROMOTION);
		}
		
		return 0;
	}

	uint32_t black_pawn_special_move(Chessboard& board, uint32_t idx) {
		int ypos = idx >> 3;
		int xpos = idx & 7;
		
		// En passant
		if (ypos == 3 && board.lastPawn != 0) {
			int lyp = board.lastPawn >> 3;
			if (lyp == 2) {
				int lxp = board.lastPawn & 7;
				if (xpos - 1 == lxp) {
					return (idx - 9) | SM::EN_PASSANT;
				}
				
				if (xpos + 1 == lxp) {
					return (idx - 7) | SM::EN_PASSANT;
				}
			}
		}
		
		// Promotion
		if (ypos == 1) {
			int result = 0;
			
			if (xpos > 0) {
				uint64_t mask = (uint64_t)(1) << (idx - 9);
				if ((board.whiteMask & mask) != 0) {
					result |= Promotion::LEFT;
				}
			}
			
			if (xpos < 7) {
				uint64_t mask = (uint64_t)(1) << (idx - 7);
				if ((board.whiteMask & mask) != 0) {
					result |= Promotion::RIGHT;
				}
			}
			
			{
				uint64_t mask = (uint64_t)(1) << (idx - 8);
				if (((board.pieceMask) & mask) == 0) {
					result |= Promotion::MIDDLE;
				}
			}
			
			return result == 0 ? 0 : (result | SM::PROMOTION);
		}
		
		return 0;
	}

	uint32_t white_king_special_move(Chessboard& board, uint32_t idx) {
		uint32_t result = 0;
		if ((board.pieceMask & MASK_WHITE_K) == 0 && Board::hasFlags(board, CastlingFlags::WHITE_CASTLE_K)) {
			result |= SM::CASTLING | CastlingFlags::WHITE_CASTLE_K;
		}
			
		if ((board.pieceMask & MASK_WHITE_Q) == 0 && Board::hasFlags(board, CastlingFlags::WHITE_CASTLE_Q)) {
			result |= SM::CASTLING | CastlingFlags::WHITE_CASTLE_Q;
		}
		
		return result;
	}

	uint32_t black_king_special_move(Chessboard& board, uint32_t idx) {
		uint32_t result = 0;
		if ((board.pieceMask & MASK_BLACK_K) == 0 && Board::hasFlags(board, CastlingFlags::BLACK_CASTLE_K)) {
			result |= SM::CASTLING | CastlingFlags::BLACK_CASTLE_K;
		}
			
		if ((board.pieceMask & MASK_BLACK_Q) == 0 && Board::hasFlags(board, CastlingFlags::BLACK_CASTLE_Q)) {
			result |= SM::CASTLING | CastlingFlags::BLACK_CASTLE_Q;
		}
		
		return result;
	}

	uint32_t special_piece_move(Chessboard& board, int piece, uint32_t idx) {
		switch (piece) {
			case Pieces::W_PAWN: return white_pawn_special_move(board, idx);
			case Pieces::W_KING: return white_king_special_move(board, idx);

			case Pieces::B_PAWN: return black_pawn_special_move(board, idx);
			case Pieces::B_KING: return black_king_special_move(board, idx);
			default: return 0;
		}
	}
	
	bool isAttacked(Chessboard& board, uint32_t idx) {
		bool isWhite = Board::isWhite(board);
		uint64_t pieceMask = board.pieceMask;
		
		if (isWhite) {
			uint64_t _rook_move = (rook_move(pieceMask, idx) & ~board.whiteMask) & board.blackMask;
			if (_hasTwoPiece<Pieces::B_ROOK, Pieces::B_QUEEN>(board, _rook_move)) {
				return true;
			}
			
			uint64_t _bishop_move = (bishop_move(pieceMask, idx) & ~board.whiteMask) & board.blackMask;
			if (_hasTwoPiece<Pieces::B_BISHOP, Pieces::B_QUEEN>(board, _bishop_move)) {
				return true;
			}
			
			uint64_t _knight_move = (knight_move(idx) & ~board.whiteMask) & board.blackMask;
			if (_hasPiece<Pieces::B_KNIGHT>(board, _knight_move)) {
				return true;
			}
			
			uint64_t _king_move = (king_move(idx) & ~board.whiteMask) & board.blackMask;
			if (_hasPiece<Pieces::B_KING>(board, _king_move)) {
				return true;
			}
			
			uint64_t _pawn_move = white_pawn_attack(idx) & board.blackMask;
			return _hasPiece<Pieces::B_PAWN>(board, _pawn_move);
		} else {
			uint64_t _rook_move = (rook_move(pieceMask, idx) & ~board.blackMask) & board.whiteMask;
			if (_hasTwoPiece<Pieces::W_ROOK, Pieces::W_QUEEN>(board, _rook_move)) {
				return true;
			}
			
			uint64_t _bishop_move = (bishop_move(pieceMask, idx) & ~board.blackMask) & board.whiteMask;
			if (_hasTwoPiece<Pieces::W_BISHOP, Pieces::W_QUEEN>(board, _bishop_move)) {
				return true;
			}
			
			uint64_t _knight_move = (knight_move(idx) & ~board.blackMask) & board.whiteMask;
			if (_hasPiece<Pieces::W_KNIGHT>(board, _knight_move)) {
				return true;
			}
			
			uint64_t _king_move = (king_move(idx) & ~board.blackMask) & board.whiteMask;
			if (_hasPiece<Pieces::W_KING>(board, _king_move)) {
				return true;
			}
			
			uint64_t _pawn_move = black_pawn_attack(idx) & board.whiteMask;
			return _hasPiece<Pieces::W_PAWN>(board, _pawn_move);
		}
	}
	
	bool isKingAttacked(Chessboard& board, bool isWhite) {
		int old = board.halfMove;
		uint32_t idx;
		board.halfMove = isWhite ? 0 : 1;
		
		// Find the king
		if (isWhite) {
			idx = _getFirst<Pieces::W_KING>(board);
		} else {
			idx = _getFirst<Pieces::B_KING>(board);
		}
		
		if (idx != -1 && isAttacked(board, idx)) {
			board.halfMove = old;
			return true;
		}
		
		board.halfMove = old;
		return false;
	}

	// TODO: fixure out how to get this working
	/*
	template <bool WHITE>
	bool _Is_attacked(Chessboard& board, uint32_t idx) {
		uint64_t pieceMask = board.pieceMask;

		if constexpr (WHITE) {
			uint64_t _rook_move = (rook_move(pieceMask, idx) & ~board.whiteMask) & board.blackMask;
			if (_hasTwoPiece<Pieces::B_ROOK, Pieces::B_QUEEN>(board, _rook_move)) {
				return true;
			}

			uint64_t _bishop_move = (bishop_move(pieceMask, idx) & ~board.whiteMask) & board.blackMask;
			if (_hasTwoPiece<Pieces::B_BISHOP, Pieces::B_QUEEN>(board, _bishop_move)) {
				return true;
			}

			uint64_t _knight_move = (knight_move(idx) & ~board.whiteMask) & board.blackMask;
			if (_hasPiece<Pieces::B_KNIGHT>(board, _knight_move)) {
				return true;
			}

			uint64_t _king_move = (king_move(idx) & ~board.whiteMask) & board.blackMask;
			if (_hasPiece<Pieces::B_KING>(board, _king_move)) {
				return true;
			}

			uint64_t _pawn_move = white_pawn_attack(idx) & board.blackMask;
			return _hasPiece<Pieces::B_PAWN>(board, _pawn_move);
		} else {
			uint64_t _rook_move = (rook_move(pieceMask, idx) & ~board.blackMask) & board.whiteMask;
			if (_hasTwoPiece<Pieces::W_ROOK, Pieces::W_QUEEN>(board, _rook_move)) {
				return true;
			}

			uint64_t _bishop_move = (bishop_move(pieceMask, idx) & ~board.blackMask) & board.whiteMask;
			if (_hasTwoPiece<Pieces::W_BISHOP, Pieces::W_QUEEN>(board, _bishop_move)) {
				return true;
			}

			uint64_t _knight_move = (knight_move(idx) & ~board.blackMask) & board.whiteMask;
			if (_hasPiece<Pieces::W_KNIGHT>(board, _knight_move)) {
				return true;
			}

			uint64_t _king_move = (king_move(idx) & ~board.blackMask) & board.whiteMask;
			if (_hasPiece<Pieces::W_KING>(board, _king_move)) {
				return true;
			}

			uint64_t _pawn_move = black_pawn_attack(idx) & board.whiteMask;
			return _hasPiece<Pieces::W_PAWN>(board, _pawn_move);
		}
	}

	template <bool WHITE>
	bool _Is_king_attacked(Chessboard& board) {
		int old = board.halfMove;
		uint32_t idx;
		board.halfMove = WHITE ? 0 : 1;

		// find the king
		if constexpr (WHITE) {
			idx = _getFirst<Pieces::W_KING>(board);
		} else {
			idx = _getFirst<Pieces::B_KING>(board);
		}

		if (idx != -1 && PieceManager::_Is_attacked<WHITE>(board, idx)) {
			board.halfMove = old;
			return true;
		}

		board.halfMove = old;
		return false;
	}
	*/
}
