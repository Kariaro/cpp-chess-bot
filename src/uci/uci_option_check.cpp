#pragma once

#ifndef UCI_OPTION_CHECK_CPP
#define UCI_OPTION_CHECK_CPP

#include "../../include/uci/uci_debug.h"
#include "../../include/uci/uci_option.h"

UciOption::Check::Check(const std::string& key, bool def) : UciOption(key), m_def(def), m_val(def) {
	
}

bool UciOption::Check::get_default() {
	return m_def;
}

bool UciOption::Check::get_value() {
	return m_val;
}

UciOptionType UciOption::Check::get_type() {
	return UciOptionType::CHECK;
}

bool UciOption::Check::set_value(std::string& value) {
	if (value == "true") {
		m_val = true;
	} else if (value == "false") {
		m_val = false;
	} else {
		std::cerr << "Invalid usage of 'setoption'. UciOptionType::CHECK does not allow the value [" << value << "]" << std::endl;
		return false;
	}

	return true;
}

std::string UciOption::Check::to_string() {
	return "option name " + m_key + " type check default " + (m_def ? "true" : "false");
}

#endif // !UCI_OPTION_CHECK_CPP
