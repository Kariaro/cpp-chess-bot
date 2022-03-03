#pragma once

#ifndef GENERATOR_H
#define GENERATOR_H

#include <vector>
#include "utils_type.h"
#include "utils.h"
#include "chessboard.h"
#include "pieces.h"
#include "piece_manager.h"

const uint32_t PROMOTION_PIECES[4] {
	KNIGHT,
	BISHOP,
	QUEEN,
	ROOK,
};

namespace Generator {
	std::vector<Move> generate_valid_moves(Chessboard& board);
	std::vector<Move> generate_valid_quiesce_moves(Chessboard& board);

	bool isValid(Chessboard& board, uint32_t fromIdx, uint32_t toIdx, uint32_t special);
	bool isValid(Chessboard& board, Move& move);

	bool playMove(Chessboard& board, uint32_t fromIdx, uint32_t toIdx, uint32_t special);
	bool playMove(Chessboard& board, Move& move);
}

namespace Generator {
	template <bool White, int Type>
	_ForceInline bool _Is_valid(Chessboard& board, uint32_t fromIdx, uint32_t toIdx, uint32_t special) {
		if constexpr (Type == SM::NORMAL) {
			int oldFrom = board.pieces[fromIdx];
			int oldTo = board.pieces[toIdx];

			Board::setPiece(board, fromIdx, Pieces::NONE);
			Board::setPiece(board, toIdx, oldFrom);

			bool isValid = !PieceManager::_Is_king_attacked<White>(board);

			Board::setPiece(board, fromIdx, oldFrom);
			Board::setPiece(board, toIdx, oldTo);

			return isValid;
		} else if constexpr (Type == SM::CASTLING) {
			if ((special & CastlingFlags::ANY_CASTLE_K) != 0) {
				return !(PieceManager::_Is_attacked<White>(board, fromIdx)
					|| PieceManager::_Is_attacked<White>(board, fromIdx + 1)
					|| PieceManager::_Is_attacked<White>(board, fromIdx + 2));
			}

			if ((special & CastlingFlags::ANY_CASTLE_Q) != 0) {
				return !(PieceManager::_Is_attacked<White>(board, fromIdx)
					|| PieceManager::_Is_attacked<White>(board, fromIdx - 1)
					|| PieceManager::_Is_attacked<White>(board, fromIdx - 2));
			}

			return false;
		} else if constexpr (Type == SM::EN_PASSANT) {
			int oldFrom = board.pieces[fromIdx];
			int remIdx = toIdx + (White ? -8 : 8);
			int oldRem = board.pieces[remIdx];
			int oldTo = board.pieces[toIdx];

			Board::setPiece(board, fromIdx, Pieces::NONE);
			Board::setPiece(board, remIdx, Pieces::NONE);
			Board::setPiece(board, toIdx, oldFrom);

			bool isValid = !PieceManager::_Is_king_attacked<White>(board);

			Board::setPiece(board, fromIdx, oldFrom);
			Board::setPiece(board, remIdx, oldRem);
			Board::setPiece(board, toIdx, oldTo);

			return isValid;
		} else if constexpr (Type == SM::PROMOTION) {
			int oldFrom = board.pieces[fromIdx];
			int oldTo = board.pieces[toIdx];

			Board::setPiece(board, fromIdx, Pieces::NONE);
			// We do not need this piece to have the correct value here because it would not change the outcome
			Board::setPiece(board, toIdx, oldFrom);

			bool isValid = !PieceManager::_Is_king_attacked<White>(board);

			Board::setPiece(board, fromIdx, oldFrom);
			Board::setPiece(board, toIdx, oldTo);

			return isValid;
		}

		return false;
	}

	template <bool White, int Type>
	_ForceInline bool _Is_valid(Chessboard& board, Move& move) {
		return _Is_valid<White, Type>(board, move.from, move.to, move.special);
	}

