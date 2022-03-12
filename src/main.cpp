#include <stdio.h>
#include <chrono>
#include <iostream>

#include "serial.h"
#include "generator.h"
#include "analyser/ab_pruning_v2.h"
#include "uci/uci_manager.h"
#include "codec/fen_codec.h"

template <bool White>
long goDepth(Chessboard& a_parent, int a_depth) {
	if (a_depth < 2) {
		return 1;
	}

	Chessboard board = a_parent;

	std::vector<Move> moves;
	moves.reserve(96);
	Generator::_Generate_valid_moves<White>(moves, board);

	long count = 0;
	for (Move move : moves) {
		if (!Generator::playMove(board, move)) {
			continue;
		}

		count += goDepth<!White>(board, a_depth - 1);
		// Generator::_Undo_move<White>(board, move);
		board = a_parent;
	}

	return count;
}

template <bool White>
long computePerft(Chessboard& parent, int depth) {
	Chessboard board = parent;

	printf("Checking depth = %d\n\n", depth);
	auto start = std::chrono::high_resolution_clock::now();
	long totalCount = 0;
	long count;

	std::vector<Move> moves;
	moves.reserve(96);
	Generator::_Generate_valid_moves<White>(moves, board);

	for (Move move : moves) {
		if (!Generator::playMove(board, move)) {
			continue;
		}

		count = goDepth<!White>(board, depth);

		printf("%s: %d\n", Serial::get_move_string(move).c_str(), count);

		totalCount += count;
		board = parent;
		//Generator::_Undo_move<White>(board, move);
	}

	auto finish = std::chrono::high_resolution_clock::now();
	auto timeTook = std::chrono::duration_cast<std::chrono::nanoseconds>(finish-start).count();

	printf("\nTotal moves: %d\n", totalCount);
	printf("Moves: %d / sec\n", (long)(totalCount / (timeTook / 1000000000.0)));
	printf("Time: %.2f / sec\n", (timeTook / 1000000000.0));
	
	return totalCount;
}

long computePerft(Chessboard& parent, int depth) {
	return Board::isWhite(parent)
		? computePerft<WHITE>(parent, depth)
		: computePerft<BLACK>(parent, depth);
}

int main(int argc, char** argv) {
	ChessAnalyser* analyser = new ABPruningV2();
	UciManager manager("HardCoded", "HardCodedBot 1.0", analyser);

	Chessboard board;

	// Test perft
	//Codec::FEN::import_fen(board, "r2qkb1r/1Q3pp1/pN1p3p/3P1P2/3pP3/4n3/PP4PP/1R3RK1 b - - 0 0");
	Codec::FEN::import_fen(board, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 0");

	// 872389934
	computePerft(board, 6);

	// This will read from cin and only exit when the 'quit' command is called
	manager.run();

	/*
	//int result = Codec::FEN::import_fen(board, "r6r/pp1k1p1p/4pq2/2ppnn2/1b3Q2/2N1P2N/PPPP1PPP/R1B1K2R w KQ - 0 12");
	int result = Codec::FEN::import_fen(board, "r2qkb1r/1Q3pp1/pN1p3p/3P1P2/3pP3/4n3/PP4PP/1R3RK1 b - - 0 0");
	// int result = Codec::FEN::import_fen(board, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 0");
	printf("FEN: [%s]\n", Codec::FEN::export_fen(board).c_str());

	char* chars = Serial::getBoardString(&board);
	printf("result: %d\n", result);
	printf("Board:\n%s\n", chars);
	printf("lastCapture: %d\n", board.lastCapture);
	printf("lastPawn: %d\n", board.lastPawn);
	printf("halfMove: %d\n", board.halfMove);
	printf("whiteMask: %lld\n", board.whiteMask);
	printf("blackMask: %lld\n", board.blackMask);
	printf("pieceMask: %lld\n", board.pieceMask);
	printf("\n");
	free(chars);

	// computePerft(board, 6);

	/*
	for (Move move : Generator::generateValidMoves(board)) {
		char* chars = Serial::getMoveString(move.piece, move.from, move.to, move.special);

		printf("move: [%s]\n", chars);
		free(chars);
	}
	*/

	return 0;
}