#include <stdio.h>
#include <chrono>
#include <iostream>

#include "serial.h"
#include "generator.h"
#include "analyser/ab_pruning_v2.h"
#include "uci/uci_manager.h"

int main(int argc, char** argv) {
	ChessAnalyser* analyser = new ABPruningV2();
	UciManager manager("HardCoded", "HardCodedBot 1.0", analyser);

	// Test perft
	//Chessboard board;
	//Codec::FEN::import_fen(board, "r2qkb1r/1Q3pp1/pN1p3p/3P1P2/3pP3/4n3/PP4PP/1R3RK1 b - - 0 0");
	//Codec::FEN::import_fen(board, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 0");

	// 872389934
	manager.process_command("position fen r2qkb1r/1Q3pp1/pN1p3p/3P1P2/3pP3/4n3/PP4PP/1R3RK1 b - - 0 0");
	//manager.process_command("position fen rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 0");
	manager.process_command("go perft 6");

	// stockfish   : 222400965 moves/sec
	// hardcodedbot:  14399941 moves/sec

	// This will read from cin and only exit when the 'quit' command is called
	manager.run();

	/*
	//int result = Codec::FEN::import_fen(board, "r6r/pp1k1p1p/4pq2/2ppnn2/1b3Q2/2N1P2N/PPPP1PPP/R1B1K2R w KQ - 0 12");
	int result = Codec::FEN::import_fen(board, "r2qkb1r/1Q3pp1/pN1p3p/3P1P2/3pP3/4n3/PP4PP/1R3RK1 b - - 0 0");
	// int result = Codec::FEN::import_fen(board, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 0");
	printf("FEN: [%s]\n", Codec::FEN::export_fen(board).c_str());

	printf("result: %d\n", result);
	printf("Board:\n%s\n", Serial::get_board_string(board).c_str());
	printf("lastCapture: %d\n", board.lastCapture);
	printf("lastPawn: %d\n", board.lastPawn);
	printf("halfMove: %d\n", board.halfMove);
	printf("whiteMask: %lld\n", board.whiteMask);
	printf("blackMask: %lld\n", board.blackMask);
	printf("pieceMask: %lld\n", board.pieceMask);
	printf("\n");

	/*
	for (Move move : Generator::generateValidMoves(board)) {
		char* chars = Serial::getMoveString(move.piece, move.from, move.to, move.special);

		printf("move: [%s]\n", chars);
		free(chars);
	}
	*/

	return 0;
}