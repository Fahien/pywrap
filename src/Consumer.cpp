#include "pywrap/Consumer.h"

namespace pyspot
{


Consumer::Consumer( FrontendAction& frontend )
:	m_Handler { frontend }
{
	// Match classes with pyspot attribute
	auto hasPyspot = clang::ast_matchers::hasAttr( clang::attr::Pyspot );
	auto pyMatcher = clang::ast_matchers::decl( hasPyspot ).bind( "PyspotTag" );
	m_Matcher.addMatcher( pyMatcher, &m_Handler );
}


void Consumer::HandleTranslationUnit( clang::ASTContext& context )
{
	// Run the matchers when we have the whole TU parsed
	m_Matcher.matchAST( context );
}


} // namespace pyspot
