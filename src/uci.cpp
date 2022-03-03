
#include <string>
#include <array>
#include <iostream>

#include "../include/uci/uci_option.h"
#include "../include/codec/fen_codec.h"
#include "../include/analyser/chess_analyser.h"

#include "uci.h"
#include "ab_pruning_v2.h"

constexpr auto ENGINE_AUTHOR = "HardCoded";
constexpr auto ENGINE_NAME = "HardCodedBot 1.0";

namespace UCI {
	std::string GetUciCommandAlias(std::string& command) {
		const char* begin = command.c_str();
		const char* position = std::strchr(begin, ' ');

		return position == nullptr
			? command
			: command.substr(0, position - begin);
	}

	void DebugUciOptions(ChessAnalyser* analyser) {
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

	void PrintUciOptions(ChessAnalyser* analyser) {
		for (UciOption* option : analyser->get_options()) {
			std::cout << "option name " << option->get_key() << " type ";

			switch (option->get_type()) {
				case UciOptionType::CHECK: {
					UciOption::Check* opt = (UciOption::Check*)option;
					std::cout << "check default " << opt->get_default() << std::endl;
					break;
				}
				case UciOptionType::SPIN: {
					UciOption::Spin* opt = (UciOption::Spin*)option;
					std::cout << "spin default " << opt->get_default()
							  << " min " << opt->get_minimum()
							  << " max " << opt->get_maximum() << std::endl;
					break;
				}
				case UciOptionType::COMBO: {
					UciOption::Combo* opt = (UciOption::Combo*)option;
					std::cout << "combo default " << opt->get_list()[opt->get_value()];
					
					for (const std::string& str : opt->get_list()) {
						std::cout << " var " << str;
					}

					std::cout << std::endl;
					break;
				}
				case UciOptionType::BUTTON: {
					std::cout << "button" << std::endl;
					break;
				}
				case UciOptionType::STRING: {
					UciOption::String* opt = (UciOption::String*)option;
					std::cout << "string default " << opt->get_default() << std::endl;
					break;
				}
			}

			option++;
		}
	}

	bool SetUciOption(ChessAnalyser* analyser, std::string command) {
		if (!command._Starts_with("setoption name ")) {
			std::cerr << "Invalid usage of 'setoption' [" << command << "]" << std::endl;
			return false;
		}

		// Remove the alias from the command
		command = command.substr(15);

		// Match the longest command
		UciOption* option = nullptr;
		for (UciOption* item : analyser->get_options()) {
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

		return analyser->set_option(option->get_key(), command);
	}

	bool SetAnalysisPosition(ChessAnalysis& analysis, std::string command) {
		if (!command._Starts_with("position ")) {
			std::cerr << "Invalid usage of 'position' [" << command << "]" << std::endl;
			return false;
		}

		// Remove the alias from the command
		command = command.substr(9);

		// startpos, fen
		if (command._Starts_with("fen ")) {
			command = command.substr(4);
			int matched;
			if (Codec::FEN::import_fen(analysis.board, command, matched) != FEN_CODEC_SUCCESSFUL) {
				std::cerr << "Invalid usage of 'position fen'. Invalid fen [" << command << "]" << std::endl;
				return false;
			}

			command = command.substr(matched);
		} else if (command._Starts_with("startpos")) {
			command = command.substr(8);
			int matched;
			if (Codec::FEN::import_fen(analysis.board, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 0", matched) != FEN_CODEC_SUCCESSFUL) {
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

		// Remove the space
		command = command.substr(1);

		std::cerr << "Remaining: [" << command << "]" << std::endl;

		return true;
	}

	// http://wbec-ridderkerk.nl/html/UCIProtocol.html
	void StartUCI() {
		std::string line;
		std::string alias;
		bool hasUCI = false;
		ChessAnalyser* analyser = new ABPruningV2();
		ChessAnalysis analysis{};

		// While true keep looping
		while (true) {
			std::getline(std::cin, line);

			alias = GetUciCommandAlias(line);

			if (alias == "uci") {
				// Print information about the name and author of this bot
				std::cout << "id name " << ENGINE_NAME << std::endl
				          << "id author " << ENGINE_AUTHOR << std::endl
						  << std::endl;
				
				// Print information about the currently available options
				PrintUciOptions(analyser);
				
				hasUCI = true;
				std::cout << "uciok" << std::endl;
				continue;
			}

			if (alias == "@debugoptions") {
				DebugUciOptions(analyser);
				continue;
			}

			// If we do not have uci we continue
			if (!hasUCI) {
				continue;
			}

			if (alias == "setoption") {
				SetUciOption(analyser, line);
				continue;
			}

			if (alias == "isready") {
				std::cout << "readyok";
				continue;
			}

			if (alias == "position") {
				SetAnalysisPosition(analysis, line);
				continue;
			}

			if (alias == "go") {
				analyser->start_analysis(analysis);
				continue;
			}

			if (alias == "quit") {
				// Break the loop
				return;
			}

			// Print an error
			std::cerr << "Unknown uci command [" << line << "]" << std::endl;
		}
	}
}
