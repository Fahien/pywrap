#include "pywrap/binding/Function.h"

#include <sstream>

#include "pywrap/Util.h"


namespace pywrap
{
namespace binding
{
Function::Function( const clang::FunctionDecl* f ) : Binding{ f }, func{ f }
{
	init();
}

void Function::gen_sign()
{
	// Return type is known
	sign << "PyObject* ";

	// Python name of the function with namespace
	sign << get_py_name();

	// Function parameters are always these
	sign << "( PyObject* self, PyObject* args )";
}

void Function::gen_def()
{
	// TODO consider whether it is too complex

	// Open {
	def << sign.str() << "\n{\n";

	std::stringstream kwlist;
	kwlist << "static char* kwlist[] { ";

	std::stringstream fmt;
	fmt << "static const char* fmt { \"";
	std::stringstream arg_list;
	std::stringstream call_arg_list;

	// Perform the call
	std::stringstream call;
	call << func->getQualifiedNameAsString() << "(";

	// Check if there are parameters
	auto param_count = func->parameters().size();
	if ( param_count > 0 )
	{
		for ( size_t i = 0; i < param_count; ++i )
		{
			// TODO Param object?

			auto param = func->parameters()[i];

			// We need to get the parameter by string
			// So we create a string repr of the param
			auto name = param->getName().str();

			auto kwname = name + "_str";
			def << "\tstatic char " << kwname << "[] = { \"" << name << "\" };\n";

			// This array needs to be added to the kwlist
			kwlist << kwname << ", ";

			auto paramName       = "pw" + param->getNameAsString();
			auto dataName        = "p" + param->getNameAsString();
			auto staticParamName = "a" + paramName;

			// We need to know the type of the param
			auto qualType = param->getType();
			if ( param->isTemplated() )
			{
				// TODO Handle template functions
			}
			// So we can convert it properly
			fmt << pywrap::to_parser( qualType );

			// We then need to call this function passing this argument, which is
			// coming in as a python type, so we need to parse it using ParseTupleAndKeywords
			// which expects an output parameter
			auto type = qualType.getAsString();
			if ( qualType->isBuiltinType() )
			{
				def << type << " " + name + ";\n";
				// This will be used to call the wrapped function
				call_arg_list << name;
			}
			else
			{
				if ( qualType->isPointerType() )
				{
					auto pPointeeType = qualType->getPointeeOrArrayElementType();
					// String
					if ( pPointeeType->isAnyCharacterType() )
					{
						def << "PyObject* " << name << ";\n";
						call_arg_list << pywrap::to_c( type, name );
					}
				}
				else
				{
					// Wrapper
					def << "_PyspotWrapper* " << name << ";\n";
					// Pointer to data
					def << "\tauto " + name + " = reinterpret_cast<" + type + "*>( " + name + "->pData );\n";
					call_arg_list << "*" + name + ", ";
				}
			}
			arg_list << ", &" << name;
		}

		def << "\t" << kwlist.rdbuf() << "nullptr };\n";
		def << "\t" << fmt.rdbuf() << "|\" };\n\n";

		def << "\tif ( !PyArg_ParseTupleAndKeywords( args, kwds, fmt, kwlist," << arg_list.str() <<
		    // TODO macro for returning Py_None?
		    " ) )\n\t{\n\t\tPy_INCREF( Py_None );\n\t\treturn Py_None;\n\t}\n\n";

		call << " " << call_arg_list.rdbuf() << " ";
	}
	call << ")";

	// If is not returning
	if ( func->getReturnType()->isVoidType() )
	{
		def << "\t" << call.rdbuf()
		    << ";\n"
		       "\tPy_INCREF( Py_None );\n\treturn Py_None;\n}\n\n";
	}
	else
	{
		def << "\treturn " << pywrap::to_python( func->getReturnType(), call.str() ) << ";\n";
	}

	// Close }
	def << "}\n";
}


}  // namespace binding
}  // namespace pywrap
