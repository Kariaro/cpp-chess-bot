#include "../ab_pruning_v2.h"
#include "../piece_manager.h"
#include <chrono>
#include <thread>
#include <iostream>

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
	result.numMoves = branch.numMoves + 1;
	result.moves[0] = move;
	memcpy(result.moves + 1, branch.moves, sizeof(Move) * branch.numMoves);
}

template<bool WHITE>
double an_quiesce(Chessboard& a_parent, Move a_lastMove, int depth, double alpha, double beta) {
	double evaluation = an_get_advanced_material(a_parent, a_lastMove);
	if (depth == 0) {
		return evaluation;
	}
	
	if constexpr (WHITE) {
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
	std::vector<Move> moves = Generator::generate_valid_quiesce_moves(board);

	double value = evaluation;
	for (Move move : moves) {
		if (!Generator::playMove(board, move)) {
			// This should never happen
			continue;
		}

		double score = an_quiesce<!WHITE>(board, move, depth - 1, alpha, beta);
		if constexpr (WHITE) {
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
		// std::cerr << "time=" << time << " max=" << m_max_time << std::endl;

		if (time > m_max_time) {
			m_stop = true;
			return true;
		}
	}

	return m_stop;
}

long an_total_nodes;
long an_nodes;
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
	std::vector<Move> moves = Generator::generate_valid_moves(board);
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
			if (value < scannedResult.value) {
				an_update_moves(move, result, scannedResult);
				value = scannedResult.value;
			}
			
			if (value >= beta) {
				break;
			}
			
			alpha = alpha > value ? alpha : value;
		} else {
			if (value > scannedResult.value) {
				an_update_moves(move, result, scannedResult);
				value = scannedResult.value;
			}
			
			if (value <= alpha) {
				break;
			}
			
			beta = beta < value ? beta : value;
		}
		
		board = a_parent;
	}

	result.value = value;
	board = a_parent;
	
	if (countMoves == 0) {
		// There are no moves therefore stalemate
		// Check if the king is in check
		
		if (PieceManager::_Is_king_attacked<White>(board)) {
			// Checkmate
			result.value = (MATE_MULTIPLIER * (depth + 1)) * (White ? -1 : 1);
		} else {
			// Stalemate
			result.value = 0;
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
			scan.m_branch_result.numMoves = 0;
		}

		auto start = std::chrono::high_resolution_clock::now();
		an_nodes = 0;
		BranchResult branchResult = is_parent_white
			? analyse_branches<false>(board, move, a_depth, NEGATIVE_INFINITY, POSITIVE_INFINITY)
			: analyse_branches<true>(board, move, a_depth, NEGATIVE_INFINITY, POSITIVE_INFINITY);

		if (m_stop && a_depth > 0) {
			break;
		}

		auto finish = std::chrono::high_resolution_clock::now();
		double scannedResult = branchResult.value;
		
		{
			std::cerr << "move: " << Serial::get_move_string(move) << "(" << scannedResult / 100.0 << "), [";

			for (int i = 0; i < branchResult.numMoves; i++) {
				if (i > 0) {
					std::cerr << ", ";
				}

				Move m = branchResult.moves[i];
				std::cerr << Serial::get_move_string(m);
			}
			auto timeTook = std::chrono::duration_cast<std::chrono::nanoseconds>(finish-start).count();
			std::cerr << "]\t" << (long)(an_nodes / (timeTook / 1000000000.0)) << " nodes / sec" << std::endl;
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
	// std::cerr << "info string option change: " << option->get_key() << std::endl;
}

void ABPruningV2::thread_loop(ChessAnalysis* a_analysis) {
	m_stop = false;

	using namespace std::chrono;

	m_start_time = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	m_max_time = a_analysis->m_max_time;
	a_analysis->bestmove = { 0, 0, 0, false };

	// TODO: This engine does not want to do checkmates.... FIX THIS
	Move best_move{};
	for (int i = 0; i < DEPTH && !m_stop; i++) {
		an_total_nodes = 0;
		Scanner scanner = analyse_branch_moves(a_analysis->board, i);
		if (i == 0 || !m_stop) {
			int score = (int)(scanner.bestMaterial);
			int mate = (int)(score / MATE_MULTIPLIER);

			std::cout << "info depth " << (i + 1) << " nodes " << an_total_nodes << " score ";
			
			if (mate != 0) {
				std::cout << "mate " << (DEPTH - (mate < 0 ? -mate : mate) + 1);
			} else {
				std::cout << "cp " << score;
			}
			
			std::cout << " pv " << Serial::get_move_string(scanner.best);
			for (int j = 0; j < scanner.m_branch_result.numMoves; j++) {
				std::cout << " " << Serial::get_move_string(scanner.m_branch_result.moves[j]);
			}
			std::cout << std::endl;

			a_analysis->bestmove = scanner.best;
			best_move = scanner.best;
		}
	}

	// Print the best move value for the engine
	std::cout << "bestmove " << Serial::get_move_string(best_move) << std::endl;
	m_stop = true;
}

bool ABPruningV2::stop_analysis() {
	if (m_stop) {
		std::cerr << "Thread has already been closed!" << std::endl;
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
		std::cerr << "Thread has already been started!" << std::endl;
		return false;
	}

	m_stop = false;
	m_thread = std::thread(&ABPruningV2::thread_loop, this, &a_analysis);
	m_thread.detach();
	return true;
}
