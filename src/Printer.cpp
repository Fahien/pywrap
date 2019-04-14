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

void Printer::process_includes( llvm::raw_fd_ostream& file, const binding::Module& module )
{
	// Nested modules
	for ( auto& child : module.get_modules() )
	{
		process_includes( file, child );
	}

	auto process_include = [&file, this]( const binding::Binding& b ) {
		auto incl = b.get_incl();
		auto it   = processed_includes.find( incl );
		if ( it == processed_includes.end() )
		{
			file << "#include \"" << incl << "\"\n";
			processed_includes.emplace( incl );
		}
	};

	for ( auto& function : module.get_functions() )
	{
		process_include( function );
	}

	for ( auto& en : module.get_enums() )
	{
		process_include( en );
	}

	for ( auto& record : module.get_records() )
	{
		process_include( record );
	}
}

void Printer::process_decls( llvm::raw_fd_ostream& file, const binding::Module& module )
{
	// Nested modules
	for ( auto& child : module.get_modules() )
	{
		process_decls( file, child );
	}

	auto print_decl = [&file]( const binding::Binding& b ) { file << b.get_decl() << '\n'; };

	// Functions
	auto& functions = module.get_functions();
	std::for_each( std::begin( functions ), std::end( functions ), print_decl );

	// Enums
	auto& enums = module.get_enums();
	std::for_each( std::begin( enums ), std::end( enums ), print_decl );

	// Structs, unions, classes
	auto& records = module.get_records();
	std::for_each( std::begin( records ), std::end( records ), print_decl );
}


void Printer::process_wrappers( llvm::raw_fd_ostream& file, const binding::Module& module )
{
	// Nested modules
	for ( auto& child : module.get_modules() )
	{
		process_wrappers( file, child );
	}

	auto print_wrapper_decl = [&file]( const binding::Tag& b ) { file << b.get_wrapper().get_decl() << '\n'; };

	auto& enums = module.get_enums();
	std::for_each( std::begin( enums ), std::end( enums ), print_wrapper_decl );

	auto& records = module.get_records();
	std::for_each( std::begin( records ), std::end( records ), print_wrapper_decl );
}

void Printer::printBindingsHeader( llvm::StringRef name )
{
	OPEN_FILE_STREAM( file, name );

	// Guards
	file << "#ifndef PYSPOT_BINDINGS_H_\n#define PYSPOT_BINDINGS_H_\n\n";

	// Includes
	for ( auto& pr : *modules )
	{
		process_includes( file, pr.second );
	}

	// Tail includes
	file << "\n#include <pyspot/Wrapper.h>\n#include <structmember.h>\n\n";

	// Extern C
	file << "\n#ifdef __cplusplus\nextern \"C\" {\n#endif // __cplusplus\n\n";

	// Declarations
	for ( auto& pr : *modules )
	{
		process_decls( file, pr.second );
	}

	// End extern C
	file << "\n#ifdef __cplusplus\n} // extern \"C\"\n#endif // __cplusplus\n\n";

	// Wrappers
	for ( auto& pr : *modules )
	{
		process_wrappers( file, pr.second );
	}

	// End guards
	file << "\n#endif // PYSPOT_BINDINGS_H_\n";
}

void Printer::process_defs( llvm::raw_fd_ostream& file, const binding::Module& module )
{
	// Nested modules
	for ( auto& child : module.get_modules() )
	{
		process_defs( file, child );
	}

	auto print_def = [&]( const binding::Binding& b ) { file << b.get_def() << '\n'; };

	// Functions
	auto& functions = module.get_functions();
	std::for_each( functions.begin(), functions.end(), print_def );

	// Enums
	auto& enums = module.get_enums();
	std::for_each( enums.begin(), enums.end(), print_def );

	// CXXRecord
	auto& records = module.get_records();
	std::for_each( records.begin(), records.end(), print_def );
}


void Printer::printBindingsSource( llvm::StringRef name )
{
	OPEN_FILE_STREAM( file, name );

	auto include = name.slice( 4, name.size() - 3 );
	file << "#include \"" << include.str() << "h\"\n\n#include <Python.h>\n#include <pyspot/String.h>\n\n\n";

	for ( auto& pr : *modules )
	{
		process_defs( file, pr.second );
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
	     << "#include \"pyspot/Bindings.h\"\n\n"
	     << "struct ModuleState\n{\n"
	     << "\tPyObject* error;\n};\n\n";

	std::function<void( const binding::Module& )> process_module_defs =
	    [&file, &process_module_defs]( const binding::Module& module ) {
		    for ( auto& child : module.get_modules() )
		    {
			    process_module_defs( child );
		    }
		    file << module.get_methods().get_def();
		    file << module.get_def();
	    };

	for ( auto& pr : *modules )
	{
		process_module_defs( pr.second );
	}
}


void Printer::PrintOut( const std::unordered_map<std::string, binding::Module>& m )
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
