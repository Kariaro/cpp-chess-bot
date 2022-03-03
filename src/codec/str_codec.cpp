#include "../../include/codec/str_codec.h"

template <typename integer_type>
std::string Codec::STR::read_integer(std::string& a_input, integer_type& a_output) {
	int value = std::atoi(a_input.c_str());
	size_t len{0};

	int loop = value;
	do {
		loop /= 10;
		len++;
	} while (loop != 0);

	a_output = (integer_type)value;
	return a_input.substr(len);
}
