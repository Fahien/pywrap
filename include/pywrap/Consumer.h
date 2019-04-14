#pragma once

#include <clang/AST/ASTConsumer.h>
#include <clang/ASTMatchers/ASTMatchers.h>

#include "pywrap/FrontendAction.h"
#include "pywrap/MatchHandler.h"

namespace pywrap
{
// @brief Implementation of the ASTConsumer interface for reading an AST produced
// by the Clang parser. It registers a couple of matchers and runs them on the AST.
class Consumer : public clang::ASTConsumer
{
  public:
	Consumer( std::unordered_map<std::string, binding::Module>& m, FrontendAction& frontend );

	void HandleTranslationUnit( clang::ASTContext& context ) override;

  private:
	MatchHandler m_Handler;

	clang::ast_matchers::MatchFinder m_Matcher;
};


}  // namespace pywrap
