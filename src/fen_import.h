#pragma once

#ifndef FEN_IMPORT_H
#define FEN_IMPORT_H

#include "utils_type.h"

#define FEN_IMPORT_SUCCESSFULL 0
#define FEN_IMPORT_INVALID_FILE 1
#define FEN_IMPORT_INVALID_CHARACTER 2
#define FEN_IMPORT_INVALID_FILE_OOB 3
#define FEN_IMPORT_INVALID_RANK_OOB 4
#define FEN_IMPORT_INVALID_BOARD 5
#define FEN_IMPORT_INVALID_TURN 6
#define FEN_IMPORT_INVALID_FORMAT 7

extern int import_fen_old(Chessboard* board, const char* chars);

extern char* export_fen_old(Chessboard* board);

#endif // !FEN_IMPORT_H

