#include <sstream>
#include "fen_codec.h"
#include "../serial.h"
#include "../pieces.h"

static int _Read_number(const std::string& str, int& matched) {
	int value = std::atoi(str.c_str() + matched);

	int loop = value;
	do {
		loop /= 10;
		matched++;
	} while (loop != 0);

	return value;
}

static int _Read_square(std::string& str, int matched) {
	int file = str[matched      ] - 'a';
	int rank = str[matched + 1ll] - '1';

	if (file < 0 || file > 7 || rank < 0 || rank > 7) {
		return -1;
	}

	return (file) + (rank << 3);
}

int Codec::FEN::import_fen(Chessboard& board, std::string str, int& matched) {
	int lastCapture;
	int lastPawn;
	int halfMove;
	int flags;
	char c;

	matched = 0;

	// clear the board
	for (int i = 0; i < 64; i++) {
		board.pieces[i] = Pieces::NONE;
	}

	{
		int rank = 0;
		int file = 0;
		for (;;) {
			c = str[matched++];
			if (c == '/') {
				if (file != 8) {
					return FEN_CODEC_INVALID_FILE;
				}

				if (++rank > 7) {
					return FEN_CODEC_INVALID_RANK_OOB;
				}

				file = 0;
				continue;
			}

			if (c == ' ') {
				if (file != 8 && rank != 7) {
					return FEN_CODEC_INVALID_BOARD;
				}

				break;
			}

			uint32_t idx = (file) + ((7 - rank) << 3);
			switch (c) {
				case 'r': case 'n': case 'b': case 'q': case 'k': case 'p':
				case 'R': case 'N': case 'B': case 'Q': case 'K': case 'P': {
					board.pieces[(++file, idx)] = Serial::get_piece_from_character(c);
					break;
				}

				case '1': case '2': case '3': case '4':
				case '5': case '6': case '7': case '8': {
					file += c - '0';
					break;
				}

				default: {
					return FEN_CODEC_INVALID_CHARACTER;
				}
			}

			if (file > 8) {
				return FEN_CODEC_INVALID_FILE_OOB;
			}
		}
	}

	// Read turn
	c = str[matched++];
	if (c != 'w' && c != 'b') {
		return FEN_CODEC_INVALID_TURN;
	}

	if (str[matched++] != ' ') {
		return FEN_CODEC_INVALID_FORMAT;
	}

	halfMove = (c == 'w') ? 0 : 1;

	// Read flags
	{
		flags = 0;
		if (str[matched] != '-') {
			if (str[matched] == 'K') {
				flags |= CastlingFlags::WHITE_CASTLE_K;
				matched++;
			}

			if (str[matched] == 'Q') {
				flags |= CastlingFlags::WHITE_CASTLE_Q;
				matched++;
			}

			if (str[matched] == 'k') {
				flags |= CastlingFlags::BLACK_CASTLE_K;
				matched++;
			}

			if (str[matched] == 'q') {
				flags |= CastlingFlags::BLACK_CASTLE_Q;
				matched++;
			}
		} else {
			matched++;
		}

		if (str[matched++] != ' ') {
			return FEN_CODEC_INVALID_FORMAT;
		}
	}

	// En passant
	{
		lastPawn = 0;
		if (str[matched] != '-') {
			lastPawn = _Read_square(str, matched);

			if (lastPawn < 0) {
				return FEN_CODEC_INVALID_FORMAT;
			}
			matched += 2;
		} else {
			matched++;
		}

		if (str[matched++] != ' ') {
			return FEN_CODEC_INVALID_FORMAT;
		}
	}


	lastCapture = _Read_number(str, matched);
	if (str[matched++] != ' ') {
		return FEN_CODEC_INVALID_FORMAT;
	}

	halfMove = halfMove + 2 * _Read_number(str, matched);

	uint64_t whiteMask = 0;
	uint64_t blackMask = 0;
	for (int i = 0; i < 64; i++) {
		int piece = board.pieces[i];

		if (piece < 0) {
			blackMask |= (uint64_t)(1) << i;
		}

		if (piece > 0) {
			whiteMask |= (uint64_t)(1) << i;
		}
	}

	board.lastCapture = lastCapture;
	board.lastPawn = lastPawn;
	board.halfMove = halfMove;
	board.flags = flags;
	board.whiteMask = whiteMask;
	board.blackMask = blackMask;
	board.pieceMask = whiteMask | blackMask;
	return FEN_CODEC_SUCCESSFUL;
}

int Codec::FEN::import_fen(Chessboard& board, std::string str) {
	int temp;
	return Codec::FEN::import_fen(board, str, temp);
}

std::string Codec::FEN::export_fen(Chessboard& board) {
	std::stringstream ss;

	// "rnbqkbnr/pppppppp/pppppppp/pppppppp/PPPPPPPP/PPPPPPPP/PPPPPPPP/RNBQKBNR w KQkq e4 2147483647 2147483647"
	for (int rank = 7; rank >= 0; rank--) {
		int empty = 0;
		for (int file = 0; file < 8; file++) {
			int idx = file | (rank << 3);

			char c = Serial::get_piece_character(board.pieces[idx]);
			if (c == 0) {
				empty++;
				continue;
			}

			if (empty > 0) {
				ss << (char)('0' + empty);
				empty = 0;
			}

			ss << c;
		}

		if (empty > 0) {
			ss << (char)('0' + empty);
			empty = 0;
		}

		ss << (char)((rank == 0) ? ' ' : '/');
	}

	ss << (((board.halfMove & 1) == 0) ? 'w' : 'b') << ' ';

	if ((board.flags & CastlingFlags::ANY_CASTLE_ANY) == 0) {
		ss << '-';
	} else {
		if ((board.flags & CastlingFlags::WHITE_CASTLE_K) != 0) ss << 'K';
		if ((board.flags & CastlingFlags::WHITE_CASTLE_Q) != 0) ss << 'Q';
		if ((board.flags & CastlingFlags::BLACK_CASTLE_K) != 0) ss << 'k';
		if ((board.flags & CastlingFlags::BLACK_CASTLE_Q) != 0) ss << 'q';
	}
	ss << ' ';

	if (board.lastPawn == 0) {
		ss << '-';
	} else {
		int square = board.lastPawn;
		ss << (char)('a' + (square & 7))
		   << (char)('1' + ((square >> 3) & 7));
	}
	ss << ' ' << board.lastCapture << ' ' << (board.halfMove / 2);

	return ss.str();
}
