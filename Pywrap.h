#ifndef PYSPOT_PYWRAP_H_
#define PYSPOT_PYWRAP_H_

#include <sstream>
#include <string>
#include <set>
#include <llvm/Support/FileSystem.h>

#include <clang/Frontend/CompilerInstance.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>

#include "Util.h"

using namespace clang;


// Classes to be mapped to Pyspot
struct OutputStreams
{
	std::string headerString { "" };
	llvm::raw_string_ostream headerOS { headerString };
};


class Printer
{
  public:
	Printer();

	void AddClassInclude( const std::string& include )   { m_ClassIncludes.emplace( include ); }
	void AddClassDeclaration( const std::string& str )   { m_ClassDeclarations.emplace_back( str ); }
	void AddClassDefinition( const std::string& str )    { m_ClassDefinitions.emplace_back( str ); }
	void AddClassRegistration( const std::string& str )  { m_ClassRegistrations.emplace_back( str ); }
	void AddEnumerator( const std::string& str )         { m_Enumerators.emplace_back( str ); }
	void AddWrapperDeclaration( const std::string& str ) { m_WrapperDeclarations.emplace_back( str ); }
	void AddWrapperDefinition( const std::string& str )  { m_WrapperDefinitions.emplace_back( str ); }
	void AddHandled( const std::string& name )           { m_Handled.emplace_back( name ); }
	const std::vector<std::string>& GetHandled() const { return m_Handled; }

	/// @brief Finishes handling the files
	void PrintOut();

  private:

	/// @brief Prints bindings header
	/// @param[in] name Path of the output file
	void printBindingsHeader( StringRef name );

	/// @brief Prints bindings source
	/// @param[in] name Path of the output file
	void printBindingsSource( StringRef name );

	/// @brief Prints extension header
	/// @param[in] name Path of the output file
	void printExtensionHeader( StringRef name );

	/// @brief Prints extension source
	/// @param[in] name Path of the output file
	void printExtensionSource( StringRef name );

	/// List of already handled class names
	std::vector<std::string> m_Handled;

	std::set<std::string> m_ClassIncludes;
	std::vector<std::string> m_ClassDeclarations;
	std::vector<std::string> m_ClassDefinitions;
	std::vector<std::string> m_ClassRegistrations;
	std::vector<std::string> m_Enumerators;
	std::vector<std::string> m_WrapperDeclarations;
	std::vector<std::string> m_WrapperDefinitions;
};


class PyspotFrontendAction : public ASTFrontendAction
{
  public:
	PyspotFrontendAction();

	const std::vector<std::string>& GetGlobalIncludes() const { return m_GlobalIncludes; }

	void AddGlobalInclude( const std::string& searchPath );
	void AddClassInclude( const std::string& include )   { m_Printer.AddClassInclude( include ); }
	void AddClassDeclaration( const std::string& str )   { m_Printer.AddClassDeclaration( str ); }
	void AddClassDefinition( const std::string& str )    { m_Printer.AddClassDefinition( str ); }
	void AddClassRegistration( const std::string& str )  { m_Printer.AddClassRegistration( str ); }
	void AddEnumerator( const std::string& str )         { m_Printer.AddEnumerator( str ); }
	void AddWrapperDeclaration( const std::string& str ) { m_Printer.AddWrapperDeclaration( str ); }
	void AddWrapperDefinition( const std::string& str )  { m_Printer.AddWrapperDefinition( str ); }
	void AddHandled( const std::string& name )           { m_Printer.AddHandled( name ); }

	const std::vector<std::string>& GetHandled() const { return m_Printer.GetHandled(); }

	/// @brief Creates a consumer
	std::unique_ptr<ASTConsumer> CreateASTConsumer( CompilerInstance& compiler, StringRef file ) override;

	/// @brief Starts handling a source file
	bool BeginSourceFileAction( CompilerInstance& compiler ) override;


  private:
	Printer& m_Printer;

	std::vector<std::string> m_GlobalIncludes;
};


class PyspotMatchHandler : public ast_matchers::MatchFinder::MatchCallback
{
  public:
	/// @brief Constructs the handler for a Pyspot class match
	/// @param[in] includes Vector to put headers to include
	/// @param[in] classes Vector to put bindings of our classes
	PyspotMatchHandler( PyspotFrontendAction& );

	/// @return The cwd using forward slashes
	static std::string getCwd();

	/// @param[in] name Qualified class name
	/// @return A class name for Pyspot
	static std::string toPyspotName( std::string );

	/// @brief Handles a match
	/// @param[in] result Match result for pyspot attribute
	virtual void run( const ast_matchers::MatchFinder::MatchResult& );

	virtual void onEndOfTranslationUnit() {}

  private:

	/// @param[in] manager Context SourceManager
	/// @param[in] pDecl Pointer to the decl we want to get the path
	/// @return A proper include path
	std::string getIncludePath( const Decl* const pDecl );

	struct PyDecl
	{
		void Flush( PyspotFrontendAction& );

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

		void Add( const OverloadedOperatorKind );
		void Flush( PyspotFrontendAction& );
	};

	struct PyMethod
	{
		PyMethod( const PyTag&, const CXXMethodDecl* const );

		const PyTag& owner;
		std::string name;
		std::string qualifiedName;
		
		/// Array of chars with the name
		PyDecl nameDecl;
		/// Method wrapper
		PyDecl methodDecl;

		void Flush( PyspotFrontendAction& action );
	};

	struct PyMethods : PyDecl
	{
		PyMethods( const PyTag& );

		/// Number of fields
		size_t count = 0;

		void Add( const PyMethod& );
		void Flush( PyspotFrontendAction& action );
	};

	struct PyField
	{
		PyField( const PyTag&, const FieldDecl* const );

		const PyTag& owner;
		PyDecl decl;
		std::string name; // field name
		std::string type;
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

		void Add( const CXXConstructorDecl* );

		void Flush( PyspotFrontendAction& );

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
		void Flush( PyspotFrontendAction& );
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

		void Flush( PyspotFrontendAction& );

		std::string definition;
	};

	struct PyTag
	{
		PyTag( const TagDecl* const );

		/// @return Class decl pointer if it is an enum, or nullptr
		const CXXRecordDecl* AsClass() const;
		/// @return Enum decl pointer if it is an enum, or nullptr
		const EnumDecl* AsEnum() const;

		/// @brief Generates bindings and flushes
		void Flush( PyspotFrontendAction& );

		// Decl
		const TagDecl* const pDecl;

		// Simple name
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
		PyClass( const CXXRecordDecl* const );

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

		void Flush( PyspotFrontendAction& );
	};

	ASTContext* m_pContext = nullptr;

	/// Frontend observer to notify as we process classes
	PyspotFrontendAction& m_Frontend;

	/// Current working directory
	std::string m_Cwd;
};


// @brief Implementation of the ASTConsumer interface for reading an AST produced
// by the Clang parser. It registers a couple of matchers and runs them on the AST.
class PyspotConsumer : public ASTConsumer
{
  public:
	PyspotConsumer( PyspotFrontendAction& frontend );

	void HandleTranslationUnit( ASTContext& context ) override;

  private:
	PyspotMatchHandler m_HandlerForClassMatcher;

	ast_matchers::MatchFinder m_Matcher;
};

#endif // PYSPOT_PYWRAP_H_
