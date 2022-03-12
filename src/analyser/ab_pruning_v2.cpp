#include "ab_pruning_v2.h"

constexpr double NEGATIVE_INFINITY = -1000000000.0;
constexpr double POSITIVE_INFINITY =  1000000000.0;
constexpr double MATE_MULTIPLIER   = 10000;
constexpr int QUIESCE_DEPTH = 6;

#define max(a, b) (a > b ? a : b)
#define min(a, b) (a < b ? a : b)

int an_get_material(Chessboard& a_board) {
	uint64_t mask = a_board.pieceMask;
	int material = 0;

	if (Board::hasFlags(a_board, CastlingFlags::BLACK_CASTLE_K)) material -= 30;
	if (Board::hasFlags(a_board, CastlingFlags::BLACK_CASTLE_Q)) material -= 30;
	if (Board::hasFlags(a_board, CastlingFlags::WHITE_CASTLE_K)) material += 30;
	if (Board::hasFlags(a_board, CastlingFlags::WHITE_CASTLE_Q)) material += 30;

	while (mask != 0) {
		uint64_t pick = Utils::lowestOneBit(mask);
		mask &= ~pick;
		uint32_t idx = (uint32_t)Utils::numberOfTrailingZeros(pick);
		int pic = a_board.pieces[idx];
		int val = Serial::get_piece_value(pic);

		if (pic == Pieces::W_PAWN) val += (idx >> 3) * 10;
		if (pic == Pieces::B_PAWN) val -= (8 - (idx >> 3)) * 10;
		material += val;
	}

	return material;
}

int an_non_developing(Chessboard& a_board) {
	int result = 0;
	if (a_board.pieces[1] == Pieces::W_KNIGHT) result -= 10;
	if (a_board.pieces[2] == Pieces::W_BISHOP) result -= 10;
	if (a_board.pieces[5] == Pieces::W_BISHOP) result -= 10;
	if (a_board.pieces[6] == Pieces::W_KNIGHT) result -= 10;
	if (a_board.pieces[11] == Pieces::W_PAWN) result -= 11;
	if (a_board.pieces[12] == Pieces::W_PAWN) result -= 11;
	if (a_board.pieces[4] == Pieces::W_KING) result -= 8;

	if (a_board.pieces[57] == Pieces::B_KNIGHT) result += 10;
	if (a_board.pieces[58] == Pieces::B_BISHOP) result += 10;
	if (a_board.pieces[61] == Pieces::B_BISHOP) result += 10;
	if (a_board.pieces[62] == Pieces::B_KNIGHT) result += 10;
	if (a_board.pieces[51] == Pieces::B_PAWN) result += 11;
	if (a_board.pieces[52] == Pieces::B_PAWN) result += 11;
	if (a_board.pieces[60] == Pieces::B_KING) result += 8;

	return result * 3;
}

int an_un_developing(Chessboard& a_board, Move& a_move) {
	int id = a_board.pieces[a_move.from];
	int move_to = a_move.to;
	int result = 0;

	switch (id) {
		case Pieces::W_KNIGHT: {
			if (move_to == 1 || move_to == 6) result -= 10;
			break;
		}
		case Pieces::B_KNIGHT: {
			if (move_to == 57 || move_to == 62) result += 10;
			break;
		}
		case Pieces::W_BISHOP: {
			if (move_to == 2 || move_to == 5) result -= 10;
			break;
		}
		case Pieces::B_BISHOP: {
			if (move_to == 58 || move_to == 61) result += 10;
			break;
		}
		case Pieces::W_QUEEN: result -= 5; break;
		case Pieces::B_QUEEN: result += 5; break;
		case Pieces::W_KING: result -= 15; break;
		case Pieces::B_KING: result += 15; break;
	}

	return result * 3;
}

double an_get_advanced_material(Chessboard& a_board, Move& a_lastMove) {
	double material = an_get_material(a_board);
	material += an_un_developing(a_board, a_lastMove);
	material += an_non_developing(a_board);
	return material;
}

