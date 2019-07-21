#include "pywrap/Pywrap.h"

#include <memory>

#include "clang/Driver/Options.h"
#include "llvm/Option/OptTable.h"


int main( int argc, const char** argv )
{
	// Parse the command-line args passed to your code
	llvm::cl::OptionCategory            pyspot_category{ "Pyspot options" };
	clang::tooling::CommonOptionsParser op{ argc, argv, pyspot_category };

	// Create a new Clang Tool instance (a LibTooling environment)
	clang::tooling::ClangTool tool{ op.getCompilations(), op.getSourcePathList() };

	// Run the Clang Tool, creating a new FrontendAction
	pywrap::FrontendActionFactory factory{};
	auto                          result = tool.run( &factory );
	if ( result == EXIT_SUCCESS )
	{
		// This is going to write code for us
		pywrap::Printer printer{};
		printer.print_out( factory.get_modules() );
	}

	return result;
}
