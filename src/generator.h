#ifndef GENERATOR_H
#define GENERATOR_H

#include <vector>
#include "utils_type.h"
#include "utils.h"
#include "chessboard.h"
#include "codec/fen_codec.h"
#include "serial.h"
#include "pieces.h"
#include "piece_manager.h"

//#define while_bits(a, b)\
//	for(uint64_t _ ## a ## _pick; a != 0;)\
//	for(uint32_t b; a != 0; _ ## a ## _pick = Utils::lowestOneBit(a), a &= ~_ ## a ## _pick, b = Utils::numberOfTrailingZeros(_ ## a ## _pick))

const uint32_t PROMOTION_PIECES[4] {
	KNIGHT,
	BISHOP,
	QUEEN,
	ROOK,
};

namespace Generator {
	template <bool White>
	_Inline bool _Is_valid(Chessboard& board, uint8_t fromIdx, uint8_t toIdx) {
		int oldFrom = board.pieces[fromIdx];
		int oldTo = board.pieces[toIdx];

		Board::setPiece(board, fromIdx, Pieces::NONE);
		Board::setPiece(board, toIdx, oldFrom);

		bool isValid = !PieceManager::_Is_king_attacked<White>(board);

		Board::setPiece(board, fromIdx, oldFrom);
		Board::setPiece(board, toIdx, oldTo);

		return isValid;
	}