void an_update_moves(Move& move, BranchResult& result, BranchResult& branch) {
	result.m_num_moves = branch.m_num_moves + 1;
	result.m_moves[0] = move;
	memcpy(result.m_moves + 1, branch.m_moves, sizeof(Move) * branch.m_num_moves);
}

template<bool White>
double an_quiesce(Chessboard& a_parent, Move a_lastMove, int depth, double alpha, double beta) {
	double evaluation = an_get_advanced_material(a_parent, a_lastMove);
	if (depth == 0) {
		return evaluation;
	}
	
	if constexpr (White) {
		if (evaluation >= beta) {
			return beta;
		}
		
		if (evaluation > alpha) {
			alpha = evaluation;
		}
	} else {
		if (evaluation <= alpha) {
			return alpha;
		}
		
		if (evaluation < beta) {
			beta = evaluation;
		}
	}
	
	Chessboard board = a_parent;
	std::vector<Move> moves;// = Generator::generate_valid_quiesce_moves(board);
	moves.reserve(96);
	Generator::_Generate_valid_quiesce_moves<White>(moves, board);

	double value = evaluation;
	for (Move move : moves) {
		if (!Generator::playMove(board, move)) {
			// This should never happen
			continue;
		}

		double score = an_quiesce<!White>(board, move, depth - 1, alpha, beta);
		if constexpr (White) {
			if (score >= beta) {
				return beta;
			}
			
			if (score > alpha) {
				alpha = score;
				value = score;
			}
		} else {
			if (score <= alpha) {
				return alpha;
			}
			
			if (score < beta) {
				beta = score;
				value = score;
			}
		}
		
		board = a_parent;
	}
	
	return value;
}

bool ABPruningV2::should_stop() {
	if (!m_stop && m_max_time != 0) {
		using namespace std::chrono;
		int64_t time = (int64_t)(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() - m_start_time);
		// fprintf(stderr, "time=%lld max=%lld\n", time, m_max_time);

		if (time > m_max_time) {
			m_stop = true;
			return true;
		}
	}

	return m_stop;
}

uint64_t an_total_nodes;
uint64_t an_nodes;

template <bool White>
BranchResult ABPruningV2::analyse_branches(Chessboard& a_parent, Move& a_lastMove, int depth, double alpha, double beta) {
	an_total_nodes++;
	an_nodes++;
	// Branch zero should always evaluate
	if (depth == 0) {
		return BranchResult{ an_quiesce<White>(a_parent, a_lastMove, QUIESCE_DEPTH, alpha, beta) };
	}

	if (should_stop()) {
		return BranchResult{};
	}
	
	// Default state of the board
	Chessboard board = a_parent;
	//std::vector<Move> moves = Generator::generate_valid_moves(board);
	std::vector<Move> moves;
	moves.reserve(96);
	Generator::_Generate_valid_moves<White>(moves, board);
	double value;
	
	BranchResult result{};
	
	int countMoves = 0;
	value = (MATE_MULTIPLIER * (depth + 1)) * (White ? -1 : 1);
	for (Move move : moves) {
		if (!Generator::playMove(board, move)) {
			continue;
		}
		
		countMoves++;
		
		BranchResult scannedResult = analyse_branches<!White>(board, move, depth - 1, alpha, beta);
		if constexpr (White) {
			if (value < scannedResult.m_value) {
				an_update_moves(move, result, scannedResult);
				value = scannedResult.m_value;
			}
			
			if (value >= beta) {
				break;
			}
			
			alpha = alpha > value ? alpha : value;
		} else {
			if (value > scannedResult.m_value) {
				an_update_moves(move, result, scannedResult);
				value = scannedResult.m_value;
			}
			
			if (value <= alpha) {
				break;
			}
			
			beta = beta < value ? beta : value;
		}
		
		board = a_parent;
	}

	result.m_value = value;
	board = a_parent;
	
	if (countMoves == 0) {
		// There are no moves therefore stalemate
		// Check if the king is in check
		
		if (PieceManager::_Is_king_attacked<White>(board)) {
			// Checkmate
			result.m_value = (MATE_MULTIPLIER * (depth + 1)) * (White ? -1 : 1);
		} else {
			// Stalemate
			result.m_value = 0;
		}
	}

	board = a_parent;
	return result;
}

