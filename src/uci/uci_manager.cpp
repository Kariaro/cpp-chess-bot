#include <iostream>

#include "uci_manager.h"
#include "../codec/fen_codec.h"
#include "../codec/str_codec.h"
#include "../generator.h"
#include "../serial.h"

void _Debug_options(ChessAnalyser* analyser) {
	for (UciOption* option : analyser->get_options()) {
		fprintf(stderr, "info string [%s] ", option->get_key().c_str());

		switch (option->get_type()) {
			case UciOptionType::CHECK: {
				UciOption::Check* opt = (UciOption::Check*)option;
				fprintf(stderr, "[check] = %s\n", opt->get_value() ? "true" : "false");
				break;
			}
			case UciOptionType::SPIN: {
				UciOption::Spin* opt = (UciOption::Spin*)option;
				fprintf(stderr, "[spin] = %lld\n", opt->get_value());
				break;
			}
			case UciOptionType::COMBO: {
				UciOption::Combo* opt = (UciOption::Combo*)option;
				fprintf(stderr, "[combo] = %s\n", opt->get_list()[opt->get_value()].c_str());
				break;
			}
			case UciOptionType::BUTTON: {
				fprintf(stderr, "[button]\n");
				break;
			}
			case UciOptionType::STRING: {
				UciOption::String* opt = (UciOption::String*)option;
				fprintf(stderr, "[string] = %s\n", opt->get_value().c_str());
				break;
			}
			default: {
				fprintf(stderr, "Undefined UciOptionType ( %d )\n", (int)option->get_type());
				break;
			}
		}

		option++;
	}
}

UciManager::UciManager(const std::string author, const std::string name, ChessAnalyser* analyser) : m_author(author), m_name(name), m_analyser(analyser) {
	m_running = true;
}

/*
bool UciManager::process_ponderhit(std::string command) {
	return false;
}
*/

bool UciManager::process_go(std::string command) {
	// http://wbec-ridderkerk.nl/html/UCIProtocol.html
	// TODO: Implement this method
	// TODO: Implement 'searchmoves'
	// TODO: Implement 'ponder'
	// TODO: Implement 'movestogo'

	// Remove 'go'
	command = command.substr(2);

	uint64_t wtime{0xffffffffffffffffull};
	uint64_t btime{0xffffffffffffffffull};
	uint64_t winc{};
	uint64_t binc{};
	bool infinite{};

	if (command._Starts_with(" infinite")) {
		infinite = true;
		command = command.substr(9);
	}

	if (command._Starts_with(" movetime ")) {
		command = command.substr(10);
		command = Codec::STR::read_integer<uint64_t>(command, wtime);
		btime = wtime;
	}

	if (command._Starts_with(" wtime ")) {
		command = command.substr(7);
		command = Codec::STR::read_integer<uint64_t>(command, wtime);
	}

	if (command._Starts_with(" btime ")) {
		command = command.substr(7);
		command = Codec::STR::read_integer<uint64_t>(command, btime);
	}

	if (command._Starts_with(" winc ")) {
		command = command.substr(6);
		command = Codec::STR::read_integer<uint64_t>(command, winc);
	}

	if (command._Starts_with(" binc ")) {
		command = command.substr(6);
		command = Codec::STR::read_integer<uint64_t>(command, binc);
	}

	if (!command.empty()) {
		fprintf(stderr, "Failed to fully parse the go command\n");
	}

	bool is_white = Board::isWhite(m_analysis.board);
	uint64_t turn_inc = is_white ? winc : binc;
	uint64_t turn_time = is_white ? wtime : btime;

	if (infinite) {
		m_analysis.m_max_time = (uint32_t)(100000000);
	} else {
		m_analysis.m_max_time = (uint32_t)(turn_time > 100000 ? 10000 : ((turn_time / 10) + 1));
	}
	m_analyser->start_analysis(m_analysis);

	return true;
}

bool UciManager::process_position_moves(std::string command) {
	// fprintf(stderr, "Remaining: [%s]\n", command.c_str());

	while (!command.empty()) {
		size_t first_space = command.find_first_of(' ');

		std::string move_str = (first_space == -1) ? command : command.substr(0, first_space);
		if (first_space != -1) {
			command = command.substr((size_t)first_space + 1ull);
		} else {
			command = "";
		}

		// fprintf(stderr, "info string [%s]\n", move_str.c_str());
		bool found = false;
		std::vector<Move> moves = Generator::generate_valid_moves(m_analysis.board);
		for (Move move : moves) {
			// fprintf(stderr, "info string [%s] <=> [%s]\n", move_str.c_str(), Serial::get_move_string(move).c_str());

			if (move_str == Serial::get_move_string(move)) {
				found = Generator::playMove(m_analysis.board, move);
				break;
			}
		}

		if (!found) {
			fprintf(stderr, "into string Could not play the move [%s]\n", move_str.c_str());
			return false;
		}
	}

	return true;
}

