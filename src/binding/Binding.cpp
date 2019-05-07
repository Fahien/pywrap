#include "pywrap/binding/Binding.h"

#include "pywrap/Util.h"


namespace pywrap
{
namespace binding
{
Binding::Binding( const clang::NamedDecl* n, const Binding* p )
    : named{ n }, id{ n ? n->getQualifiedNameAsString() : "" }, parent{ p }
{
}


void Binding::init()
{
	gen_name();
	gen_qualified_name();
	gen_py_name();
	gen_sign();
	gen_decl();
	gen_def();
}


void Binding::gen_name()
{
	if ( named )
	{
		name << named->getName().str();
	}
}


void Binding::gen_qualified_name()
{
	if ( named )
	{
		qualified_name << named->getQualifiedNameAsString();
	}
}

void Binding::gen_py_name()
{
	if ( named )
	{
		py_name << to_pyspot_name( qualified_name.str() );
	}
}


void Binding::gen_decl()
{
	decl << sign.str() << ";\n\n";
}


}  // namespace binding
}  // namespace pywrap