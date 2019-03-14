#include "pywrap/binding/Init.h"

#include "pywrap/binding/Tag.h"

namespace pywrap
{
namespace binding
{
Init::Init( const Tag& t ) : tag{ t }
{
	// Initialized by the tag
}

void Init::gen_name()
{
	name << tag.get_py_name() << "_init";
}

void Init::gen_sign()
{
	sign << "int " << name.str() << "( _PyspotWrapper* self, PyObject* args, PyObject* kwds )";
}

void Init::gen_def()
{
	def << sign.str() << "\n{\n\t"
		<< tag.get_qualified_name() << "* data = nullptr;\n"
		<< "\tif ( self->data )\n\t{\n"
		<< "\t\tdata = reinterpret_cast<" << tag.get_qualified_name() << "*>( self->data );\n"
		<< "\t\treturn 0;\n\t}\n\n";
}

std::string Init::get_def() const
{
	return def.str() + "\treturn -1;\n}\n\n";
}
}  // namespace binding
}  // namespace pywrap