bool UciManager::process_position(std::string command) {
	if (command._Starts_with("position fen ")) {
		command = command.substr(13);
		int matched;
		if (Codec::FEN::import_fen(m_analysis.board, command, matched) != FEN_CODEC_SUCCESSFUL) {
			fprintf(stderr, "Invalid usage of 'position fen'. Invalid fen [%s]\n", command.c_str());
			return false;
		}

		command = command.substr(matched);
	} else if (command._Starts_with("position startpos")) {
		command = command.substr(17);
		int matched;
		if (Codec::FEN::import_fen(m_analysis.board, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 0", matched) != FEN_CODEC_SUCCESSFUL) {
			fprintf(stderr, "Invalid usage of 'position fen'. Invalid fen [%s]\n", command.c_str());
			return false;
		}

	} else {
		fprintf(stderr, "Invalid usage of 'position'. Expected 'fen' or 'startpos' but got [%s]\n", command.c_str());
		return false;
	}

	// Check if there is more data to parse
	if (command.empty()) {
		return true;
	} else if (command[0] != ' ') {
		fprintf(stderr, "Invalid usage of 'position'. Expected space after fen [%s]\n", command.c_str());
		return false;
	}

	if (command._Starts_with(" moves ")) {
		command = command.substr(7);
		return process_position_moves(command);
	}

	return false;
}

bool UciManager::process_setoption(std::string command) {
	if (!command._Starts_with("setoption name ")) {
		fprintf(stderr, "Invalid usage of 'setoption' [%s]\n", command.c_str());
		return false;
	}

	// Remove the alias from the command
	command = command.substr(15);

	// Match the longest command
	UciOption* option = nullptr;
	for (UciOption* item : m_analyser->get_options()) {
		if (command._Starts_with(item->get_key()) && (option == nullptr || (item->get_key().length() > option->get_key().length()))) {
			option = item;
		}
	}

	if (option == nullptr) {
		fprintf(stderr, "Invalid usage of 'setoption'. The option [%s] does not exist\n", command.c_str());
		return false;
	}

	// Removed the matched name from the command
	command = command.substr(option->get_key().length());

	if (option->get_type() != UciOptionType::BUTTON) {
		if (!command._Starts_with(" value ")) {
			fprintf(stderr, "Invalid usage of 'setoption'. Value tag was missing\n");
			return false;
		}

		// Remove ' value ' text
		command = command.substr(7);
	}

	return m_analyser->set_option(option->get_key(), command);
}

bool UciManager::process_debug_command(std::string command) {
	if (command == "@debugoptions") {
		_Debug_options(m_analyser);
	} else if (command == "@debugboard") {
		char* chars = Serial::getBoardString(&m_analysis.board);
		fprintf(stderr, "%s\n", chars);
		free(chars);
	} else if (command == "@debugfen") {
		fprintf(stderr, "%s\n", Codec::FEN::export_fen(m_analysis.board).c_str());
	}

	return true;
}

bool UciManager::process_command(std::string command) {
	if (command == "uci") {
		printf("id name %s\n", m_name.c_str());
		printf("id author %s\n", m_author.c_str());

		for (UciOption* option : m_analyser->get_options()) {
			printf("%s\n", option->to_string().c_str());
		}

		printf("uciok\n");
		return true;
	} else if (command == "ucinewgame") {
		return true;
	} else if (command == "stop") {
		m_analyser->stop_analysis();
		return true;
	} else if (command == "quit") {
		m_running = false;
		return true;
	} else if (command == "isready") {
		printf("readyok\n");
		return true;
	}

	if (command._Starts_with("go")) { 
		return process_go(command);
	}

	if (command._Starts_with("setoption")) {
		return process_setoption(command);
	} else if (command._Starts_with("position")) {
		// TODO: calulate the hash of each board and store them to check for threefold repetition
		return process_position(command);
	} else if (command._Starts_with("@")) {
		return process_debug_command(command);
	}
	
	/*
	if (alias == "ponderhit") {
		return process_ponderhit(command);
	}
	*/

	return false;
}

void UciManager::run() {
	std::string command;
	while (m_running) {
		std::getline(std::cin, command);
		if (!process_command(command)) {
			// Invalid command
			fprintf(stderr, "Unknown uci command [%s]\n", command.c_str());
		}
	}
}

bool UciManager::running() {
	return m_running;
}
