#ifndef STR_CODEC_H
#define STR_CODEC_H

#include <string>

namespace Codec::STR {
	template <typename integer_type>
	inline std::string read_integer(std::string& a_input, integer_type& a_output) {
		int value = std::atoi(a_input.c_str());
		size_t len{ 0 };

		int loop = value;
		do {
			loop /= 10;
			len++;
		} while (loop != 0);

		a_output = (integer_type)value;
		return a_input.substr(len);
	}

}

#endif // STR_CODEC_H
