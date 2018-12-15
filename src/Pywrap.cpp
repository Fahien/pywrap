#include "pywrap/Pywrap.h"

#include <memory>

#include "clang/Driver/Options.h"
#include "llvm/Option/OptTable.h"


int main( int argc, const char** argv )
{
	// Applied to all command-line options
	std::unique_ptr<llvm::opt::OptTable> optTable { clang::driver::createDriverOptTable() };
	llvm::cl::opt<std::string> extensionName { "o", llvm::cl::desc( "Extension name" ) };

	// Parse the command-line args passed to your code
	llvm::cl::OptionCategory pyspotCategory { "Pyspot options" };
	clang::tooling::CommonOptionsParser op { argc, argv, pyspotCategory };
	// This is going to write code for us
	pyspot::Printer printer { extensionName.getValue() };

	// Create a new Clang Tool instance (a LibTooling environment)
	clang::tooling::ClangTool tool { op.getCompilations(), op.getSourcePathList() };

	// Run the Clang Tool, creating a new FrontendAction
	pyspot::FrontendActionFactory factory { printer };
	auto result = tool.run( &factory );
	if ( result == EXIT_SUCCESS )
	{
		printer.PrintOut();
	}

	return result;
}