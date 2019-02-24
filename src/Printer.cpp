#include "pywrap/Printer.h"


namespace pyspot
{
#define OPEN_FILE_STREAM( file, name )                                                  \
	std::error_code      error;                                                         \
	llvm::raw_fd_ostream file{ name, error, llvm::sys::fs::F_None };                    \
	if ( error )                                                                        \
	{                                                                                   \
		llvm::errs() << " while opening '" << name << "': " << error.message() << '\n'; \
		exit( 1 );                                                                      \
	}


void Printer::printBindingsHeader( StringRef name )
{
	OPEN_FILE_STREAM( file, name );

	// Guards
	std::string source{ "#ifndef PYSPOT_BINDINGS_H_\n#define PYSPOT_BINDINGS_H_\n\n" };

	// Includes
	for ( auto& name : m_ClassIncludes )
	{
		source += name + "\n";
	}

	// Tail includes
	source += "\n#include <pyspot/Wrapper.h>\n#include <structmember.h>\n\n";

	// Extern C
	source += "\n#ifdef __cplusplus\nextern \"C\" {\n#endif // __cplusplus\n\n";

	for ( auto& decl : m_ClassDeclarations )
	{
		source += decl;
	}

	// End extern C
	source += "\n#ifdef __cplusplus\n} // extern \"C\"\n#endif // __cplusplus\n\n";

	// Wrapper constructors declarations
	for ( auto& constr : m_WrapperDeclarations )
	{
		source += constr;
	}

	// End guards
	source += "\n#endif // PYSPOT_BINDINGS_H_\n";

	file << source;
}


void Printer::printBindingsSource( StringRef name )
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


void Printer::printExtensionHeader( StringRef name )
{
	OPEN_FILE_STREAM( file, name );

	std::string source
	{
		// Guards
		"#ifndef PYSPOT_EXTENSION_H_\n"
		"#define PYSPOT_EXTENSION_H_\n\n"
		"#include <Python.h>\n\n"

		// Extern C
		"\n#ifdef __cplusplus\nextern \"C\" {\n#endif // __cplusplus\n\n"

		"extern PyObject* g_pPyspotError;\n"
		"extern char g_aPyspotErrorName[13];\n"
		"extern char g_aErrorName[6];\n\n"
		"struct ModuleState\n{\n\tPyObject* error;\n};\n\n"
		"extern char g_aPyspotDescription[7];\n\n"
#if PY_MAJOR_VERSION >= 3
		"extern PyModuleDef g_sModuleDef;\n\n"
#endif

		// End extern C
		"\n#ifdef __cplusplus\n} // extern \"C\"\n#endif // __cplusplus\n\n"

		"PyMODINIT_FUNC PyInit_" +
		    m_ExtensionName +
		    "();\n\n"

		    // End guards
		    "#endif // PYSPOT_EXTENSION_H_\n"
	};

	file << source;
}


void Printer::printExtensionSource( StringRef name )
{
	OPEN_FILE_STREAM( file, name );

	std::string source
	{
		"#include \"" + name.slice( 4, name.size() - 3 ).str() +
		    "h\"\n\n"
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
		    // Extension function
		    "PyMODINIT_FUNC PyInit_" +
		    m_ExtensionName +
		    "()\n{\n"
		    "\t// Create the module\n"
		    "\tPyObject* pModule { "
#if PY_VERSION >= 3
		    "PyModule_Create( &g_sModuleDef )
#else
		    "Py_InitModule( \"" +
		    m_ExtensionName +
		    "\", nullptr )"
#endif
		    "};\n"
		    "\tif ( pModule == nullptr )\n\t{\n\t\treturn;\n\t}\n\n"
		    "\t// Module exception\n"
		    "\tg_pPyspotError = PyErr_NewException( g_aPyspotErrorName, nullptr, nullptr );\n"
		    "\tPy_INCREF( g_pPyspotError );\n"
		    "\tPyModule_AddObject( pModule, g_aErrorName, g_pPyspotError );\n\n"
	};

	// Add objects to module
	for ( auto& reg : m_ClassRegistrations )
	{
		source += reg;
	}

	// Add enumerators
	for ( auto& item : m_Enumerators )
	{
		source += item;
	}

	// Return the module
	source +=
#if PY_MAJOR_VERSION >= 3
	    "\treturn pModule;\n"
#endif
	    "}\n";

	file << source;
}


void Printer::PrintOut()
{
	llvm::sys::fs::create_directory( "include" );
	llvm::sys::fs::create_directory( "src" );
	llvm::sys::fs::create_directory( "include/pyspot" );
	llvm::sys::fs::create_directory( "src/pyspot" );
	printBindingsHeader( "include/pyspot/Bindings.h" );
	printBindingsSource( "src/pyspot/Bindings.cpp" );
	printExtensionHeader( "include/pyspot/Extension.h" );
	printExtensionSource( "src/pyspot/Extension.cpp" );
}


}  // namespace pyspot
