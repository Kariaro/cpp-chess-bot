#pragma once

#ifndef UCI_OPTION_BUTTON_CPP
#define UCI_OPTION_BUTTON_CPP

#include "../../include/uci/uci_debug.h"
#include "../../include/uci/uci_option.h"

UciOption::Button::Button(const std::string& key) : UciOption(key) {
	
}

UciOptionType UciOption::Button::get_type() {
	return UciOptionType::BUTTON;
}

bool UciOption::Button::set_value(std::string& value) {
	if (!value.empty()) {
		std::cerr << "Invalid usage of 'setoption'. UciOptionType::BUTTON does not allow non empty values [" << value << "]" << std::endl;
		return false;
	}

	return true;
}

std::string UciOption::Button::to_string() {
	return "option name " + m_key + " type button";
}

#endif // !UCI_OPTION_BUTTON_CPP
