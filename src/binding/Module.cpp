#include "pywrap/binding/Module.h"

namespace pywrap
{
namespace binding
{
Module::Methods::Methods( const Module& m ) : module{ m }, closed{ false } { gen_def(); }

void Module::Methods::gen_py_name() { py_name << "py_" << module.ns->getName().str() << "_methods"; }

void Module::Methods::gen_def() { def << "PyMethodDef " << get_py_name() << "[] = {\n"; }

const char* gen_meth( const Function& function )
{
	if ( function.get_decl()->param_size() == 0 )
	{
		return "METH_NOARGS";
	} else
	{
		return "METH_VARARGS | METH_KEYWORDS";
	}
}

void Module::Methods::add( const Function& function )
{
	assert( !closed && "Methods have been closed" );
	def << "\t{ \"" << function.get_name() << "\", " << function.get_py_name() << ", " << gen_meth( function ) << ", \""
	    << function.get_name() << "\" },\n";
}

void Module::Methods::close()
{
	closed = true;
	def << "\t{ NULL, NULL, 0, NULL } // sentinel\n};\n";
}

Module::Module( const clang::NamespaceDecl* n ) : ns{ n }, methods{ *this }
{
	gen_name();
	gen_py_name();
	gen_sign();
	gen_decl();
	gen_def();
}

void Module::gen_name() { name << ns->getName().str(); }

void Module::gen_py_name() { py_name << "init_" << get_name(); }

void Module::gen_sign() { sign << "PyMODINIT_FUNC " << get_py_name() << "()"; }

void Module::gen_decl() { decl << sign.str() << ";"; }

void Module::gen_def()
{
	def << sign.str() << "\n{\n\tPy_InitModule( name, " << methods.get_py_name() << " );\n}\n";
}

void Module::add( Function&& function ) { methods.add( function ); }

}  // namespace binding
}  // namespace pywrap
