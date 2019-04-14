#pragma once
#include <string>
#include <vector>

#include <clang/Tooling/Tooling.h>

#include <pywrap/binding/Module.h>

namespace pywrap
{
class Printer
{
  public:
	Printer( const std::string& name = "extension" ) : m_ExtensionName{ name }
	{
	}

	/// Adds an include path to the set of processed ones
	/// @param[in] include Include path
	void add_include( const std::string& include )
	{
		processed_includes.emplace( include );
	}

	void AddClassDeclaration( const std::string& str )
	{
		m_ClassDeclarations.emplace_back( str );
	}

	void AddClassDefinition( const std::string& str )
	{
		m_ClassDefinitions.emplace_back( str );
	}

	void AddClassRegistration( const std::string& str )
	{
		m_ClassRegistrations.emplace_back( str );
	}

	void AddEnumerator( const std::string& str )
	{
		m_Enumerators.emplace_back( str );
	}

	void AddWrapperDeclaration( const std::string& str )
	{
		m_WrapperDeclarations.emplace_back( str );
	}

	void AddWrapperDefinition( const std::string& str )
	{
		m_WrapperDefinitions.emplace_back( str );
	}

	void AddHandled( const std::string& name )
	{
		m_Handled.emplace_back( name );
	}

	const std::vector<std::string>& GetHandled() const
	{
		return m_Handled;
	}

	/// @brief Finishes handling the files
	void PrintOut( const std::unordered_map<const clang::NamespaceDecl*, binding::Module>& modules );

  private:
	/// @brief Prints bindings header
	/// @param[in] name Path of the output file
	void printBindingsHeader( llvm::StringRef name );

	/// @brief Prints bindings source
	/// @param[in] name Path of the output file
	void printBindingsSource( llvm::StringRef name );

	/// @brief Prints extension header
	/// @param[in] name Path of the output file
	void printExtensionHeader( llvm::StringRef name );

	/// @brief Prints extension source
	/// @param[in] name Path of the output file
	void printExtensionSource( llvm::StringRef name );

	/// Recursively process includes for a module and its submodules
	/// @param[in] file The current output stream
	/// @param[in] module The current module to process
	void process_includes( llvm::raw_fd_ostream& file, const binding::Module& module );

	/// Recursively process declarations for a module and its submodules
	/// @param[in] file The current output stream
	/// @param[in] module The current module to process
	void process_decls( llvm::raw_fd_ostream& file, const binding::Module& module );

	/// Recursively process wrappers for a module and its submodules
	/// @param[in] file The current output stream
	/// @param[in] module The current module to process
	void process_wrappers( llvm::raw_fd_ostream& file, const binding::Module& module );

	/// Recursively process definitions for a module and its submodules
	/// @param[in] file The current output stream
	/// @param[in] module The current module to process
	void process_defs( llvm::raw_fd_ostream& file, const binding::Module& module );

	const std::unordered_map<const clang::NamespaceDecl*, binding::Module>* modules;

	/// Extension name
	const std::string m_ExtensionName;

	/// List of already handled class names
	std::vector<std::string> m_Handled;

	std::set<std::string>    processed_includes;
	std::vector<std::string> m_ClassDeclarations;
	std::vector<std::string> m_ClassDefinitions;
	std::vector<std::string> m_ClassRegistrations;
	std::vector<std::string> m_Enumerators;
	std::vector<std::string> m_WrapperDeclarations;
	std::vector<std::string> m_WrapperDefinitions;
};


}  // namespace pywrap
