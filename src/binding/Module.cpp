#include "pywrap/binding/Module.h"

namespace pywrap
{
namespace binding
{
Module::Methods::Methods( const Module& m ) : module{ m }
{
	gen_py_name();
	gen_def();
}

void Module::Methods::gen_py_name() { py_name << "py_" << module.ns->getName().str() << "_methods"; }

void Module::Methods::gen_def() { def << "PyMethodDef " << get_py_name() << "[] = {\n"; }

const char* gen_meth( const Function& function )
{
	if ( function.get_func()->param_size() == 0 )
	{
		return "METH_NOARGS";
	} else
	{
		return "METH_VARARGS | METH_KEYWORDS";
	}
}

std::string Module::Methods::get_def() const { return def.str() + "\t{ NULL, NULL, 0, NULL } // sentinel\n};\n\n"; }

void Module::Methods::add( const Function& function )
{
	def << "\t{ \"" << function.get_name() << "\", " << function.get_py_name() << ", " << gen_meth( function ) << ", \""
	    << function.get_name() << "\" },\n";
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

void Module::gen_py_name() { py_name << "PyInit_" << get_name(); }

void Module::gen_sign() { sign << "PyMODINIT_FUNC " << get_py_name() << "()"; }

void Module::gen_decl() { decl << sign.str() << ";\n\n"; }

void Module::gen_def()
{
	std::stringstream exception;
	exception << "exception";

	std::stringstream exception_name;
	exception_name << exception.str() << "_name";

	std::stringstream module_exception_name;
	module_exception_name << get_name() << "_" << exception.str();

	def << sign.str() << "\n{\n\tauto module = Py_InitModule( name, " << methods.get_py_name() << " );\n\n"
	    << "\tstatic char " << module_exception_name.str() << "[] = { \"" << get_name() << ".exception\" };\n"
	    << "\tstatic char " << exception_name.str() << "[] = { \"" << exception.str() << "\" };\n"
	    << "\tauto " << exception.str() << " = PyErr_NewException( " << module_exception_name.str() << ", NULL, NULL );\n"
	    << "\tPy_INCREF( " << exception.str() << " );\n"
	    << "\tPyModule_AddObject( module, " << exception_name.str() << ", " << exception.str() << " );\n"
	    << "}\n";
}

void Module::add( Function&& f )
{
	methods.add( f );
	functions.emplace_back( std::move( f ) );
}

}  // namespace binding
}  // namespace pywrap