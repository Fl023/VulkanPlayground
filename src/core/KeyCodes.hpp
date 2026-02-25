#pragma once

enum class KeyCode
{
	// Define the KEY macro to generate "name = value,"
	#define KEY(name, value) name = value,

	// Include the file. The preprocessor will paste its contents here,
	// and for each line, it will stamp out our macro.
	#include "KeyCodes.def"

	// It's very important to undefine the macro immediately
	// to prevent it from leaking into other files.
	#undef KEY
};

// Forward declare our conversion function
const char* to_string(KeyCode key);

// The ostream operator can now call our function
inline std::ostream& operator<<(std::ostream& os, KeyCode keyCode)
{
	os << to_string(keyCode);
	return os;
}