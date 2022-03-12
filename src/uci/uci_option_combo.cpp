#include <sstream>
#include "uci_debug.h"
#include "uci_option.h"

UciOption::Combo::Combo(const std::string& key, std::initializer_list<std::string> list, int64_t def) : UciOption(key), m_def(def), m_val(def) {
	for (const std::string& str : list) {
		m_list.push_back(str);
	}
}

const std::vector<std::string>& UciOption::Combo::get_list() {
	return m_list;
}

int64_t UciOption::Combo::get_default() {
	return m_def;
}

int64_t UciOption::Combo::get_value() {
	return m_val;
}

UciOptionType UciOption::Combo::get_type() {
	return UciOptionType::COMBO;
}

bool UciOption::Combo::set_value(std::string& value) {
	size_t size = m_list.size();
	for (int i = 0; i < size; i++) {
		if (value == m_list[i]) {
			m_val = i;
			return true;
		}
	}

	fprintf(stderr, "Invalid usage of 'setoption'. UciOptionType::COMBO element does not exist [%s]\n", value.c_str());
	return false;
}

std::string UciOption::Combo::to_string() {
	std::stringstream ss;
	ss << "option name " << m_key << " type combo default " << m_list[m_def];

	for (const std::string& str : m_list) {
		ss << " var " << str;
	}

	return ss.str();
}
