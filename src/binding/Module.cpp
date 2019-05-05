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


void Module::Methods::gen_py_name()
{
	py_name << "py_" << module.ns->getName().str() << "_methods";
}


void Module::Methods::gen_def()
{
	def << "PyMethodDef " << get_py_name() << "[] = {\n";
}


const char* gen_meth( const Function& function )
{
	if ( function.get_func().param_size() == 0 )
	{
		return "METH_NOARGS";
	}
	else
	{
		return "METH_VARARGS | METH_KEYWORDS";
	}
}


std::string Module::Methods::get_def() const
{
	return def.str() + "\t{ NULL, NULL, 0, NULL } // sentinel\n};\n\n";
}


void Module::Methods::add( const Function& function )
{
	def << "\t{ \"" << function.get_name() << "\", " << function.get_py_name() << ", " << gen_meth( function ) << ", \""
	    << function.get_name() << "\" },\n";
}


Module::Module( const clang::NamespaceDecl* n, const Binding* parent ) : Binding{ n, parent }, ns{ n }, methods{ *this }
{
	init();
	if ( parent )
	{
		gen_reg();
	}
}


void Module::gen_sign()
{
	sign << "PyMODINIT_FUNC PyInit_" << get_name() << "()";
}


void Module::gen_def()
{
	std::stringstream description;
	description << get_py_name() << "_description";

	std::stringstream module_def;
	module_def << get_py_name() << "_module_def";

	def << "char " << description.str() << "[] = \"" << get_py_name() << "\";\n\n"
	    << "PyModuleDef " << module_def.str() << "{\n"
	    << "\tPyModuleDef_HEAD_INIT,\n"
	    << "\t" << description.str() << ",\n"
	    << "\tnullptr,\n"
	    << "\tsizeof( ModuleState ),\n"
	    << "\t" << methods.get_py_name() << ",\n"
	    << "\tnullptr,\n"
	    << "\tnullptr,\n"
	    << "\tnullptr,\n"
	    << "\tnullptr,\n};\n\n";

	// Module init function if not nested
	if ( !parent )
	{
		def << sign.str() << "\n{\n\tauto " << get_py_name() << " = PyModule_Create( &" << module_def.str() << " );\n\n";

		// Exception
		std::stringstream exception;
		exception << "exception";

		std::stringstream module_exception_name;
		module_exception_name << get_py_name() << "_" << exception.str();

		def << "\tstatic char " << module_exception_name.str() << "[] = { \"" << get_name() << ".exception\" };\n"
		    << "\tauto " << exception.str() << " = PyErr_NewException( " << module_exception_name.str()
		    << ", NULL, NULL );\n"
		    << "\tPy_INCREF( " << exception.str() << " );\n"
		    << "\tPyModule_AddObject( " << get_py_name() << ", \"" << exception.str() << "\", " << exception.str()
		    << " );\n\n";
	}

	// will be closed by get_def
}


std::string Module::get_def() const
{
	auto ret = def.str();

	// Close init function if not nested module
	if ( !parent )
	{
		for ( auto& module : modules )
		{
			ret += module.get_reg();
		}

		ret += "\treturn " + get_py_name() + ";\n}\n";
	}

	return ret;
}


void Module::gen_reg()
{
	std::stringstream module_def;
	module_def << get_py_name() << "_module_def";

	reg << "\tauto " << get_py_name() << " = PyModule_Create( &" << module_def.str() << " );\n\n";

	// Exception
	std::stringstream exception;
	exception << get_py_name() << "_exception";

	std::stringstream module_exception_name;
	module_exception_name << get_py_name() << "_" << exception.str();

	reg << "\tstatic char " << module_exception_name.str() << "[] = { \"" << get_name() << ".exception\" };\n"
	    << "\tauto " << exception.str() << " = PyErr_NewException( " << module_exception_name.str() << ", NULL, NULL );\n"
	    << "\tPy_INCREF( " << exception.str() << " );\n"
	    << "\tPyModule_AddObject( " << get_py_name() << ", \"" << exception.str() << "\", " << exception.str() << " );\n\n";

	// Reg to parent
	reg << "\tPyModule_AddObject( " << parent->get_py_name() << ", \"" << get_name() << "\", " << get_py_name() << " );\n\n";
}


std::string Module::get_reg() const
{
	assert( parent && "Module has no parent to register to" );
	return reg.str();
}


void Module::add( Module&& m )
{
	modules.emplace_back( std::move( m ) );
}


void Module::add( Function&& f )
{
	methods.add( f );
	functions.emplace_back( std::move( f ) );
}


void Module::add( Enum&& e )
{
	if ( parent )
	{
		reg << e.get_reg();
	}
	else
	{
		def << e.get_reg();
	}
	enums.emplace_back( std::move( e ) );
}


void Module::add( CXXRecord&& r )
{
	if ( parent )
	{
		reg << r.get_reg();
	}
	else
	{
		def << r.get_reg();
	}
	records.emplace_back( std::move( r ) );
}


void Module::add( Template&& t )
{
	if ( parent )
	{
		reg << t.get_reg();
	}
	else
	{
		def << t.get_reg();
	}
	templates.emplace_back( std::move( t ) );
}


void Module::add( Specialization&& s )
{
	if ( parent )
	{
		reg << s.get_reg();
	}
	else
	{
		def << s.get_reg();
	}
	specializations.emplace_back( std::move( s ) );
}


}  // namespace binding
}  // namespace pywrap