	template <bool White, int Type>
	_Inline bool _Is_valid(Chessboard& board, uint8_t fromIdx, uint8_t toIdx, uint8_t special) {
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
	constexpr bool _Is_valid(Chessboard& board, const Move move) {
		return _Is_valid<White, Type>(board, get_move_from(move), get_move_to(move), get_move_special(move));
	}

	template <bool White>
	_Inline void _Generate_valid_quiesce_moves(std::vector<Move>& vector_moves, Chessboard& board) {
		uint64_t mask;
		if constexpr (White) {
			mask = board.whiteMask;
		} else {
			mask = board.blackMask;
		}

		bool is_attacked = PieceManager::_Is_king_attacked<White>(board);

		// If the king is attacked all pieces are pinned
		uint64_t pinned_pieces = 0xffffffffffffffffull;
		if (!is_attacked) {
			pinned_pieces = PieceManager::_Get_attack_square<White>(board);
		}

		// If the king is not attacked only pieces the king can see can give check
		while (mask != 0) {
			uint64_t pick = Utils::lowestOneBit(mask);
			mask &= ~pick;
			uint8_t idx = Utils::numberOfTrailingZeros(pick);

			int piece = board.pieces[idx];
			uint64_t moves = PieceManager::piece_move(board, piece, idx);

			while (moves != 0) {
				uint64_t move_bit = Utils::lowestOneBit(moves);
				moves &= ~move_bit;
				uint8_t move_idx = Utils::numberOfTrailingZeros(move_bit);

				// Only pieces that lay on an attacking position towards the king needs to be checked
				if (board.pieces[move_idx] != 0 && ((pick & pinned_pieces) == 0 || _Is_valid<White>(board, idx, move_idx))) {
					vector_moves.push_back(create_move(idx, move_idx, 0, true));
				}
			}

			int pieceSq = piece * piece;
			if (pieceSq == 1 || pieceSq == 36) {
				uint8_t special = (uint8_t)PieceManager::special_piece_move(board, piece, idx);
				int type = special & 0b11000000;
				if (type == SM::CASTLING) {
					// Split the castling moves up into multiple moves
					uint8_t specialFlag;
					if ((specialFlag = (special & CastlingFlags::ANY_CASTLE_K)) != 0) {
						Move move = create_move(idx, (uint8_t)(idx + 2), (uint8_t)(SM::CASTLING | specialFlag), true);
						if (_Is_valid<White, SM::CASTLING>(board, move)) {
							vector_moves.push_back(move);
						}
					}
					if ((specialFlag = (special & CastlingFlags::ANY_CASTLE_Q)) != 0) {
						Move move = create_move(idx, (uint8_t)(idx - 2), (uint8_t)(SM::CASTLING | specialFlag), true);
						if (_Is_valid<White, SM::CASTLING>(board, move)) {
							vector_moves.push_back(move);
						}
					}

				} else if (type == SM::EN_PASSANT) {
					uint8_t move_idx = (uint8_t)(special & 0b111111);
					if (_Is_valid<White, SM::EN_PASSANT>(board, idx, move_idx, special)) {
						vector_moves.push_back(create_move(idx, move_idx, special, true));
					}

				} else if (type == SM::PROMOTION) {
					// Split promotion into multiple moves
					uint8_t toIdx = (uint8_t)(idx + (White ? 8 : -8));
					uint8_t specialFlag;

					if ((specialFlag = (special & Promotion::LEFT)) != 0) {
						// test if move is valid
						Move move = create_move(idx, (uint8_t)(toIdx - 1), (uint8_t)(SM::PROMOTION | specialFlag), true);

						if (_Is_valid<White, SM::PROMOTION>(board, move)) {
							for (uint32_t promotionPiece : PROMOTION_PIECES) {
								vector_moves.push_back(create_move(idx, (uint8_t)(toIdx - 1), (uint8_t)(SM::PROMOTION | promotionPiece << 3 | specialFlag), true));
							}
						}
					}
					if ((specialFlag = (special & Promotion::MIDDLE)) != 0) {
						Move move = create_move(idx, (uint8_t)(toIdx), (uint8_t)(SM::PROMOTION | specialFlag), true);

						if (_Is_valid<White, SM::PROMOTION>(board, move)) {
							for (uint32_t promotionPiece : PROMOTION_PIECES) {
								vector_moves.push_back(create_move(idx, (uint8_t)(toIdx), (uint8_t)(SM::PROMOTION | promotionPiece << 3 | specialFlag), true));
							}
						}
					}
					if ((specialFlag = (special & Promotion::RIGHT)) != 0) {
						Move move = create_move(idx, (uint8_t)(toIdx + 1), (uint8_t)(SM::PROMOTION | specialFlag), true);

						if (_Is_valid<White, SM::PROMOTION>(board, move)) {
							for (uint32_t promotionPiece : PROMOTION_PIECES) {
								vector_moves.push_back(create_move(idx, (uint8_t)(toIdx + 1), (uint8_t)(SM::PROMOTION | promotionPiece << 3 | specialFlag), true));
							}
						}
					}

					/*
					if ((specialFlag = (special & Promotion::LEFT)) != 0) {
						for (uint32_t promotionPiece : PROMOTION_PIECES) {
							Move move = create_move(idx, (uint8_t)(toIdx - 1), (uint8_t)(SM::PROMOTION | promotionPiece << 3 | specialFlag), true);
							if (_Is_valid<White, SM::PROMOTION>(board, move)) {
								vector_moves.push_back(move);
							}
						}
					}
					if ((specialFlag = (special & Promotion::MIDDLE)) != 0) {
						for (uint32_t promotionPiece : PROMOTION_PIECES) {
							Move move = create_move(idx, (uint8_t)(toIdx), (uint8_t)(SM::PROMOTION | promotionPiece << 3 | specialFlag), true);
							if (_Is_valid<White, SM::PROMOTION>(board, move)) {
								vector_moves.push_back(move);
							}
						}
					}
					if ((specialFlag = (special & Promotion::RIGHT)) != 0) {
						for (uint32_t promotionPiece : PROMOTION_PIECES) {
							Move move = create_move(idx, (uint8_t)(toIdx + 1), (uint8_t)(SM::PROMOTION | promotionPiece << 3 | specialFlag), true);
							if (_Is_valid<White, SM::PROMOTION>(board, move)) {
								vector_moves.push_back(move);
							}
						}
					}
					*/
				}
			}
		}
	}

	template <bool White>
	_Inline void _Generate_valid_moves(std::vector<Move>& vector_moves, Chessboard& board) {
		uint64_t mask;
		if constexpr (White) {
			mask = board.whiteMask;
		} else {
			mask = board.blackMask;
		}

		bool is_attacked = PieceManager::_Is_king_attacked<White>(board);

		// If the king is attacked all pieces are pinned
		uint64_t pinned_pieces = 0xffffffffffffffffull;
		if (!is_attacked) {
			pinned_pieces = PieceManager::_Get_attack_square<White>(board);
		}

		// If the king is not attacked only pieces the king can see can give check
		while (mask != 0) {
			uint64_t pick = Utils::lowestOneBit(mask);
			mask &= ~pick;
			uint8_t idx = Utils::numberOfTrailingZeros(pick);

			int piece = board.pieces[idx];
			uint64_t moves = PieceManager::piece_move(board, piece, idx);

			while (moves != 0) {
				uint64_t move_bit = Utils::lowestOneBit(moves);
				moves &= ~move_bit;
				uint8_t move_idx = Utils::numberOfTrailingZeros(move_bit);

				// Only pieces that lay on an attacking position towards the king needs to be checked
				if ((pick & pinned_pieces) == 0 || _Is_valid<White>(board, idx, move_idx)) {
					vector_moves.push_back(create_move(idx, move_idx, 0, true));
				}
			}

			int pieceSq = piece * piece;
			if (pieceSq == 1 || pieceSq == 36) {
				uint8_t special = (uint8_t)PieceManager::special_piece_move(board, piece, idx);
				int type = special & 0b11000000;
				if (type == SM::CASTLING) {
					// Split the castling moves up into multiple moves
					uint8_t specialFlag;
					if ((specialFlag = (special & CastlingFlags::ANY_CASTLE_K)) != 0) {
						Move move = create_move(idx, (uint8_t)(idx + 2), (uint8_t)(SM::CASTLING | specialFlag), true);
						if (_Is_valid<White, SM::CASTLING>(board, move)) {
							vector_moves.push_back(move);
						}
					}
					if ((specialFlag = (special & CastlingFlags::ANY_CASTLE_Q)) != 0) {
						Move move = create_move(idx, (uint8_t)(idx - 2), (uint8_t)(SM::CASTLING | specialFlag), true);
						if (_Is_valid<White, SM::CASTLING>(board, move)) {
							vector_moves.push_back(move);
						}
					}

				} else if (type == SM::EN_PASSANT) {
					Move move = create_move(idx, (uint8_t)(special & 0b111111), special, true);
					if (_Is_valid<White, SM::EN_PASSANT>(board, move)) {
						vector_moves.push_back(move);
					}

				} else if (type == SM::PROMOTION) {
					// Split promotion into multiple moves
					uint8_t toIdx = (uint8_t)(idx + (White ? 8 : -8));
					uint8_t specialFlag;
					
					if ((specialFlag = (special & Promotion::LEFT)) != 0) {
						Move move = create_move(idx, (uint8_t)(toIdx - 1), (uint8_t)(SM::PROMOTION | specialFlag), true);

						if (_Is_valid<White, SM::PROMOTION>(board, move)) {
							for (uint32_t promotionPiece : PROMOTION_PIECES) {
								vector_moves.push_back(create_move(idx, (uint8_t)(toIdx - 1), (uint8_t)(SM::PROMOTION | promotionPiece << 3 | specialFlag), true));
							}
						}
					}
					if ((specialFlag = (special & Promotion::MIDDLE)) != 0) {
						Move move = create_move(idx, (uint8_t)(toIdx), (uint8_t)(SM::PROMOTION | specialFlag), true);

						if (_Is_valid<White, SM::PROMOTION>(board, move)) {
							for (uint32_t promotionPiece : PROMOTION_PIECES) {
								vector_moves.push_back(create_move(idx, (uint8_t)(toIdx), (uint8_t)(SM::PROMOTION | promotionPiece << 3 | specialFlag), true));
							}
						}
					}
					if ((specialFlag = (special & Promotion::RIGHT)) != 0) {
						Move move = create_move(idx, (uint8_t)(toIdx + 1), (uint8_t)(SM::PROMOTION | specialFlag), true);

						if (_Is_valid<White, SM::PROMOTION>(board, move)) {
							for (uint32_t promotionPiece : PROMOTION_PIECES) {
								vector_moves.push_back(create_move(idx, (uint8_t)(toIdx + 1), (uint8_t)(SM::PROMOTION | promotionPiece << 3 | specialFlag), true));
							}
						}
					}

					/*
					if ((specialFlag = (special & Promotion::LEFT)) != 0) {
						for (uint32_t promotionPiece : PROMOTION_PIECES) {
							Move move = create_move(idx, (uint8_t)(toIdx - 1), (uint8_t)(SM::PROMOTION | promotionPiece << 3 | specialFlag), true);
							if (_Is_valid<White, SM::PROMOTION>(board, move)) {
								vector_moves.push_back(move);
							}
						}
					}
					if ((specialFlag = (special & Promotion::MIDDLE)) != 0) {
						for (uint32_t promotionPiece : PROMOTION_PIECES) {
							Move move = create_move(idx, (uint8_t)(toIdx), (uint8_t)(SM::PROMOTION | promotionPiece << 3 | specialFlag), true);
							if (_Is_valid<White, SM::PROMOTION>(board, move)) {
								vector_moves.push_back(move);
							}
						}
					}
					if ((specialFlag = (special & Promotion::RIGHT)) != 0) {
						for (uint32_t promotionPiece : PROMOTION_PIECES) {
							Move move = create_move(idx, (uint8_t)(toIdx + 1), (uint8_t)(SM::PROMOTION | promotionPiece << 3 | specialFlag), true);
							if (_Is_valid<White, SM::PROMOTION>(board, move)) {
								vector_moves.push_back(move);
							}
						}
					}
					*/
				}
			}
		}
	}
}

namespace Generator {
	_Inline std::vector<Move> generate_valid_moves(Chessboard& board) {
		std::vector<Move> moves;
		moves.reserve(96);
		return (Board::isWhite(board)
			? _Generate_valid_moves<WHITE>(moves, board)
			: _Generate_valid_moves<BLACK>(moves, board), moves);
	}

	_Inline std::vector<Move> generate_valid_quiesce_moves(Chessboard& a_board) {
		std::vector<Move> moves;
		moves.reserve(96);
		return (Board::isWhite(a_board)
			? _Generate_valid_quiesce_moves<WHITE>(moves, a_board)
			: _Generate_valid_quiesce_moves<BLACK>(moves, a_board), moves);
	}

	//bool isValid(Chessboard& board, uint32_t fromIdx, uint32_t toIdx, uint32_t special);
	//bool isValid(Chessboard& board, Move move);

	// bool playMove(Chessboard& board, uint8_t fromIdx, uint8_t toIdx, uint8_t special);
	bool playMove(Chessboard& board, const Move move);
}

#endif // GENERATOR_H
