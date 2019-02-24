#pragma once
#include <string>
#include <vector>

#include <clang/Tooling/Tooling.h>


namespace pyspot
{
class Printer
{
  public:
	Printer( const std::string& name = "extension" ) : m_ExtensionName{ name } {}

	void AddClassInclude( const std::string& include ) { m_ClassIncludes.emplace( include ); }
	void AddClassDeclaration( const std::string& str ) { m_ClassDeclarations.emplace_back( str ); }
	void AddClassDefinition( const std::string& str ) { m_ClassDefinitions.emplace_back( str ); }
	void AddClassRegistration( const std::string& str ) { m_ClassRegistrations.emplace_back( str ); }
	void AddEnumerator( const std::string& str ) { m_Enumerators.emplace_back( str ); }
	void AddWrapperDeclaration( const std::string& str ) { m_WrapperDeclarations.emplace_back( str ); }
	void AddWrapperDefinition( const std::string& str ) { m_WrapperDefinitions.emplace_back( str ); }
	void AddHandled( const std::string& name ) { m_Handled.emplace_back( name ); }
	const std::vector<std::string>& GetHandled() const { return m_Handled; }

	/// @brief Finishes handling the files
	void PrintOut();

  private:
	/// @brief Prints bindings header
	/// @param[in] name Path of the output file
	void printBindingsHeader( StringRef name );

	/// @brief Prints bindings source
	/// @param[in] name Path of the output file
	void printBindingsSource( StringRef name );

	/// @brief Prints extension header
	/// @param[in] name Path of the output file
	void printExtensionHeader( StringRef name );

	/// @brief Prints extension source
	/// @param[in] name Path of the output file
	void printExtensionSource( StringRef name );

	/// Extension name
	const std::string m_ExtensionName;

	/// List of already handled class names
	std::vector<std::string> m_Handled;

	std::set<std::string>    m_ClassIncludes;
	std::vector<std::string> m_ClassDeclarations;
	std::vector<std::string> m_ClassDefinitions;
	std::vector<std::string> m_ClassRegistrations;
	std::vector<std::string> m_Enumerators;
	std::vector<std::string> m_WrapperDeclarations;
	std::vector<std::string> m_WrapperDefinitions;
};


}  // namespace pyspot
