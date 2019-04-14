#ifndef PYWRAP_FRONTEND_ACTION_H_
#define PYWRAP_FRONTEND_ACTION_H_

#include <string>
#include <unordered_map>
#include <vector>

#include <clang/Frontend/FrontendAction.h>
#include <clang/Tooling/Tooling.h>

#include "pywrap/Printer.h"

#include "pywrap/binding/Module.h"

namespace pywrap
{
class FrontendAction : public clang::ASTFrontendAction
{
  public:
	FrontendAction( std::unordered_map<const clang::NamespaceDecl*, binding::Module>& m, Printer& printer )
	    : modules{ m }, m_Printer{ printer }
	{
	}

	const std::vector<std::string>& GetGlobalIncludes() const
	{
		return m_GlobalIncludes;
	}

	void AddGlobalInclude( const std::string& searchPath );
	// TODO remove
	void AddClassInclude( const std::string& include )
	{
	}
	void AddClassDeclaration( const std::string& str )
	{
		m_Printer.AddClassDeclaration( str );
	}
	void AddClassDefinition( const std::string& str )
	{
		m_Printer.AddClassDefinition( str );
	}
	void AddClassRegistration( const std::string& str )
	{
		m_Printer.AddClassRegistration( str );
	}
	void AddEnumerator( const std::string& str )
	{
		m_Printer.AddEnumerator( str );
	}
	void AddWrapperDeclaration( const std::string& str )
	{
		m_Printer.AddWrapperDeclaration( str );
	}
	void AddWrapperDefinition( const std::string& str )
	{
		m_Printer.AddWrapperDefinition( str );
	}
	void AddHandled( const std::string& name )
	{
		m_Printer.AddHandled( name );
	}

	const std::vector<std::string>& GetHandled() const
	{
		return m_Printer.GetHandled();
	}

	/// Creates a consumer
	std::unique_ptr<clang::ASTConsumer> CreateASTConsumer( clang::CompilerInstance& compiler,
	                                                       llvm::StringRef          file ) override;

	/// Starts handling a source file
	bool BeginSourceFileAction( clang::CompilerInstance& compiler ) override;


  private:
	/// Map to be populated by the consumer
	std::unordered_map<const clang::NamespaceDecl*, binding::Module>& modules;

	Printer& m_Printer;

	std::vector<std::string> m_GlobalIncludes;
};


/// This factory creates an action which populates the modules map
class FrontendActionFactory : public clang::tooling::FrontendActionFactory
{
  public:
	FrontendActionFactory( Printer& printer ) : m_Printer{ printer }
	{
	}
	FrontendAction* create() override
	{
		return new FrontendAction{ modules, m_Printer };
	}

	/// @return The modules created by the action
	std::unordered_map<const clang::NamespaceDecl*, binding::Module>& get_modules()
	{
		return modules;
	};

  private:
	Printer& m_Printer;

	std::unordered_map<const clang::NamespaceDecl*, binding::Module> modules;
};


}  // namespace pywrap

#endif  // PYWRAP_FRONTEND_ACTION_H_
