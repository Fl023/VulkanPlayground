#pragma once

enum class MouseCode
{
	// Define the KEY macro to generate "name = value,"
	#define KEY(name, value) name = value,
	#define KEY_ALIAS(name, value) name = value,

	// Include the file. The preprocessor will paste its contents here,
	// and for each line, it will stamp out our macro.
	#include "MouseCodes.def"

	// It's very important to undefine the macro immediately
	// to prevent it from leaking into other files.
	#undef KEY
	#undef KEY_ALIAS
};

// Forward declare our conversion function
const char* to_string(MouseCode button);

// The ostream operator can now call our function
inline std::ostream& operator<<(std::ostream& os, MouseCode buttonCode)
{
	os << to_string(buttonCode);
	return os;
}