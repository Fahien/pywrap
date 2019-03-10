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
	for ( auto& pr : *modules )
	{
		auto process_include = [&]( const binding::Binding& b ) {
			auto incl = b.get_incl();
			auto it   = processed_includes.find( incl );
			if ( it == processed_includes.end() )
			{
				file << "#include \"" << incl << "\"\n";
				processed_includes.emplace( incl );
			}
		};

		auto& module = pr.second;

		auto& functions = module.get_functions();
		auto& enums     = module.get_enums();

		std::for_each( functions.begin(), functions.end(), process_include );
		std::for_each( enums.begin(), enums.end(), process_include );
	}

	// Tail includes
	file << "\n#include <pyspot/Wrapper.h>\n#include <structmember.h>\n\n";

	// Extern C
	file << "\n#ifdef __cplusplus\nextern \"C\" {\n#endif // __cplusplus\n\n";

	for ( auto& pr : *modules )
	{
		auto print_decl = [&]( const binding::Binding& b ) { file << b.get_decl() << '\n'; };

		auto& module = pr.second;

		// Functions
		auto& functions = module.get_functions();
		std::for_each( functions.begin(), functions.end(), print_decl );

		// Enums
		auto& enums = module.get_enums();
		std::for_each( enums.begin(), enums.end(), print_decl );
	}

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
	file << "#include \"" << include.str() << "h\"\n\n#include <Python.h>\n#include <pyspot/String.h>\n\n\n";

	for ( auto& pr : *modules )
	{
		auto print_def = [&]( const binding::Binding& b ) { file << b.get_def() << '\n'; };

		auto& module = pr.second;

		// Functions
		auto& functions = module.get_functions();
		std::for_each( functions.begin(), functions.end(), print_def );

		// Enums
		auto& enums = module.get_enums();
		std::for_each( enums.begin(), enums.end(), print_def );
	}

	// Class binding definitions
	for ( auto& def : m_ClassDefinitions )
	{
		file << def;
	}

	// Wrapper constructors definitions
	for ( auto& constr : m_WrapperDefinitions )
	{
		file << constr;
	}
}


void Printer::printExtensionHeader( llvm::StringRef name )
{
	OPEN_FILE_STREAM( file, name );

	// Guards
	file << "#ifndef PYSPOT_EXTENSION_H_\n"
	        "#define PYSPOT_EXTENSION_H_\n\n"
	        "#include <Python.h>\n\n";

	// Extern C
	file << "\n#ifdef __cplusplus\nextern \"C\" {\n#endif // __cplusplus\n\n";

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

	file << "#include \"" << name.slice( 4, name.size() - 3 ).str() << "h\"\n\n"
	     << "#include \"pyspot/Bindings.h\"\n\n";

	for ( auto& pr : *modules )
	{
		auto& module = pr.second;
		file << module.get_methods().get_def();
		file << module.get_def();
	}

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
