#include "uci_debug.h"
#include "uci_option.h"

UciOption::String::String(const std::string& key, const std::string& def) : UciOption(key), m_def(def), m_val(def) {
	
}

const std::string& UciOption::String::get_default() {
	return this->m_def;
}

const std::string& UciOption::String::get_value() {
	return this->m_val;
}

UciOptionType UciOption::String::get_type() {
	return UciOptionType::STRING;
}

bool UciOption::String::set_value(std::string& value) {
	this->m_val = value;
	return true;
}

std::string UciOption::String::to_string() {
	return "option name " + m_key + " type string default " + m_def;
}
