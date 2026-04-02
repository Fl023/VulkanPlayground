#include "MouseCodes.hpp"

const char* to_string(MouseCode button)
{
	switch (button)
	{
		// This time, define the KEY macro to generate "case KeyCode::name: return "name";"
		#define KEY(name, value) case MouseCode::name: return #name;
		#define KEY_ALIAS(name, value)
		// Include the file again. It will now stamp out all the case statements.
		#include "MouseCodes.def"
		
		// Clean up the macro.
		#undef KEY
		#undef KEY_ALIAS
	}
	return "Unknown Button";
}