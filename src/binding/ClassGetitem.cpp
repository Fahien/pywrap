#include "pywrap/binding/ClassGetitem.h"

#include "pywrap/binding/CXXRecord.h"

namespace pywrap
{
namespace binding
{
void ClassGetitem::gen_py_name()
{
	if ( !tag )
	{
		py_name << "0";
		return;
	}

	py_name << tag->get_py_name() << "_class_getitem";
}


void ClassGetitem::gen_sign()
{
	if ( tag )
	{
		sign << "PyObject* " << py_name.str() << "( PyObject* type, PyObject* item )";
	}
}


void ClassGetitem::gen_def()
{
	if ( tag )
	{
		def << sign.str() << "\n{\n"
		    << "\tusing namespace std::literals;\n"
		    << "\tauto item_type = reinterpret_cast<PyTypeObject*>( item );\n\n";
	}
}

void ClassGetitem::add( const Specialization& spec )
{
	auto& args = spec.get_args();
	assert( args.size() == 1 && "Multiple template arguments not supported yet" );
	auto& arg = args[0];
	def << "\tif ( item_type->tp_name == \"" << arg.getAsType().getAsString() << "\"s )\n\t{\n"
	    << "\t\tPy_INCREF( &" << spec.get_type_object().get_py_name() << " );\n"
	    << "\t\treturn reinterpret_cast<PyObject*>( &" << spec.get_type_object().get_py_name() << " );\n"
	    << "\t}\n\n";
}

std::string ClassGetitem::get_def() const
{
	if ( !tag )
	{
		return "";
	}

	return def.str() + "\treturn nullptr;\n}\n\n";
}

}  // namespace binding
}  // namespace pywrap
