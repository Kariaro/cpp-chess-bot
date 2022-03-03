#include <stdio.h>
#include <stdlib.h>
#include "pieces.h"
#include "serial.h"
#include "fen_import.h"

int _readNumber(const char** chars) {
	int value = atoi(*chars);

	int loop = value;
	do {
		loop /= 10;
		(*chars)++;
	} while (loop != 0);

	return value;
}

int _readSquare(const char* chars) {
	char file = *(chars + 0);
	char rank = *(chars + 1);

	int idx = 0;
	if (file >= 'a' && file <= 'h') {
		idx = 'h' - file;
	} else {
		return -1;
	}

	if (rank >= '1' && rank <= '8') {
		idx += (rank - '1') << 3;
	} else {
		return -1;
	}

	return idx;
}

int import_fen_old(struct Chessboard* board, const char* chars) {
	int8_t* pieces = board->pieces;
	int lastCapture;
	int lastPawn;
	int halfMove;
	int flags;
	
	{
		int rank = 0;
		int file = 0;
		for (;;) {
			char c = *(chars++);
			if (c == '/') {
				if (file != 8) {
					return FEN_IMPORT_INVALID_FILE;
				}

				if (++rank > 7) {
					return FEN_IMPORT_INVALID_RANK_OOB;
				}

				file = 0;

				continue;
			}

			if (c == ' ') {
				if (file != 8 && rank != 7) {
					return FEN_IMPORT_INVALID_BOARD;
				}
				
				break;
			}

			uint32_t idx = (file) + ((7 - rank) << 3);
			switch (c) {
				case 'r': case 'n': case 'b': case 'q': case 'k': case 'p':
				case 'R': case 'N': case 'B': case 'Q': case 'K': case 'P': {
					pieces[(++file, idx)] = Serial::get_piece_from_character(c);
					break;
				}

				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8': {
					file += c - '0';
					break;
				}

				default:
					return FEN_IMPORT_INVALID_CHARACTER;
			}

			if (file > 8) {
				return FEN_IMPORT_INVALID_FILE_OOB;
			}
		}
	}

	// Read turn
	{
		char c = *(chars++);

		if (c == 'w' || c == 'b') {
			halfMove = (c == 'w') ? 0 : 1;
		} else {
			return FEN_IMPORT_INVALID_TURN;
		}
		
		if (*(chars++) != ' ') {
			return FEN_IMPORT_INVALID_FORMAT;
		}
	}

	// Read flags
	{
		flags = 0;
		if (*chars != '-') {
			if (*chars == 'K') {
				flags |= CastlingFlags::WHITE_CASTLE_K;
				chars++;
			}

			if (*chars == 'Q') {
				flags |= CastlingFlags::WHITE_CASTLE_Q;
				chars++;
			}

			if (*chars == 'k') {
				flags |= CastlingFlags::BLACK_CASTLE_K;
				chars++;
			}

			if (*chars == 'q') {
				flags |= CastlingFlags::BLACK_CASTLE_Q;
				chars++;
			}
		} else {
			chars++;
		}

		if (*(chars++) != ' ') {
			return FEN_IMPORT_INVALID_FORMAT;
		}
	}

	// En passant
	{
		lastPawn = 0;
		if (*chars != '-') {
			lastPawn = _readSquare(chars);

			if (lastPawn < 0) {
				return FEN_IMPORT_INVALID_FORMAT;
			}

			chars += 2;
		} else {
			chars++;
		}

		if (*(chars++) != ' ') {
			return FEN_IMPORT_INVALID_FORMAT;
		}
	}
	
	lastCapture = _readNumber(&chars);
	if (*(chars++) != ' ') {
		return FEN_IMPORT_INVALID_FORMAT;
	}
	
	halfMove = halfMove + 2 * _readNumber(&chars);

	uint64_t whiteMask = 0;
	uint64_t blackMask = 0;
	for (int i = 0; i < 64; i++) {
		int piece = pieces[i];

		if (piece < 0) {
			blackMask |= (uint64_t)(1) << i;
		}

		if (piece > 0) {
			whiteMask |= (uint64_t)(1) << i;
		}
	}
	
	board->lastCapture = lastCapture;
	board->lastPawn = lastPawn;
	board->halfMove = halfMove;
	board->flags = flags;
	board->whiteMask = whiteMask;
	board->blackMask = blackMask;
	board->pieceMask = whiteMask | blackMask;
	return FEN_IMPORT_SUCCESSFULL;
}

static void _writeNumber(char** ptr, int value) {
	value &= 0x7fffffff;
	int len = 0;
	int cpy = value;

	do {
		len ++;
		cpy /= 10;
	} while (cpy != 0);

	int len2 = len;
	do {
		*((*ptr) + (--len)) = '0' + (value % 10);
		value /= 10;
	} while(value != 0);
	(*ptr) += len2;
}

char* export_fen_old(Chessboard* board) {
	// "rnbqkbnr/pppppppp/pppppppp/pppppppp/PPPPPPPP/PPPPPPPP/PPPPPPPP/RNBQKBNR w KQkq e4 2147483647 2147483647"
	char* chars = (char*)calloc(107, sizeof(char));
	if (!chars) {
		return 0;
	}
	chars[106] = '\0';
	char* ptr = chars;
	{
		for (int rank = 7; rank >= 0; rank--) {
			int empty = 0;
			for (int file = 0; file < 8; file++) {
				int idx = file | (rank << 3);

				char c = Serial::get_piece_character(board->pieces[idx]);
				if (c == 0) {
					empty ++;
					continue;
				}

				if (empty > 0) {
					*(ptr++) = '0' + empty;
					empty = 0;
				}

				*(ptr++) = c;
			}
			
			if (empty > 0) {
				*(ptr++) = '0' + empty;
				empty = 0;
			}

			*(ptr++) = (rank == 0) ? ' ' : '/';
		}
	}

	*(ptr++) = ((board->halfMove & 1) == 0) ? 'w' : 'b';
	*(ptr++) = ' ';

	if ((board->flags & CastlingFlags::ANY_CASTLE_ANY) == 0) {
		*(ptr++) = '-';
	} else {
		if ((board->flags & CastlingFlags::WHITE_CASTLE_K) != 0) *(ptr++) = 'K';
		if ((board->flags & CastlingFlags::WHITE_CASTLE_Q) != 0) *(ptr++) = 'Q';
		if ((board->flags & CastlingFlags::BLACK_CASTLE_K) != 0) *(ptr++) = 'k';
		if ((board->flags & CastlingFlags::BLACK_CASTLE_Q) != 0) *(ptr++) = 'q';
	}
	*(ptr++) = ' ';

	if (board->lastPawn == 0) {
		*(ptr++) = '-';
	} else {
		int square = board->lastPawn;
		*(ptr++) = 'h' - (square & 7);
		*(ptr++) = '1' + ((square >> 3) & 7);
	}
	*(ptr++) = ' ';
	_writeNumber(&ptr, board->lastCapture);

	*(ptr++) = ' ';
	_writeNumber(&ptr, board->halfMove / 2);

	return chars;
}
