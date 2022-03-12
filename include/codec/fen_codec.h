#ifndef FEN_CODEC_H
#define FEN_CODEC_H

#include <string>
#include "../../src/utils.h"

constexpr int FEN_CODEC_SUCCESSFUL        = 0;
constexpr int FEN_CODEC_INVALID_FILE      = 1;
constexpr int FEN_CODEC_INVALID_CHARACTER = 2;
constexpr int FEN_CODEC_INVALID_FILE_OOB  = 3;
constexpr int FEN_CODEC_INVALID_RANK_OOB  = 4;
constexpr int FEN_CODEC_INVALID_BOARD     = 5;
constexpr int FEN_CODEC_INVALID_TURN      = 6;
constexpr int FEN_CODEC_INVALID_FORMAT    = 7;

namespace Codec::FEN {
	int import_fen(Chessboard& board, std::string str, int& matched_chars);
	int import_fen(Chessboard& board, std::string str);
	std::string export_fen(Chessboard& board);
}

#endif // FEN_CODEC_H
