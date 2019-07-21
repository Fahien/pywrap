#include "pywrap/Consumer.h"

namespace pywrap
{
Consumer::Consumer( std::unordered_map<std::string, binding::Module>& m, FrontendAction& frontend ) : handler{ m, frontend }
{
	// Match classes with pyspot attribute
	auto hasPyspot = clang::ast_matchers::hasAttr( clang::attr::Annotate );
	auto pyMatcher = clang::ast_matchers::decl( hasPyspot ).bind( "PyspotTag" );
	matcher.addMatcher( pyMatcher, &handler );
}


void Consumer::HandleTranslationUnit( clang::ASTContext& context )
{
	// Run the matchers when we have the whole TU parsed
	matcher.matchAST( context );
}


}  // namespace pywrap
