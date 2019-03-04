#include "pywrap/Printer.h"


namespace pywrap
{
#define OPEN_FILE_STREAM( file, name )                                                  \
	std::error_code      error;                                                         \
	llvm::raw_fd_ostream file{ name, error, llvm::sys::fs::F_None };                    \
	if ( error )                                                                        \
	{                                                                                   \
		llvm::errs() << " while opening '" << name << "': " << error.message() << '\n'; \
		exit( 1 );                                                                      \
	}


void Printer::printBindingsHeader( llvm::StringRef name )
{
	OPEN_FILE_STREAM( file, name );

	// Guards
	file << "#ifndef PYSPOT_BINDINGS_H_\n#define PYSPOT_BINDINGS_H_\n\n";

	// Includes
	for ( auto& name : m_ClassIncludes )
	{
		file << name << "\n";
	}

	// Tail includes
	file << "\n#include <pyspot/Wrapper.h>\n#include <structmember.h>\n\n";

	// Extern C
	file << "\n#ifdef __cplusplus\nextern \"C\" {\n#endif // __cplusplus\n\n";

	for ( auto& decl : m_ClassDeclarations )
	{
		file << decl;
	}

	// End extern C
	file << "\n#ifdef __cplusplus\n} // extern \"C\"\n#endif // __cplusplus\n\n";

	// Wrapper constructors declarations
	for ( auto& constr : m_WrapperDeclarations )
	{
		file << constr;
	}

	// End guards
	file << "\n#endif // PYSPOT_BINDINGS_H_\n";
}


void Printer::printBindingsSource( llvm::StringRef name )
{
	OPEN_FILE_STREAM( file, name );

	auto include = name.slice( 4, name.size() - 3 );
	auto source  = "#include \"" + include.str() + "h\"\n\n#include <Python.h>\n#include <pyspot/String.h>\n";

	// Class binding definitions
	for ( auto& def : m_ClassDefinitions )
	{
		source += def;
	}

	// Wrapper constructors definitions
	for ( auto& constr : m_WrapperDefinitions )
	{
		source += constr;
	}

	file << source;
}


void Printer::printExtensionHeader( llvm::StringRef name )
{
	OPEN_FILE_STREAM( file, name );

	// Guards
	file << "#ifndef PYSPOT_EXTENSION_H_\n"
	        "#define PYSPOT_EXTENSION_H_\n\n"
	        "#include <Python.h>\n\n";

	// Extern C
	file << "\n#ifdef __cplusplus\nextern \"C\" {\n#endif // __cplusplus\n\n"

	        "extern PyObject* g_pPyspotError;\n"
	        "extern char g_aPyspotErrorName[13];\n"
	        "extern char g_aErrorName[6];\n\n"
	        "struct ModuleState\n{\n\tPyObject* error;\n};\n\n"
	        "extern char g_aPyspotDescription[7];\n\n"
#if PY_MAJOR_VERSION >= 3
	        "extern PyModuleDef g_sModuleDef;\n\n"
#endif
	    ;

	// Print modules declaration
	for ( auto& pr : *modules )
	{
		auto& module = pr.second;
		file << module.get_decl();
	}

	// End extern C
	file << "\n#ifdef __cplusplus\n} // extern \"C\"\n#endif // __cplusplus\n\n";


	// End guards
	file << "#endif // PYSPOT_EXTENSION_H_\n";
}


void Printer::printExtensionSource( llvm::StringRef name )
{
	OPEN_FILE_STREAM( file, name );

	file << "#include \"" << name.slice( 4, name.size() - 3 ).str()
	     << "h\"\n\n"
	        "#include \"pyspot/Bindings.h\"\n\n"
	        "PyObject* g_pPyspotError = nullptr;\n"
	        "char g_aPyspotErrorName[] { \"pyspot.error\" };\n"
	        "char g_aErrorName[] { \"error\" };\n"
	        "char g_aPyspotDescription[] { \"Pyspot\" };\n\n"
#if PY_MAJOR_VERSION >= 3
	        "PyModuleDef g_sModuleDef {\n"
	        "\tPyModuleDef_HEAD_INIT,\n"
	        "\tg_aPyspotDescription,\n"
	        "\tnullptr,\n"
	        "\tsizeof( ModuleState ),\n"
	        "\tnullptr,\n"
	        "\tnullptr,\n"
	        "\tnullptr,\n"
	        "\tnullptr,\n"
	        "\tnullptr,\n"
	        "};\n\n"
#endif
	    ;

	for ( auto& pr : *modules )
	{
		auto& module = pr.second;
		file << module.get_methods().get_def();
		file << module.get_def();
	}

	file <<
	    // Extension function
	    "PyMODINIT_FUNC PyInit_" << m_ExtensionName
	     << "()\n{\n"
	        "\t// Create the module\n"
	        "\tPyObject* pModule { "
#if PY_VERSION >= 3
	        "PyModule_Create( &g_sModuleDef )"
#else
	        "Py_InitModule( \""
	     << m_ExtensionName
	     << "\", nullptr )"
#endif
	        "};\n"
	        "\tif ( pModule == nullptr )\n\t{\n\t\treturn;\n\t}\n\n"
	        "\t// Module exception\n"
	        "\tg_pPyspotError = PyErr_NewException( g_aPyspotErrorName, nullptr, nullptr );\n"
	        "\tPy_INCREF( g_pPyspotError );\n"
	        "\tPyModule_AddObject( pModule, g_aErrorName, g_pPyspotError );\n\n";

	// Add objects to module
	for ( auto& reg : m_ClassRegistrations )
	{
		file << reg;
	}

	// Add enumerators
	for ( auto& item : m_Enumerators )
	{
		file << item;
	}

	// Return the module
	file <<
#if PY_MAJOR_VERSION >= 3
	    "\treturn pModule;\n"
#endif
	    "}\n";
}


void Printer::PrintOut( const std::unordered_map<unsigned int, binding::Module>& m )
{
	// TODO make member variable
	modules = &m;

	llvm::sys::fs::create_directory( "include" );
	llvm::sys::fs::create_directory( "src" );
	llvm::sys::fs::create_directory( "include/pyspot" );
	llvm::sys::fs::create_directory( "src/pyspot" );
	printBindingsHeader( "include/pyspot/Bindings.h" );
	printBindingsSource( "src/pyspot/Bindings.cpp" );
	printExtensionHeader( "include/pyspot/Extension.h" );
	printExtensionSource( "src/pyspot/Extension.cpp" );
}


}  // namespace pywrap