	template <bool White>
	_ForceInline void _Generate_valid_moves(std::vector<Move>& vector_moves, Chessboard& board) {
		// 128 -> ~ 48.3 sec
		//  96 -> ~ 48.0 sec
		//  64 -> ~ 53.8 sec
		vector_moves.clear();

		uint64_t mask;
		if constexpr (White) {
			mask = board.whiteMask;
		} else {
			mask = board.blackMask;
		}

		while (mask != 0) {
			uint64_t pick = Utils::lowestOneBit(mask);
			mask &= ~pick;
			uint32_t idx = (uint32_t)Utils::numberOfTrailingZeros(pick);
			
			int piece = board.pieces[idx];
			uint64_t moves = PieceManager::piece_move(board, piece, idx);
			
			while (moves != 0) {
				uint64_t move_bit = Utils::lowestOneBit(moves);
				moves &= ~move_bit;
				uint32_t move_idx = (uint32_t)Utils::numberOfTrailingZeros(move_bit);
				
				if (_Is_valid<White, SM::NORMAL>(board, idx, move_idx, 0)) {
					vector_moves.push_back({ idx, move_idx, 0, true });
				}
			}
			
			int pieceSq = piece * piece;
			if (pieceSq == 1 || pieceSq == 36) {
				uint32_t special = (uint32_t)PieceManager::special_piece_move(board, piece, idx);
				int type = special & 0b11000000;
				if (type == SM::CASTLING) {
					// Split the castling moves up into multiple moves
					uint32_t specialFlag;
					if ((specialFlag = (special & CastlingFlags::ANY_CASTLE_K)) != 0) {
						Move move = { idx, (uint32_t)(idx + 2), (uint32_t)(SM::CASTLING | specialFlag), true };
						if (_Is_valid<White, SM::CASTLING>(board, move)) {
							vector_moves.push_back(move);
						}
					}
					if ((specialFlag = (special & CastlingFlags::ANY_CASTLE_Q)) != 0) {
						Move move = { idx, (uint32_t)(idx - 2), (uint32_t)(SM::CASTLING | specialFlag), true };
						if (_Is_valid<White, SM::CASTLING>(board, move)) {
							vector_moves.push_back(move);
						}
					}

				} else if (type == SM::EN_PASSANT) {
					if (_Is_valid<White, SM::EN_PASSANT>(board, idx, special & 0b111111, special)) {
						vector_moves.push_back({ idx, (uint32_t)(special & 0b111111), special, true });
					}

				} else if (type == SM::PROMOTION) {
					// Split promotion into multiple moves
					uint32_t toIdx = (uint32_t)(idx + (White ? 8 : -8));
					uint32_t specialFlag;
					if ((specialFlag = (special & Promotion::LEFT)) != 0) {
						for (uint32_t promotionPiece : PROMOTION_PIECES) {
							Move move = { idx, (uint32_t)(toIdx - 1), (uint32_t)(SM::PROMOTION | promotionPiece << 3 | specialFlag), true };
							if (_Is_valid<White, SM::PROMOTION>(board, move)) {
								vector_moves.push_back(move);
							}
						}
					}
					if ((specialFlag = (special & Promotion::MIDDLE)) != 0) {
						for (uint32_t promotionPiece : PROMOTION_PIECES) {
							Move move = { idx, (uint32_t)(toIdx), (uint32_t)(SM::PROMOTION | promotionPiece << 3 | specialFlag), true };
							if (_Is_valid<White, SM::PROMOTION>(board, move)) {
								vector_moves.push_back(move);
							}
						}
					}
					if ((specialFlag = (special & Promotion::RIGHT)) != 0) {
						for (uint32_t promotionPiece : PROMOTION_PIECES) {
							Move move = { idx, (uint32_t)(toIdx + 1), (uint32_t)(SM::PROMOTION | promotionPiece << 3 | specialFlag), true };
							if (_Is_valid<White, SM::PROMOTION>(board, move)) {
								vector_moves.push_back(move);
							}
						}
					}
				}
			}
		}
	}
}

#endif // !GENERATOR_H


