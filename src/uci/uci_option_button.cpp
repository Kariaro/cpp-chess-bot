#include "uci_debug.h"
#include "uci_option.h"

UciOption::Button::Button(const std::string& key) : UciOption(key) {
	
}

UciOptionType UciOption::Button::get_type() {
	return UciOptionType::BUTTON;
}

bool UciOption::Button::set_value(std::string& value) {
	if (!value.empty()) {
		fprintf(stderr, "Invalid usage of 'setoption'. UciOptionType::BUTTON does not allow non empty values [%s]\n", value.c_str());
		return false;
	}

	return true;
}

std::string UciOption::Button::to_string() {
	return "option name " + m_key + " type button";
}
