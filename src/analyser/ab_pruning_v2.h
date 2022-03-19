#ifndef AB_PRUNING_V2_H
#define AB_PRUNING_V2_H

#include <chrono>
#include <thread>
#include <iostream>
#include <sstream>
#include "chess_analyser.h"
#include "../generator.h"
#include "../chessboard.h"
#include "../pieces.h"
#include "../serial.h"
#include "../piece_manager.h"

constexpr int DEPTH = 7;

struct BranchResult {
	double m_value;
	int m_num_moves;
	Move m_moves[DEPTH];
};

struct Scanner {
	bool white;
	bool draw;
	double base;
	Move best{};
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
	void thread_loop(ChessAnalysis* analysis);
	bool should_stop();

	template <bool White>
	BranchResult analyse_branches(Chessboard& parent, const Move lastMove, int depth, double alpha, double beta);
	Scanner analyse_branch_moves(Chessboard& parent, int depth);

	uint64_t m_start_time{};
	uint32_t m_max_time{};
};

#undef DEPTH

#endif // AB_PRUNING_V2_H
