#pragma once

#include <string>
#include <vector>

#include <clang/Frontend/FrontendAction.h>
#include <clang/Tooling/Tooling.h>

#include "pywrap/Printer.h"

namespace pyspot
{
class FrontendAction : public clang::ASTFrontendAction
{
  public:
	FrontendAction( Printer& printer ) : m_Printer{ printer } {}

	const std::vector<std::string>& GetGlobalIncludes() const { return m_GlobalIncludes; }

	void AddGlobalInclude( const std::string& searchPath );
	void AddClassInclude( const std::string& include ) { m_Printer.AddClassInclude( include ); }
	void AddClassDeclaration( const std::string& str ) { m_Printer.AddClassDeclaration( str ); }
	void AddClassDefinition( const std::string& str ) { m_Printer.AddClassDefinition( str ); }
	void AddClassRegistration( const std::string& str ) { m_Printer.AddClassRegistration( str ); }
	void AddEnumerator( const std::string& str ) { m_Printer.AddEnumerator( str ); }
	void AddWrapperDeclaration( const std::string& str ) { m_Printer.AddWrapperDeclaration( str ); }
	void AddWrapperDefinition( const std::string& str ) { m_Printer.AddWrapperDefinition( str ); }
	void AddHandled( const std::string& name ) { m_Printer.AddHandled( name ); }

	const std::vector<std::string>& GetHandled() const { return m_Printer.GetHandled(); }

	/// @brief Creates a consumer
	std::unique_ptr<clang::ASTConsumer> CreateASTConsumer( clang::CompilerInstance& compiler, StringRef file ) override;

	/// @brief Starts handling a source file
	bool BeginSourceFileAction( clang::CompilerInstance& compiler ) override;


  private:
	Printer& m_Printer;

	std::vector<std::string> m_GlobalIncludes;
};


class FrontendActionFactory : public clang::tooling::FrontendActionFactory
{
  public:
	FrontendActionFactory( Printer& printer ) : m_Printer{ printer } {}
	FrontendAction* create() override { return new FrontendAction( m_Printer ); }

  private:
	Printer& m_Printer;
};


}  // namespace pyspot
