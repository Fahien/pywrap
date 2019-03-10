#include "pywrap/binding/Binding.h"

#include "pywrap/Util.h"


namespace pywrap
{
namespace binding
{
Binding::Binding( const clang::NamedDecl* n ) : named{ n } {}

void Binding::init()
{
	gen_name();
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

void Binding::gen_py_name()
{
	if ( named )
	{
		py_name << to_pyspot_name( named->getQualifiedNameAsString() );
	}
}

void Binding::gen_decl() { decl << sign.str() << ";\n\n"; }

}  // namespace binding
}  // namespace pywrap