#include "generator.h"

namespace Generator {
	bool isValid(Chessboard& board, uint8_t fromIdx, uint8_t toIdx, uint8_t special) {
		bool isWhite = Board::isWhite(board);
		
		if (special == 0) {
			int oldFrom = board.pieces[fromIdx];
			int oldTo = board.pieces[toIdx];
			
			Board::setPiece(board, fromIdx, Pieces::NONE);
			Board::setPiece(board, toIdx, oldFrom);
			
			bool isValid = !PieceManager::isKingAttacked(board, isWhite);
			
			Board::setPiece(board, fromIdx, oldFrom);
			Board::setPiece(board, toIdx, oldTo);
			
			return isValid;
		} else {
			int type = special & 0b11000000;
			
			switch (type) {
				case SM::CASTLING: {
					if ((special & CastlingFlags::ANY_CASTLE_K) != 0) {
						return !(PieceManager::isAttacked(board, fromIdx)
							|| PieceManager::isAttacked(board, fromIdx + 1)
							|| PieceManager::isAttacked(board, fromIdx + 2));
					}
					
					if ((special & CastlingFlags::ANY_CASTLE_Q) != 0) {
						return !(PieceManager::isAttacked(board, fromIdx)
							|| PieceManager::isAttacked(board, fromIdx - 1)
							|| PieceManager::isAttacked(board, fromIdx - 2));
					}

					return false;
				}
				
				case SM::EN_PASSANT: {
					int oldFrom = board.pieces[fromIdx];
					int remIdx = toIdx + (isWhite ? -8 : 8);
					int oldRem = board.pieces[remIdx];
					int oldTo = board.pieces[toIdx];
					
					Board::setPiece(board, fromIdx, Pieces::NONE);
					Board::setPiece(board, remIdx, Pieces::NONE);
					Board::setPiece(board, toIdx, oldFrom);
					
					bool isValid = !PieceManager::isKingAttacked(board, isWhite);
					
					Board::setPiece(board, fromIdx, oldFrom);
					Board::setPiece(board, remIdx, oldRem);
					Board::setPiece(board, toIdx, oldTo);
					
					return isValid;
				}
				
				case SM::PROMOTION: {
					int oldFrom = board.pieces[fromIdx];
					int oldTo = board.pieces[toIdx];
					
					Board::setPiece(board, fromIdx, Pieces::NONE);
					// We do not need this piece to have the correct value here because it would not change the outcome
					Board::setPiece(board, toIdx, oldFrom);
					
					bool isValid = !PieceManager::isKingAttacked(board, isWhite);
					
					Board::setPiece(board, fromIdx, oldFrom);
					Board::setPiece(board, toIdx, oldTo);
					
					return isValid;
				}
			}
			
			return false;
		}
	}
	
	bool isValid(Chessboard& board, const Move move) {
		return isValid(board, get_move_from(move), get_move_to(move), get_move_special(move));
	}

