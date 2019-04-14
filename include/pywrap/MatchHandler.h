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
	MatchHandler( std::unordered_map<const clang::NamespaceDecl*, binding::Module>& m, FrontendAction& );

	/// @return The cwd using forward slashes
	static std::string getCwd();

	/// @brief Handles a match
	/// @param[in] result Match result for pyspot attribute
	virtual void run( const clang::ast_matchers::MatchFinder::MatchResult& );

	/// @brief Generates python bindings for a @ref clang::Decl
	/// @param[in] decl A Decl which can be a variable, a function, a struct, ...
	void generate_bindings( const clang::Decl* decl );

	/// @brief Generate python bindings for a @ref clang::FunctionDecl
	/// @param function A FunctionDecl to wrap
	void generate_function_bindings( const clang::FunctionDecl* function );

	/// @brief Handles a tag
	/// @param[in] pTag Pointer to a tag decl
	/// @paramp[in] tmap A template map (parameter, argument)
	void handleTag( const clang::TagDecl*, TemplateMap&& tMap = TemplateMap{} );

  private:
	/// Creates a module for the context if it does not already exist
	/// @param[in] ctx Declaration context to become a module
	binding::Module& get_module( const clang::DeclContext* ctx );

	/// Map of global id of the DeclContext and the associated Module
	std::unordered_map<const clang::NamespaceDecl*, binding::Module>& modules;

	/// @param[in] decl Decl we want to get the path
	/// @return A proper include path
	std::string get_include_path( const clang::Decl* const decl );

	/// Generates a binding instance
	/// @param[in] decl The decl to wrap
	/// @param[in] parent Parent of the decl
	template <typename B, typename D>
	B create_binding( const D* decl, const binding::Binding& parent );

	struct PyDecl
	{
		void Flush( FrontendAction& );

		std::string name;
		std::string declaration;
		std::string definition;
	};

	struct PyTag;
	struct PyClass;

	struct PyDestructor : PyDecl
	{
		PyDestructor( const PyTag& );
	};

	struct PyCompare : PyDecl
	{
		PyCompare( const PyTag& );
		const PyTag& owner;

		void Add( const clang::OverloadedOperatorKind );
		void Flush( FrontendAction& );
	};

	struct PyMethod
	{
		PyMethod( const PyTag&, const clang::CXXMethodDecl* const );

		const PyTag& owner;
		std::string  name;
		std::string  qualifiedName;

		/// Array of chars with the name
		PyDecl nameDecl;
		/// Method wrapper
		PyDecl methodDecl;

		void Flush( FrontendAction& action );
	};

	struct PyMethods : PyDecl
	{
		PyMethods( const PyTag& );

		/// List of methods
		std::vector<PyMethod> methods;

		/// Adds a method to the list
		/// @return Whether it has been successful or not
		bool Add( const PyMethod& );
		void Flush( FrontendAction& action );
	};

	struct PyField
	{
		PyField( const PyTag&, const clang::FieldDecl* const );

		const clang::FieldDecl* pField;
		const PyTag&            owner;
		PyDecl                  decl;
		std::string             name;  // field name
		std::string             type;
	};

	struct PyGetter
	{
		PyGetter( PyField& );
		PyDecl decl;
	};

	struct PySetter
	{
		PySetter( PyField& );
		PyDecl decl;
	};

	struct PyInit : PyDecl
	{
		PyInit( const PyTag& );

		void Add( const clang::CXXConstructorDecl* );

		void Flush( FrontendAction& );

		const PyTag& owner;
	};

	struct PyAccessors : PyDecl
	{
		PyAccessors( const PyTag& );

		/// Close flag
		bool closed = false;
		/// Number of fields
		size_t count = 0;

		void Add( const PyField&, const PyGetter&, const PySetter& );
		void Flush( FrontendAction& );
	};

	struct PyMembers : PyDecl
	{
		PyMembers( const PyTag& );
	};

	struct PyTypeObj : public PyDecl
	{
		PyTypeObj( const PyTag& );
	};

	struct PyEnums
	{
		PyEnums( const PyTag& );

		void Flush( FrontendAction& );

		std::string definition;
	};

	struct PyTag
	{
		PyTag( const clang::ASTContext*, const clang::TagDecl* const, TemplateMap&& pTemplateArgs = TemplateMap{} );

		/// @return Class decl pointer if it is an enum, or nullptr
		const clang::CXXRecordDecl* AsClass() const;
		/// @return Enum decl pointer if it is an enum, or nullptr
		const clang::EnumDecl* AsEnum() const;

		/// @return The template argument for the parameter
		const clang::TemplateArgument* GetTemplateArg( const std::string& type ) const;

		/// @return The template map (parameter, argument)
		const TemplateMap& GetTemplateMap() const
		{
			return m_TemplateMap;
		}

		/// @brief Generates bindings and flushes
		void Flush( FrontendAction& );

		/// @brief Creates a string of angled template arguments
		std::string createAngledArgs();

		/// Decl
		const clang::ASTContext*    pContext;
		const clang::TagDecl* const pDecl;

		/// Map template (parameter, argument)
		const TemplateMap m_TemplateMap;
		const std::string m_AngledArgs;

		/// Simple name
		std::string name;
		/// Qualified name like wrap::Test
		std::string qualifiedName;
		/// Pyspot name like PywrapTest
		std::string pyName;

		PyInit       init;
		PyDestructor destructor;
		PyCompare    compare;
		PyMethods    methods;
		PyAccessors  accessors;
		PyMembers    members;
		PyTypeObj    typeObject;
		PyEnums      enums;
	};

	/// @return True if this tag has already been handled
	bool checkHandled( const PyTag& );

	struct PyClass : PyTag
	{
		PyClass( const clang::ASTContext*, const clang::CXXRecordDecl* const );

		/// Whether it has a default constructor
		bool bHasDefaultConstructor;
		/// Whether it has a copy constructor
		bool bHasCopyConstructor;
		/// Whether it has a move constructor
		bool bHasMoveConstructor;
	};

	struct WrapperConstructors
	{
		WrapperConstructors( const PyTag&, const std::string& );

		PyDecl copy;
		PyDecl pointer;
		PyDecl move;

		void Flush( FrontendAction& );
	};

	clang::ASTContext* m_pContext = nullptr;

	/// Frontend observer to notify as we process classes
	FrontendAction& m_Frontend;

	/// Current working directory
	std::string m_Cwd;

	/// Templates decls
	std::vector<clang::ClassTemplateDecl*> m_Templates;
};


}  // namespace pywrap
