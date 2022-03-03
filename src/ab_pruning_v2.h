#pragma once

#ifndef AB_PRUNING_V2_H
#define AB_PRUNING_V2_H

#include <chrono>
#include "../include/analyser/chess_analyser.h"
#include "generator.h"
#include "chessboard.h"
#include "pieces.h"
#include "serial.h"
#include "piece_manager.h"

constexpr int DEPTH = 7;

struct BranchResult {
	double value;
	int numMoves;
	Move moves[DEPTH];
};

struct Scanner {
	bool white;
	bool draw;
	double base;
	Move best;
	double bestMaterial;
	BranchResult m_branch_result;
};

class ABPruningV2 : public ChessAnalyser {
public:
	ABPruningV2();

	virtual bool stop_analysis();
	virtual bool start_analysis(ChessAnalysis& analysis);

protected:
	virtual void on_option_change(UciOption* option);

private:
	void thread_loop(ChessAnalysis* a_analysis);
	bool should_stop();
	template <bool WHITE>
	BranchResult analyse_branches(Chessboard& a_parent, Move& a_lastMove, int a_depth, double a_alpha, double a_beta);
	Scanner analyse_branch_moves(Chessboard& a_parent, int a_depth);

	uint64_t m_start_time{};
	uint64_t m_max_time{};
};

#undef DEPTH

#endif // !AB_PRUNING_V2_H
