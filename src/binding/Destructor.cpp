#include "pywrap/binding/Destructor.h"

#include "pywrap/binding/Tag.h"

namespace pywrap
{
namespace binding
{
Destructor::Destructor( const Tag& t ) : tag{ t }
{
	// Initialized by the tag
}

void Destructor::gen_name()
{
	name << tag.get_py_name() << "_dealloc";
}

void Destructor::gen_sign()
{
	sign << "void " << name.str() << "( _PyspotWrapper* self )";
}

void Destructor::gen_def()
{
	def << sign.str() << "\n{\n"
	    << "\tif ( self->own_data )\n\t{\n"
	    << "\t\tdelete reinterpret_cast<" << tag.get_qualified_name() << "*>( self->data );\n\t}\n"
	    << "\tPy_TYPE( self )->tp_free( reinterpret_cast<PyObject*>( self ) );\n}\n\n";
}
}  // namespace binding
}  // namespace pywrap
