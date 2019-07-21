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
	FrontendAction( std::unordered_map<std::string, binding::Module>& m ) : modules{ m }
	{
	}

	const std::vector<std::string>& get_global_includes() const
	{
		return global_includes;
	}

	/// Creates a consumer
	std::unique_ptr<clang::ASTConsumer> CreateASTConsumer( clang::CompilerInstance& compiler,
	                                                       llvm::StringRef          file ) override;

	/// Starts handling a source file
	bool BeginSourceFileAction( clang::CompilerInstance& compiler ) override;

  private:
	/// Map to be populated by the consumer
	std::unordered_map<std::string, binding::Module>& modules;

	std::vector<std::string> global_includes;
};


/// This factory creates an action which populates the modules map
class FrontendActionFactory : public clang::tooling::FrontendActionFactory
{
  public:
	FrontendAction* create() override
	{
		return new FrontendAction{ modules };
	}

	/// @return The modules created by the action
	std::unordered_map<std::string, binding::Module>& get_modules()
	{
		return modules;
	};

  private:
	std::unordered_map<std::string, binding::Module> modules;
};


}  // namespace pywrap

#endif  // PYWRAP_FRONTEND_ACTION_H_
