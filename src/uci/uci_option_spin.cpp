#pragma once

#ifndef UCI_OPTION_SPIN_CPP
#define UCI_OPTION_SPIN_CPP

#include "../../include/uci/uci_debug.h"
#include "../../include/uci/uci_option.h"

UciOption::Spin::Spin(const std::string& key, int64_t min, int64_t max, int64_t def) : UciOption(key), m_min(min), m_max(max), m_def(def), m_val(def) {
	
}

int64_t UciOption::Spin::get_default() {
	return m_def;
}

int64_t UciOption::Spin::get_minimum() {
	return m_min;
}

int64_t UciOption::Spin::get_maximum() {
	return m_max;
}

int64_t UciOption::Spin::get_value() {
	return m_val;
}

UciOptionType UciOption::Spin::get_type() {
	return UciOptionType::SPIN;
}

bool UciOption::Spin::set_value(std::string& value) {
	char* endPos;
	int num = std::strtol(value.c_str(), &endPos, 10);

	if ((value.c_str() + value.length()) != endPos) {
		fprintf(stderr, "Invalid usage of 'setoption'. UciOptionType::SPIN not all characters was a number [%s]\n", value.c_str());
		return false;
	}

	if (num < m_min || num > m_max) {
		fprintf(stderr, "Invalid usage of 'setoption'. UciOptionType::SPIN number is outside ranges [%lld, %lld], [%s]\n", m_min, m_max, value.c_str());
		return false;
	}

	m_val = num;
	return true;
}

std::string UciOption::Spin::to_string() {
	return "option name " + m_key + " type spin default " + std::to_string(m_def) + " min " + std::to_string(m_min) + " max " + std::to_string(m_max);
}

#endif // !UCI_OPTION_SPIN_CPP
