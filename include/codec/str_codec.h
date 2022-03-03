#pragma once

#ifndef STR_CODEC_H
#define STR_CODEC_H

#include <string>

namespace Codec::STR {
	template <typename integer_type>
	std::string read_integer(std::string& a_input, integer_type& a_output);
}

#endif // !STR_CODEC_H