void an_evaluate(Chessboard& board, Scanner& scan) {
	bool isWhite = Board::isWhite(board);
	if (PieceManager::isKingAttacked(board, isWhite)) {
		double delta = isWhite ? -1 : 1;
		scan.base += 10 * delta;
		
		if (!scan.best.valid) {
			// Checkmate
			scan.base = MATE_MULTIPLIER * delta;
		}
	} else {
		if (!scan.best.valid) {
			// Stalemate
			scan.base = 0;
		}
	}
	
	if (board.lastCapture >= 50) {
		// The game should end
		scan.best.valid = false;
	}
}

Scanner ABPruningV2::analyse_branch_moves(Chessboard& a_parent, int a_depth) {
	bool is_parent_white = Board::isWhite(a_parent);

	Scanner scan { };
	scan.bestMaterial = is_parent_white ? NEGATIVE_INFINITY : POSITIVE_INFINITY;
	scan.base = an_get_material(a_parent);
	scan.best.valid = false;
	
	Chessboard board = a_parent;
	std::vector<Move> moves = Generator::generate_valid_moves(a_parent);
	for (Move move : moves) {
		if (!Generator::playMove(board, move)) {
			continue;
		}

		// Make sure atleast one move is valid
		if (!scan.best.valid) {
			scan.best.valid = true;
			scan.best = move;
			scan.m_branch_result.m_num_moves = 0;
		}

		using namespace std::chrono;
		auto start = high_resolution_clock::now();
		an_nodes = 0;
		BranchResult branchResult = is_parent_white
			? analyse_branches<BLACK>(board, move, a_depth, NEGATIVE_INFINITY, POSITIVE_INFINITY)
			: analyse_branches<WHITE>(board, move, a_depth, NEGATIVE_INFINITY, POSITIVE_INFINITY);

		if (m_stop && a_depth > 0) {
			break;
		}

		auto finish = high_resolution_clock::now();
		double scannedResult = branchResult.m_value;
		
		{
			fprintf(stderr, "move: %s (%.2f), [", Serial::get_move_string(move).c_str(), scannedResult / 100.0);

			for (int i = 0; i < branchResult.m_num_moves; i++) {
				if (i > 0) {
					fprintf(stderr, ", ");
				}

				fprintf(stderr, "%s", Serial::get_move_string(branchResult.m_moves[i]).c_str());
			}
			auto timeTook = duration_cast<nanoseconds>(finish-start).count();
			fprintf(stderr, "]\t %lld nodes / sec\n", (int64_t)(an_nodes / (timeTook / 1000000000.0)));
		}

		if (is_parent_white) {
			if (scan.bestMaterial < scannedResult) {
				scan.best = move;
				scan.bestMaterial = scannedResult;
				scan.m_branch_result = branchResult;
			}
		} else {
			if (scan.bestMaterial > scannedResult) {
				scan.best = move;
				scan.bestMaterial = scannedResult;
				scan.m_branch_result = branchResult;
			}
		}

		board = a_parent;
	}
	
	an_evaluate(a_parent, scan);
	return scan;
}


