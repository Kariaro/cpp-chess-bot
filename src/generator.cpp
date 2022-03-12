#include "generator.h"

template <int T>
bool _isValid(Chessboard& board, uint32_t fromIdx, uint32_t toIdx, uint32_t special) {
	bool isWhite = Board::isWhite(board);
	
	if constexpr (T == SM::NORMAL) {
		int oldFrom = board.pieces[fromIdx];
		int oldTo = board.pieces[toIdx];
		
		Board::setPiece(board, fromIdx, Pieces::NONE);
		Board::setPiece(board, toIdx, oldFrom);
		
		bool isValid = !PieceManager::isKingAttacked(board, isWhite);
		
		Board::setPiece(board, fromIdx, oldFrom);
		Board::setPiece(board, toIdx, oldTo);

		return isValid;
	}
	
	if constexpr (T == SM::CASTLING) {
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
	
	if constexpr (T == SM::EN_PASSANT) {
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
	
	if constexpr (T == SM::PROMOTION) {
		int oldFrom = board.pieces[fromIdx];
		int oldTo = board.pieces[toIdx];
		
		Board::setPiece(board, fromIdx, Pieces::NONE);
		// Because the promotion just blocks we do not need to calculate what type the piece is
		Board::setPiece(board, toIdx, oldFrom); //((special >> 3) & 7) * (isWhite ? 1 : -1));
		
		bool isValid = !PieceManager::isKingAttacked(board, isWhite);
		
		Board::setPiece(board, fromIdx, oldFrom);
		Board::setPiece(board, toIdx, oldTo);
		
		return isValid;
	}

	return false;
}

template <int T>
bool _isValid(Chessboard& board, Move& move) {
	return _isValid<T>(board, move.from, move.to, move.special);
}

namespace Generator {
	bool isValid(Chessboard& board, uint32_t fromIdx, uint32_t toIdx, uint32_t special) {
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
	
	bool isValid(Chessboard& board, Move& move) {
		return isValid(board, move.from, move.to, move.special);
	}
	
	bool playMove(Chessboard& board, uint32_t fromIdx, uint32_t toIdx, uint32_t special) {
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
				
				Board::setPiece(board, fromIdx, Pieces::NONE);
				Board::setPiece(board, toIdx, oldFrom);
				break;
			}
			
			case SM::CASTLING: {
				if ((special & CastlingFlags::ANY_CASTLE_K) != 0) {
					Board::setPiece(board, fromIdx + 3, Pieces::NONE);
					Board::setPiece(board, fromIdx + 2, Pieces::KING * mul);
					Board::setPiece(board, fromIdx + 1, Pieces::ROOK * mul);
					Board::setPiece(board, fromIdx, Pieces::NONE);
					board.flags &= isWhite ? ~CastlingFlags::WHITE_CASTLE_ANY : ~CastlingFlags::BLACK_CASTLE_ANY;
				}
				
				if ((special & CastlingFlags::ANY_CASTLE_Q) != 0) {
					Board::setPiece(board, fromIdx - 4, Pieces::NONE);
					Board::setPiece(board, fromIdx - 2, Pieces::KING * mul);
					Board::setPiece(board, fromIdx - 1, Pieces::ROOK * mul);
					Board::setPiece(board, fromIdx, Pieces::NONE);
					board.flags &= isWhite ? ~CastlingFlags::WHITE_CASTLE_ANY : ~CastlingFlags::BLACK_CASTLE_ANY;
				}

				break;
			}
			
			case SM::EN_PASSANT: {
				int oldFrom = board.pieces[fromIdx];
				int remIdx = toIdx - 8 * mul; // + (isWhite ? -8 : 8);
				
				nextLastCapture = 0;
				Board::setPiece(board, fromIdx, Pieces::NONE);
				Board::setPiece(board, remIdx, Pieces::NONE);
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
						Board::setPiece(board, fromIdx, Pieces::NONE);
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
	
	bool playMove(Chessboard& board, Move& move) {
		return playMove(board, move.from, move.to, move.special);
	}
}
