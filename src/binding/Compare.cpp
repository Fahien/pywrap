#include "pywrap/binding/Compare.h"

#include "pywrap/binding/Tag.h"

namespace pywrap
{
namespace binding
{
Compare::Compare( const Tag& t ) : tag{ t }
{
	// Initialized by the tag
}

void Compare::gen_name() { name << tag.get_py_name() << "_cmp"; }

void Compare::gen_sign() { sign << "PyObject* " << name.str() << "( _PyspotWrapper* lhs, _PyspotWrapper* rhs, int op )"; }

void Compare::gen_def()
{
	// operator ==
	def << sign.str() << "\n{\n"
	    << "\tif ( op == Py_EQ )\n\t{\n"
	    << "\t\tauto& l = *reinterpret_cast<" << tag.get_qualified_name() << "*>( lhs->data );\n"
	    << "\t\tauto& r = *reinterpret_cast<" << tag.get_qualified_name() << "*>( rhs->data );\n\n"
	    << "\t\tif ( l == r )\n\t\t{\n"
	       "\t\t\tPy_INCREF( Py_True );\n\t\t\treturn Py_True;\n\t\t}\n"
	       "\t\telse\n\t\t{\n"
	       "\t\t\tPy_INCREF( Py_False );\n\t\t\treturn Py_False;\n\t\t}\n\t}\n"
	    << "}\n\n";
}
}  // namespace binding
}  // namespace pywrap
