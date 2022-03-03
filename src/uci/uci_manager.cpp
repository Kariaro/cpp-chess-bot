#include "../../include/uci/uci_manager.h"
#include "../../include/codec/fen_codec.h"
#include "../../include/codec/str_codec.h"
// won't compile without this
#include "../codec/fen_codec.cpp"
#include "../codec/str_codec.cpp"

#include "../generator.h"
#include "../serial.h"

#include <iostream>

void _Debug_options(ChessAnalyser* analyser) {
	for (UciOption* option : analyser->get_options()) {
		std::cerr << "into string [" << option->get_key() << "] ";

		switch (option->get_type()) {
		case UciOptionType::CHECK: {
			UciOption::Check* opt = (UciOption::Check*)option;
			std::cerr << "[check] = " << (opt->get_value() ? "true" : "false") << std::endl;
			break;
		}
		case UciOptionType::SPIN: {
			UciOption::Spin* opt = (UciOption::Spin*)option;
			std::cerr << "[spin] = " << opt->get_value() << std::endl;
			break;
		}
		case UciOptionType::COMBO: {
			UciOption::Combo* opt = (UciOption::Combo*)option;
			std::cerr << "[combo] = " << opt->get_list()[opt->get_value()] << std::endl;
			break;
		}
		case UciOptionType::BUTTON: {
			std::cerr << "[button]" << std::endl;
			break;
		}
		case UciOptionType::STRING: {
			UciOption::String* opt = (UciOption::String*)option;
			std::cerr << "[string] = " << opt->get_value() << std::endl;
			break;
		}
		default: {
			std::cerr << "Undefined UciOptionType ( " << (int)option->get_type() << " )" << std::endl;
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
		std::cerr << "Failed to fully parse the go command" << std::endl;
	}

	bool is_white = Board::isWhite(m_analysis.board);
	uint64_t turn_inc = is_white ? winc : binc;
	uint64_t turn_time = is_white ? wtime : btime;

	if (infinite) {
		m_analysis.m_max_time = 10000000000;
	} else {
		m_analysis.m_max_time = turn_time > 100000 ? 10000 : ((turn_time / 10) + 1);
	}
	m_analyser->start_analysis(m_analysis);

	return true;
}

bool UciManager::process_position_moves(std::string command) {
	// std::cerr << "Remaining: [" << command << "]" << std::endl;

	while (!command.empty()) {
		size_t first_space = command.find_first_of(' ');

		std::string move_str = (first_space == -1) ? command : command.substr(0, first_space);
		if (first_space != -1) {
			command = command.substr((size_t)first_space + 1ull);
		} else {
			command = "";
		}

		// std::cerr << "into string [" << move_str << "]" << std::endl;
		bool found = false;
		std::vector<Move> moves = Generator::generate_valid_moves(m_analysis.board);
		for (Move move : moves) {
			// std::cerr << "into string [" << move_str << "] <=> [" << Serial::get_move_string(move) << "]" << std::endl;

			if (move_str == Serial::get_move_string(move)) {
				found = Generator::playMove(m_analysis.board, move);
				break;
			}
		}

		if (!found) {
			std::cerr << "into string Could not play the move [" << move_str << "]" << std::endl;
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
			std::cerr << "Invalid usage of 'position fen'. Invalid fen [" << command << "]" << std::endl;
			return false;
		}

		command = command.substr(matched);
	} else if (command._Starts_with("position startpos")) {
		command = command.substr(17);
		int matched;
		if (Codec::FEN::import_fen(m_analysis.board, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 0", matched) != FEN_CODEC_SUCCESSFUL) {
			std::cerr << "Invalid usage of 'position fen'. Invalid fen [" << command << "]" << std::endl;
			return false;
		}

	} else {
		std::cerr << "Invalid usage of 'position'. Expected 'fen' or 'startpos' but got [" << command << "]" << std::endl;
		return false;
	}

	// Check if there is more data to parse
	if (command.empty()) {
		return true;
	} else if (command[0] != ' ') {
		std::cerr << "Invalid usage of 'position'. Expected space after fen [" << command << "]" << std::endl;
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
		std::cerr << "Invalid usage of 'setoption' [" << command << "]" << std::endl;
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
		std::cerr << "Invalid usage of 'setoption'. The option [" << command << "] does not exist" << std::endl;
		return false;
	}

	// Removed the matched name from the command
	command = command.substr(option->get_key().length());

	if (option->get_type() != UciOptionType::BUTTON) {
		if (!command._Starts_with(" value ")) {
			std::cerr << "Invalid usage of 'setoption'. Value tag was missing" << std::endl;
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
		std::cerr << Serial::getBoardString(&m_analysis.board) << std::endl;
	} else if (command == "@debugfen") {
		std::cerr << Codec::FEN::export_fen(m_analysis.board) << std::endl;
	}

	return true;
}

bool UciManager::process_command(std::string command) {
	if (command == "uci") {
		std::cout << "id name " << m_name << std::endl
				  << "id author " << m_author << std::endl;

		for (UciOption* option : m_analyser->get_options()) {
			std::cout << option->to_string() << std::endl;
		}

		std::cout << "uciok" << std::endl;
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
		std::cout << "readyok" << std::endl;
		return true;
	}

	if (command._Starts_with("go")) { 
		return process_go(command);
	}

	if (command._Starts_with("setoption")) {
		return process_setoption(command);
	} else if (command._Starts_with("position")) {
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
			std::cerr << "Unknown uci command [" << command << "]" << std::endl;
		}
	}
}

bool UciManager::running() {
	return m_running;
}