	template <int Piece>
	inline void test_setPiece(Chessboard& board, uint32_t idx, int piece) {
		int old = board.pieces[idx];
		board.pieces[idx] = piece;

		uint64_t mask = (uint64_t)(1ull) << idx;
		if constexpr (piece >= 0) {
			if (old < 0) board.blackMask &= ~mask;
		}

		if constexpr (piece <= 0) {
			if (old > 0) board.whiteMask &= ~mask;
		}

		if constexpr (piece > 0) board.whiteMask |= mask;
		if constexpr (piece < 0) board.blackMask |= mask;
		board.pieceMask = board.blackMask | board.whiteMask;
	}

	
	bool playMove(Chessboard& board, uint8_t fromIdx, uint8_t toIdx, uint8_t special) {
		bool isWhite = Board::isWhite(board);
		int mul = isWhite ? 1 : -1;
		
		// Increase moves since last capture
		int nextLastCapture = board.lastCapture + 1;
		int nextHalfMove = board.halfMove + 1;
		int nextLastPawn = 0;
		
		switch (special & 0b11000000) {
			case SM::NORMAL: {
				int oldFrom = board.pieces[fromIdx];
				int oldTo = board.pieces[toIdx];
				int pieceSq = oldFrom * oldFrom;
				
				switch (pieceSq) {
					case Pieces::ROOK_SQ: {
						if (isWhite) {
							if (fromIdx == CastlingFlags::WHITE_ROOK_K) {
								board.flags &= ~CastlingFlags::WHITE_CASTLE_K;
							}
							
							if (fromIdx == CastlingFlags::WHITE_ROOK_Q) {
								board.flags &= ~CastlingFlags::WHITE_CASTLE_Q;
							}
						} else {
							if (fromIdx == CastlingFlags::BLACK_ROOK_K) {
								board.flags &= ~CastlingFlags::BLACK_CASTLE_K;
							}
							
							if (fromIdx == CastlingFlags::BLACK_ROOK_Q) {
								board.flags &= ~CastlingFlags::BLACK_CASTLE_Q;
							}
						}
						break;
					}
					case Pieces::KING_SQ: {
						if (isWhite) {
							if (fromIdx == CastlingFlags::WHITE_KING) {
								board.flags &= ~CastlingFlags::WHITE_CASTLE_ANY;
							}
						} else {
							if (fromIdx == CastlingFlags::BLACK_KING) {
								board.flags &= ~CastlingFlags::BLACK_CASTLE_ANY;
							}
						}
						break;
					}
					case Pieces::PAWN_SQ: {
						// Only double jumps are saved
						int distance = (fromIdx - toIdx) * (fromIdx - toIdx);
						
						// Because double pawns jump two rows they will always have a distance of 256
						if (distance == 256) {
							nextLastPawn = toIdx - 8 * mul; // + (isWhite ? -8 : 8);
						}
						
						nextLastCapture = 0;
						break;
					}
				}
				
				if (oldTo != Pieces::NONE) {
					// Capture
					nextLastCapture = 0;
				}
				
				if (board.flags != 0) {
					// Recalculate the castling flags
					if (isWhite) {
						if (toIdx == CastlingFlags::BLACK_ROOK_K) {
							board.flags &= ~CastlingFlags::BLACK_CASTLE_K;
						}
						
						if (toIdx == CastlingFlags::BLACK_ROOK_Q) {
							board.flags &= ~CastlingFlags::BLACK_CASTLE_Q;
						}
					} else {
						if (toIdx == CastlingFlags::WHITE_ROOK_K) {
							board.flags &= ~CastlingFlags::WHITE_CASTLE_K;
						}
						
						if (toIdx == CastlingFlags::WHITE_ROOK_Q) {
							board.flags &= ~CastlingFlags::WHITE_CASTLE_Q;
						}
					}
				}
				
				//Board::setPiece(board, fromIdx, Pieces::NONE);
				Board::setPiece<Pieces::NONE>(board, fromIdx);
				Board::setPiece(board, toIdx, oldFrom);
				break;
			}
			
			case SM::CASTLING: {
				if ((special & CastlingFlags::ANY_CASTLE_K) != 0) {
					Board::setPiece<Pieces::NONE>(board, fromIdx + 3);
					Board::setPiece(board, fromIdx + 2, Pieces::KING * mul);
					Board::setPiece(board, fromIdx + 1, Pieces::ROOK * mul);
					Board::setPiece<Pieces::NONE>(board, fromIdx);
					board.flags &= isWhite ? ~CastlingFlags::WHITE_CASTLE_ANY : ~CastlingFlags::BLACK_CASTLE_ANY;
				}
				
				if ((special & CastlingFlags::ANY_CASTLE_Q) != 0) {
					Board::setPiece<Pieces::NONE>(board, fromIdx - 4);
					Board::setPiece(board, fromIdx - 2, Pieces::KING * mul);
					Board::setPiece(board, fromIdx - 1, Pieces::ROOK * mul);
					Board::setPiece<Pieces::NONE>(board, fromIdx);
					board.flags &= isWhite ? ~CastlingFlags::WHITE_CASTLE_ANY : ~CastlingFlags::BLACK_CASTLE_ANY;
				}

				break;
			}
			
			case SM::EN_PASSANT: {
				int oldFrom = board.pieces[fromIdx];
				int remIdx = toIdx - 8 * mul; // + (isWhite ? -8 : 8);
				
				nextLastCapture = 0;
				Board::setPiece<Pieces::NONE>(board, fromIdx);
				Board::setPiece<Pieces::NONE>(board, remIdx);
				Board::setPiece(board, toIdx, oldFrom);
				break;
			}
			
			case SM::PROMOTION: {
				int piece = (special & 0b111000) >> 3;
				
				switch (piece) {
					case Pieces::QUEEN:
					case Pieces::BISHOP:
					case Pieces::KNIGHT:
					case Pieces::ROOK: {
						int oldFrom = board.pieces[fromIdx];
						Board::setPiece<Pieces::NONE>(board, fromIdx);
						Board::setPiece(board, toIdx, piece * mul);
						
						if (oldFrom != 0) {
							nextLastCapture = 0;
						}
						break;
					}
					default: {
						return false;
					}
				}

				break;
			}
		}
		
		board.lastCapture = nextLastCapture;
		board.lastPawn = nextLastPawn;
		board.halfMove = nextHalfMove;
		return true;
	}
	
	bool playMove(Chessboard& board, const Move move) {
		return playMove(board, get_move_from(move), get_move_to(move), get_move_special(move));
	}
}
