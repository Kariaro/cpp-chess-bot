#pragma once

#ifndef FEN_CODEC_H
#define FEN_CODEC_H

#include "../../src/utils.h"
#include <string>

constexpr int FEN_CODEC_SUCCESSFUL        = 0;
constexpr int FEN_CODEC_INVALID_FILE      = 1;
constexpr int FEN_CODEC_INVALID_CHARACTER = 2;
constexpr int FEN_CODEC_INVALID_FILE_OOB  = 3;
constexpr int FEN_CODEC_INVALID_RANK_OOB  = 4;
constexpr int FEN_CODEC_INVALID_BOARD     = 5;
constexpr int FEN_CODEC_INVALID_TURN      = 6;
constexpr int FEN_CODEC_INVALID_FORMAT    = 7;

namespace Codec::FEN {
	/// Import a fen string
	extern int import_fen(Chessboard& board, std::string str, int& matched_chars);

	/// Export a board to fen
	extern std::string export_fen(Chessboard& board);
}

#endif // !FEN_CODEC_H