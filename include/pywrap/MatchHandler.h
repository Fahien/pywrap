#pragma once

#include <string>

#include <clang/AST/Decl.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

#include "pywrap/FrontendAction.h"
#include "pywrap/Util.h"

#include "pywrap/binding/Module.h"

namespace pywrap
{
class MatchHandler : public clang::ast_matchers::MatchFinder::MatchCallback
{
  public:
	/// @brief Constructs the handler for a Pyspot class match
	/// @param[in] modules Map to populate with modules
	MatchHandler( std::unordered_map<std::string, binding::Module>& m, FrontendAction& );

	/// @brief Handles a match
	/// @param[in] result Match result for pyspot attribute
	virtual void run( const clang::ast_matchers::MatchFinder::MatchResult& );

	/// @brief Generate pythong bindings for a clang::QualType
	/// @param[in] type A QualType which can be any kind of type
	void generate_bindings( clang::QualType type );

	/// @brief Generates python bindings for a clang::Decl
	/// @param[in] decl A Decl which can be a variable, a function, a struct, ...
	void generate_bindings( const clang::Decl& decl );

	/// @brief Generate python bindings for a @ref clang::FunctionDecl
	/// @param function A FunctionDecl to wrap
	void generate_function_bindings( const clang::FunctionDecl* function );

  private:
	/// Creates a module for the context if it does not already exist
	/// @param[in] ctx Declaration context to become a module
	binding::Module& get_module( const clang::DeclContext& ctx );

	/// @param[in] decl Decl we want to get the path
	/// @return A proper include path
	std::string get_include_path( const clang::Decl& decl );

	/// Generates a binding instance
	/// @param[in] decl The decl to wrap
	/// @param[in] parent Parent of the decl
	template <typename B, typename D>
	B create_binding( const D& decl, const binding::Binding& parent );

	/// Map of global id of the DeclContext and the associated Module
	std::unordered_map<std::string, binding::Module>& modules;

	clang::ASTContext* context = nullptr;

	/// Frontend observer to notify as we process classes
	FrontendAction& frontend;
};


}  // namespace pywrap
