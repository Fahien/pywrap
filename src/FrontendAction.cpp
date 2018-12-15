#include "pywrap/FrontendAction.h"

#include <clang/Frontend/CompilerInstance.h>
#include <clang/Lex/HeaderSearch.h>

#include "pywrap/Consumer.h"
#include "pywrap/Util.h"


namespace pyspot
{


void FrontendAction::AddGlobalInclude( const std::string& searchPath )
{
	m_GlobalIncludes.emplace_back( searchPath );
}


std::unique_ptr<clang::ASTConsumer> FrontendAction::CreateASTConsumer( clang::CompilerInstance& compiler, StringRef file )
{
	return llvm::make_unique<Consumer>( *this );
}


bool FrontendAction::BeginSourceFileAction( clang::CompilerInstance& compiler )
{
	// Before executing the action get the global includes
	auto& preprocessor = compiler.getPreprocessor();
	auto& info = preprocessor.getHeaderSearchInfo();
	for ( auto dir = info.search_dir_begin(); dir != info.search_dir_end(); ++dir )
	{
		std::string directory = dir->getName();
		replace_all( directory, "\\", "/" );
		AddGlobalInclude( directory );
	}

	return clang::FrontendAction::BeginSourceFileAction( compiler );
}


} // namespace pyspot