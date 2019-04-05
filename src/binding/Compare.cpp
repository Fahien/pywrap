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

void Compare::gen_name()
{
	name << tag.get_py_name() << "_cmp";
}

void Compare::gen_sign()
{
	sign << "PyObject* " << name.str() << "( _PyspotWrapper* lhs, _PyspotWrapper* rhs, int op )";
}

void Compare::gen_eq()
{
	def << "\tif ( op == Py_EQ )\n\t{\n"
	    << "\t\tauto& l = *reinterpret_cast<" << tag.get_qualified_name() << "*>( lhs->data );\n"
	    << "\t\tauto& r = *reinterpret_cast<" << tag.get_qualified_name() << "*>( rhs->data );\n\n"
	    << "\t\tif ( l == r )\n\t\t{\n"
	       "\t\t\tPy_INCREF( Py_True );\n\t\t\treturn Py_True;\n\t\t}\n"
	       "\t\telse\n\t\t{\n"
	       "\t\t\tPy_INCREF( Py_False );\n\t\t\treturn Py_False;\n\t\t}\n\t}\n";
}

void Compare::gen_def()
{
	def << sign.str() << "\n{\n";  // begin

	// operator ==
	if ( auto record = clang::dyn_cast<clang::CXXRecordDecl>( tag.get_handle() ) )
	{
		for ( auto method : record->methods() )
		{
			if ( method->isOverloadedOperator() )
			{
				if ( method->getOverloadedOperator() == clang::OO_EqualEqual )
				{
					gen_eq();
				}
			}
		}
	}
	else
	{
		gen_eq();
	}

	def << "}\n\n";  // end
}

}  // namespace binding
}  // namespace pywrap
