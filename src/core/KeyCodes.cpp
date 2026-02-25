#include "KeyCodes.hpp"

const char* to_string(KeyCode key)
{
	switch (key)
	{
		// This time, define the KEY macro to generate "case KeyCode::name: return "name";"
		#define KEY(name, value) case KeyCode::name: return #name;

		// Include the file again. It will now stamp out all the case statements.
		#include "KeyCodes.def"
		
		// Clean up the macro.
		#undef KEY
	}
	return "Unknown Key";
}