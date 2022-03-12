#ifndef CHESS_ANALYSER_H
#define CHESS_ANALYSER_H

#include <thread>

#include "../uci/uci_option.h"
#include "../utils.h"

struct ChessAnalysis {
	Chessboard board{};
	Move bestmove{};
	Move ponder{};
	uint32_t m_max_time{0};
};

class ChessAnalyser {
public:
	ChessAnalyser() = default;

	/// Returns a vector of options this analyser has
	const std::vector<UciOption*>& get_options() {
		return m_options;
	}

	/// Update the value of an option inside this analyser
	bool set_option(const std::string& key, std::string& value) {
		UciOption* option = nullptr;
		for (UciOption* item : m_options) {
			if (key == item->get_key() && (option == nullptr || (option->get_key().length() < item->get_key().length()))) {
				option = item;
			}
		}

		if (option == nullptr) {
			return false;
		}

		bool result = option->set_value(value);

		if (result) {
			// Only call the update method when the operation was successful
			on_option_change(option);
		}

		return result;
	}

	/// Stop analyse the position
	virtual bool stop_analysis() = 0;

	/// Start analyse the position
	virtual bool start_analysis(ChessAnalysis& analysis) = 0;

protected:
	/// This method is called when an option changes
	virtual void on_option_change(UciOption* option) = 0;

	ChessAnalysis* m_analysis{ nullptr };
	std::vector<UciOption*> m_options;
	std::thread m_thread;
	bool m_running{ false };
	bool m_stop{ true };
};

#endif // CHESS_ANALYSER_H