ABPruningV2::ABPruningV2() {
	m_options.insert(m_options.end(), {
		new UciOption::Spin("Skill Level", 0, 20, 20),
		new UciOption::Spin("Move Overhead", 1, 4096, 1),
		new UciOption::Spin("Threads", 1, 512, 1),
		new UciOption::Spin("Hash", 1, 4096, 256),
		new UciOption::String("String", "Testing this tool"),
		new UciOption::Button("btn"),
		new UciOption::Combo("Combo", { "Alpha", "Beta", "Gamma", "Delta" }, 0),

		/*
		new UciOption::String("NalimovPath", ""),
		new UciOption::Spin("NalimovCache", 0, 10, 10),
		new UciOption::Check("Ponder", false),
		new UciOption::Check("OwnBook", false),
		new UciOption::Spin("MultiPV", 1, 1, 1),
		new UciOption::Check("UCI_ShowCurrLine", false),
		new UciOption::Check("UCI_ShowRefutations", false),
		new UciOption::Check("UCI_LimitStrength", false),
		new UciOption::Spin("UCI_Elo", 100, 5000, 1500),
		new UciOption::Check("UCI_AnalyseMode", false),
		new UciOption::String("UCI_Opponent", ""),
		*/
	});
}

void ABPruningV2::on_option_change(UciOption* option) {
	// fprintf(stderr, "into string option change: %s\n", option->get_key());
}

void ABPruningV2::thread_loop(ChessAnalysis* a_analysis) {
	m_stop = false;

	using namespace std::chrono;

	auto start_time = system_clock::now();
	m_start_time = duration_cast<milliseconds>(start_time.time_since_epoch()).count();
	m_max_time = a_analysis->m_max_time;
	a_analysis->bestmove = { 0, 0, 0, false };

	Move best_move{};
	int64_t total_time = 0;

	int mul = (Board::isWhite(a_analysis->board) ? 1 : -1);
	for (int i = 0; i < DEPTH && !m_stop; i++) {
		an_total_nodes = 0;
		start_time = system_clock::now();

		Scanner scanner = analyse_branch_moves(a_analysis->board, i);
		int64_t millis = duration_cast<milliseconds>(system_clock::now() - start_time).count();
		if (i == 0 || !m_stop) {
			int score = (int)(scanner.bestMaterial);
			int mate = (int)(score / MATE_MULTIPLIER);

			std::stringstream pv_stream;
			pv_stream << Serial::get_move_string(scanner.best);
			for (int j = 0; j < scanner.m_branch_result.m_num_moves; j++) {
				pv_stream << " " << Serial::get_move_string(scanner.m_branch_result.m_moves[j]);
			}

			std::stringstream sc_stream;
			if (mate != 0) {
				// odd moves we are getting mated
				// even moves we are checkmating the opponent
				int dir = 1 - ((scanner.m_branch_result.m_num_moves & 1) * 2);
				sc_stream << "mate " << (dir * (((i - (mate < 0 ? -mate : mate)) / 2) + 1));
			} else {
				sc_stream << "cp " << mul * score;
			}

			printf("info depth %d time %lld nodes %lld nps %lld score %s pv %s\n",
				i + 1,
				millis,
				an_total_nodes,
				(an_total_nodes * 1000ull) / (millis + 1),
				sc_stream.str().c_str(),
				pv_stream.str().c_str()
			);
			fflush(stdout);

			a_analysis->bestmove = scanner.best;
			best_move = scanner.best;
		}

		if (total_time + millis * 4 > m_max_time)
		{
			// Check if we have enough time 
			break;
		}

		total_time += millis;
	}

	// Print the best move value for the engine
	printf("bestmove %s\n", Serial::get_move_string(best_move).c_str());
	fflush(stdout);
	
	m_stop = true;
}

bool ABPruningV2::stop_analysis() {
	if (m_stop) {
		fprintf(stderr, "Thread has already been closed!\n");
		return false;
	}

	m_stop = true;
	if (m_thread.joinable()) {
		m_thread.join();
	}

	return true;
}

bool ABPruningV2::start_analysis(ChessAnalysis& a_analysis) {
	if (!m_stop) {
		fprintf(stderr, "Thread has already been started!\n");
		return false;
	}

	m_stop = false;
	m_thread = std::thread(&ABPruningV2::thread_loop, this, &a_analysis);
	m_thread.detach();
	return true;
}
