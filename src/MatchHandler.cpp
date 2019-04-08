#include "pywrap/MatchHandler.h"

#include <sstream>

#include "pywrap/binding/CXXRecord.h"
#include "pywrap/binding/Enum.h"
#include "pywrap/binding/Function.h"
#include "pywrap/binding/Module.h"

namespace pywrap
{
MatchHandler::MatchHandler( std::unordered_map<unsigned int, binding::Module>& m, FrontendAction& frontend )
    : modules{ m }, m_Frontend{ frontend }, m_Cwd{ getCwd() }
{
}

std::string MatchHandler::getCwd()
{
	llvm::SmallString<64> aCwd;
	llvm::sys::fs::current_path( aCwd );
	return pywrap::replace_all( aCwd.str().str(), "\\", "/" );
}

std::string MatchHandler::get_include_path( const clang::Decl* const pDecl )
{
	auto location = pDecl->getLocation().printToString( m_pContext->getSourceManager() );
	pywrap::replace_all( location, "\\", "/" );

	// Remove include directories from path
	for ( auto& path : m_Frontend.GetGlobalIncludes() )
	{
		auto found = location.find( path );

		if ( found == 0 )
		{
			auto cwdLen = path.size() + 1;  // remove '/' as well
			location    = location.substr( cwdLen, location.length() - cwdLen );
			break;
		}
	}

	// Remove everything after ":"
	auto found = location.find( ":" );
	if ( found != std::string::npos )
	{
		location = location.substr( 0, found );
	}

	return location;
}

MatchHandler::PyTag::PyTag( const clang::ASTContext* pCtx, const clang::TagDecl* const pTagDecl, TemplateMap&& tMap )
    : pContext{ pCtx }
    , pDecl{ pTagDecl }
    , m_TemplateMap{ std::move( tMap ) }
    , m_AngledArgs{ createAngledArgs() }
    , name{ pDecl->getNameAsString() + m_AngledArgs }
    , qualifiedName{ pDecl->getQualifiedNameAsString() + m_AngledArgs }
    , pyName{ to_pyspot_name( qualifiedName ) }
    , init{ *this }
    , destructor{ *this }
    , compare{ *this }
    , methods{ *this }
    , accessors{ *this }
    , members{ *this }
    , typeObject{ *this }
    , enums{ *this }
{
}

std::string MatchHandler::PyTag::createAngledArgs()
{
	std::ostringstream os{};

	if ( pDecl->isTemplated() )
	{
		os << "<";
		for ( auto& kv : m_TemplateMap )
		{
			// Argument type
			auto pArg = kv.second;
			os << pArg->getAsType().getAsString() << ",";
		}

		auto ret            = os.str();
		ret[ret.size() - 1] = '>';
		return ret;
	}

	return os.str();
}

const clang::EnumDecl* MatchHandler::PyTag::AsEnum() const
{
	return clang::dyn_cast<clang::EnumDecl>( pDecl );
}

const clang::CXXRecordDecl* MatchHandler::PyTag::AsClass() const
{
	return clang::dyn_cast<clang::CXXRecordDecl>( pDecl );
}

const clang::TemplateArgument* MatchHandler::PyTag::GetTemplateArg( const std::string& type ) const
{
	auto it = m_TemplateMap.find( type );
	if ( it != m_TemplateMap.end() )
	{
		return it->second;  // argument
	}
	assert( false && "Template argument not found" );
	return nullptr;
}

MatchHandler::PyClass::PyClass( const clang::ASTContext* pCtx, const clang::CXXRecordDecl* const pClass )
    : PyTag{ pCtx, pClass }
    , bHasDefaultConstructor{ pClass->hasDefaultConstructor() }
    , bHasCopyConstructor{ pClass->hasCopyConstructorWithConstParam() }
    , bHasMoveConstructor{ pClass->hasMoveConstructor() }
{
}

void MatchHandler::PyDecl::Flush( FrontendAction& action )
{
	action.AddClassDeclaration( declaration );
	action.AddClassDefinition( definition );
}

MatchHandler::PyDestructor::PyDestructor( const PyTag& pyTag )
{
	name        = pyTag.pyName + "_Dealloc";
	declaration = "void " + name + "( _PyspotWrapper* pSelf );\n\n";
	definition  = "void " + name + "( _PyspotWrapper* pSelf )\n{\n" + "\tif ( pSelf->bOwnData )\n\t{\n" +
	             "\t\tdelete reinterpret_cast<" + pyTag.qualifiedName + "*>( pSelf->pData );\n\t}\n" +
	             "\tPy_TYPE( pSelf )->tp_free( reinterpret_cast<PyObject*>( "
	             "pSelf ) );\n}\n\n";
}

MatchHandler::PyField::PyField( const PyTag& pyTag, const clang::FieldDecl* const pField )
    : pField{ pField }, owner{ pyTag }, name{ pField->getNameAsString() }, type{ pField->getType().getAsString() }
{
	decl.name        = "g_a" + pyTag.pyName + "_" + name;
	decl.declaration = "extern char " + decl.name + "[" + std::to_string( name.length() + 1 ) + "];\n\n";
	decl.definition  = "char " + decl.name + "[" + std::to_string( name.length() + 1 ) + "] { \"" + name + "\" };\n";

	if ( pField->isTemplated() )
	{
		type = owner.GetTemplateArg( pField->getType().getAsString() )->getAsType().getAsString();
	}
}

MatchHandler::PyGetter::PyGetter( PyField& field )
{
	decl.name        = field.owner.pyName + "_Get" + field.name;
	auto sign        = "PyObject* " + decl.name + "( _PyspotWrapper* pSelf, void* pClosure )";
	decl.declaration = sign + ";\n\n";
	decl.definition  = sign + "\n{\n\tauto pData = reinterpret_cast<" + field.owner.qualifiedName +
	                  "*>( pSelf->pData );\n\treturn " +
	                  pywrap::to_python( field.pField->getType(), "pData->" + field.name, field.owner.GetTemplateMap(),
	                                     *field.owner.pContext ) +
	                  ";\n}\n\n";
}

MatchHandler::PySetter::PySetter( PyField& field )
{
	decl.name        = field.owner.pyName + "_Set" + field.name;
	auto sign        = "int " + decl.name + "( _PyspotWrapper* pSelf, PyObject* pValue, void* pClosure )";
	decl.declaration = sign + ";\n\n";
	decl.definition  = sign +
	                  "\n{\n\tif ( pValue == nullptr )\n\t{\n\t\tPyErr_SetString( "
	                  "PyExc_TypeError, \"Cannot delete " +
	                  field.name + " pField\" );\n\t\treturn -1;\n\t}\n";

	if ( field.type == "std::string" )
	{
		decl.definition +=
		    "\n\tif( !PyUnicode_Check( pValue ) )\n\t{\n\t\tPyErr_SetString( "
		    "PyExc_TypeError, \"Field " +
		    field.name + " expects a string\" );\n\t\treturn -1;\n\t}\n";
	}

	decl.definition += "\n\tauto pData = reinterpret_cast<" + field.owner.qualifiedName + "*>( pSelf->pData );\n\tpData->" +
	                   field.name + " = " + pywrap::to_c( field.type, "pValue" ) + ";\n\treturn 0;\n}\n\n";
}

MatchHandler::PyAccessors::PyAccessors( const PyTag& pyTag )
{
	name        = "g_" + pyTag.pyName + "_accessors";
	declaration = "extern PyGetSetDef " + name + "[";
	definition  = "PyGetSetDef " + name + "[]\n{\n";
}

void MatchHandler::PyAccessors::Add( const PyField& field, const PyGetter& getter, const PySetter& setter )
{
	assert( !closed && "Cannot add field to an accessor which has been closed" );
	++count;
	definition += "\t{ " + field.decl.name + ", reinterpret_cast<getter>( " + getter.decl.name +
	              " ), reinterpret_cast<setter>( " + setter.decl.name + " ), " + field.decl.name + ", nullptr },\n";
}

void MatchHandler::PyAccessors::Flush( FrontendAction& action )
{
	if ( !closed )
	{
		declaration += std::to_string( count + 1 ) + "];\n\n";
		// Add sentinel
		definition += "\t{ nullptr } // sentinel\n};\n\n";
		// Close it
		closed = true;
		PyDecl::Flush( action );
	}
}

MatchHandler::PyInit::PyInit( const PyTag& pyTag ) : owner{ pyTag }
{
	name        = pyTag.pyName + "_Init";
	auto sign   = "int " + name + "( _PyspotWrapper* pSelf, PyObject* pArgs, PyObject* pKwds )";
	declaration = sign + ";\n\n";
	definition  = sign + "\n{\n\t" + pyTag.qualifiedName +
	             "* pData { nullptr };\n\n\tif ( pSelf->pData )\n\t{\n\t\tpData "
	             "= reinterpret_cast<" +
	             pyTag.qualifiedName + "*>( pSelf->pData );\n\t\treturn 0;\n\t}\n\n";

	definition +=
	    "\tauto argsSize = PyTuple_Size( pArgs );\n"
	    "\tauto kwdsSize = pKwds ? PyDict_Size( pKwds ) : 0;\n\n";

	auto pClass = pyTag.AsClass();
	if ( pClass && pClass->hasDefaultConstructor() )
	{
		definition +=
		    "\tif ( argsSize == 0 && kwdsSize == 0 )\n"
		    "\t{\n\t\tpData = new " +
		    pyTag.qualifiedName +
		    "{};\n"
		    "\t\tpSelf->pData = pData;\n"
		    "\t\tpSelf->bOwnData = true;\n"
		    "\t\treturn 0;\n\t}\n\n";
	}
}

void MatchHandler::PyInit::Add( const clang::CXXConstructorDecl* pConstructor )
{
	size_t positionalCount{ 0 };
	size_t keywordCount{ 0 };
	// Get positional and keyword argument size
	for ( auto pParam : pConstructor->parameters() )
	{
		pParam->hasDefaultArg() ? ++keywordCount : ++positionalCount;
	}

	definition += "\tif ( ( argsSize + kwdsSize ) == " + std::to_string( pConstructor->parameters().size() ) + " )\n\t{\n";

	auto        kwlistName = "a" + name + "_kvlist";
	auto        kwlist     = "static char* " + kwlistName + "[] { ";
	auto        fmtName    = "a" + name + "_fmt";
	auto        fmt        = "static const char* " + fmtName + " { \"";
	std::string arglist;
	std::string callArglist;

	for ( auto pParam : pConstructor->parameters() )
	{
		auto paramName       = "pw" + pParam->getNameAsString();
		auto dataName        = "p" + pParam->getNameAsString();
		auto staticParamName = "a" + paramName;

		kwlist += staticParamName + ", ";

		// Get param type
		auto qualType  = pParam->getType();
		auto paramType = qualType.getAsString();
		if ( pParam->isTemplated() )
		{
			if ( qualType->isTemplateTypeParmType() )
			{
				qualType  = owner.GetTemplateArg( qualType.getAsString() )->getAsType();
				paramType = qualType.getAsString();
			}
			else
			{
				paramType = pywrap::to_string( qualType, owner.GetTemplateMap(), *owner.pContext );
			}
		}

		if ( positionalCount == 0 )
		{
			fmt += "|";
		}

		if ( positionalCount >= 0 )
		{
			--positionalCount;
		}

		fmt += pywrap::to_py_parser( qualType );

		definition += "\t\t";
		if ( qualType->isBuiltinType() )
		{
			definition += paramType;
			definition += " " + paramName + " {};\n";
			callArglist += paramName + ", ";
		}
		else
		{
			// Wrapper
			definition += "PyObject* " + paramName + " {};\n";

			callArglist += "*" + dataName + ", ";
		}
		arglist += ", &" + paramName;

		definition += "\t\tstatic char " + staticParamName + "[" + std::to_string( paramName.length() + 1 ) + "] { \"" +
		              pParam->getNameAsString() + "\" };\n\n";
	}

	if ( positionalCount == 0 )
	{
		fmt += "|";
	}

	if ( callArglist.size() > 2 )
	{
		callArglist = callArglist.substr( 0, callArglist.size() - 2 );
	}

	definition += "\t\t" + kwlist + "nullptr };\n";
	definition += "\t\t" + fmt + "\" };\n\n";

	definition +=
	    "\t\tif ( PyArg_ParseTupleAndKeywords( pArgs, pKwds, " + fmtName + ", " + kwlistName + arglist + " ) )\n\t\t{\n";

	std::string constructorArgList{};

	for ( size_t i{ 0 }; i < pConstructor->parameters().size(); ++i )
	{
		auto pParam    = pConstructor->parameters()[i];
		auto paramName = "pw" + pParam->getNameAsString();
		auto dataName  = "p" + pParam->getNameAsString();

		// Get param type
		auto qualType  = pParam->getType();
		auto paramType = qualType.getAsString();
		if ( pParam->isTemplated() )
		{
			if ( qualType->isTemplateTypeParmType() )
			{
				qualType  = owner.GetTemplateArg( qualType.getAsString() )->getAsType();
				paramType = qualType.getAsString();
			}
			else
			{
				paramType = pywrap::to_string( qualType, owner.GetTemplateMap(), *owner.pContext );
			}
		}

		if ( qualType->isBuiltinType() )
		{
			constructorArgList += paramName;
		}
		else
		{
			// Get type as pointer
			if ( paramType.back() == '&' )
			{
				paramType.back() = ' ';
			}

			// Pointer to data
			definition += "\t\t\tauto " + dataName + " = " + pywrap::to_c( paramType, paramName ) + ";\n";
			constructorArgList += dataName;
		}

		if ( i < pConstructor->parameters().size() - 1 )
		{
			constructorArgList += ", ";
		}
	}

	definition += "\t\t\tpData = new " + owner.qualifiedName + "{ " + constructorArgList +
	              " };\n\t\t\tpSelf->pData = pData;\n\t\t\tpSelf->bOwnData = "
	              "true;\n\t\t\treturn 0;\n\t\t}\n\t}\n";
}

void MatchHandler::PyInit::Flush( FrontendAction& action )
{
	definition += "\n\treturn -1;\n}\n\n";

	PyDecl::Flush( action );
}

MatchHandler::PyCompare::PyCompare( const PyTag& pyTag ) : owner{ pyTag }
{
	name        = pyTag.pyName + "_Cmp";
	auto sign   = "PyObject* " + name + "( _PyspotWrapper* pLhs, _PyspotWrapper* pRhs, int op )";
	declaration = sign + ";\n\n";
	definition  = sign + "\n{\n";

	if ( pyTag.AsEnum() )
	{
		Add( clang::OO_EqualEqual );
	}
}

void MatchHandler::PyCompare::Add( const clang::OverloadedOperatorKind op )
{
	if ( op == clang::OO_EqualEqual )
	{
		definition +=
		    "\tif ( op == Py_EQ )\n\t{\n"
		    "\t\tauto& lhs = *reinterpret_cast<" +
		    owner.qualifiedName +
		    "*>( pLhs->pData );\n"
		    "\t\tauto& rhs = *reinterpret_cast<" +
		    owner.qualifiedName +
		    "*>( pRhs->pData );\n\n"
		    "\t\tif ( lhs == rhs )\n\t\t{\n"
		    "\t\t\tPy_INCREF( Py_True );\n\t\t\treturn Py_True;\n\t\t}\n"
		    "\t\telse\n\t\t{\n"
		    "\t\t\tPy_INCREF( Py_False );\n\t\t\treturn Py_False;\n\t\t}\n\t}\n\n";
	}
}

void MatchHandler::PyCompare::Flush( FrontendAction& action )
{
	// Compare-not-implemented tail
	definition += "\tPy_INCREF( Py_NotImplemented );\n\treturn Py_NotImplemented;\n}\n\n";
	PyDecl::Flush( action );
}

MatchHandler::PyMethods::PyMethods( const PyTag& pyTag )
{
	name        = "g_" + pyTag.pyName + "_methods";
	auto sign   = "PyMethodDef " + name;
	declaration = "extern " + sign + "[";
	definition  = sign + "[] {\n";
}

bool MatchHandler::PyMethods::Add( const PyMethod& method )
{
	// Check whether we already have a method with this name
	auto it = std::find_if( std::begin( methods ), std::end( methods ),
	                        [&method]( const PyMethod& m ) -> bool { return m.qualifiedName == method.qualifiedName; } );
	if ( it != std::end( methods ) )
	{
		return false;
	}

	// Add it to the methods def
	definition += "\t{\n\t\t" + method.nameDecl.name + ",\n" + "\t\treinterpret_cast<PyCFunction>( " +
	              method.methodDecl.name + " ),\n" + "\t\tMETH_VARARGS | METH_KEYWORDS,\n" + "\t\t\"" +
	              method.qualifiedName + "\"\n\t},\n";

	methods.emplace_back( method );
	return true;
}

void MatchHandler::PyMethods::Flush( FrontendAction& action )
{
	declaration += std::to_string( methods.size() + 1 ) + "];\n\n";
	definition += "\t{ nullptr } // sentinel\n};\n\n";
	PyDecl::Flush( action );
}

MatchHandler::PyMethod::PyMethod( const PyTag& pyTag, const clang::CXXMethodDecl* const pMethod )
    : owner{ pyTag }, name{ pMethod->getNameAsString() }, qualifiedName{ pMethod->getQualifiedNameAsString() }
{
	methodDecl.name = pyTag.pyName + "_Method_" + name;

	nameDecl.name        = "g_a" + methodDecl.name;
	auto nameSign        = "char " + nameDecl.name + "[" + std::to_string( name.length() + 1 ) + "]";
	nameDecl.declaration = "extern " + nameSign + ";\n\n";
	nameDecl.definition  = nameSign + " { \"" + name + "\" };\n\n";

	// Method wrapper
	auto sign              = "PyObject* " + methodDecl.name + "( _PyspotWrapper* pSelf, PyObject* pArgs, PyObject* pKwds )";
	methodDecl.declaration = sign + ";\n\n";
	methodDecl.definition  = sign + "\n{\n";

	auto        kwlistName = "a" + methodDecl.name + "_kvlist";
	auto        kwlist     = "static char* " + kwlistName + "[] { ";
	auto        fmtName    = "a" + methodDecl.name + "_fmt";
	auto        fmt        = "static const char* " + fmtName + " { \"";
	std::string arglist;
	std::string callArglist;

	for ( auto pParam : pMethod->parameters() )
	{
		auto paramName       = "pw" + pParam->getNameAsString();
		auto dataName        = "p" + pParam->getNameAsString();
		auto staticParamName = "a" + paramName;

		kwlist += staticParamName + ", ";

		auto qualType  = pParam->getType();
		auto paramType = qualType.getAsString();
		if ( pParam->isTemplated() )
		{
			if ( qualType->isTemplateTypeParmType() )
			{
				qualType  = pyTag.GetTemplateArg( qualType.getAsString() )->getAsType();
				paramType = qualType.getAsString();
			}
			else
			{
				paramType = pywrap::to_string( qualType, pyTag.GetTemplateMap(), *pyTag.pContext );
			}
		}

		fmt += pywrap::to_py_parser( qualType );

		methodDecl.definition += "\t";
		if ( qualType->isBuiltinType() )
		{
			methodDecl.definition += paramType;
			methodDecl.definition += " " + paramName + " {};\n";
			callArglist += paramName + ", ";
		}
		else if ( qualType->isPointerType() )
		{
			auto pPointeeType = qualType->getPointeeOrArrayElementType();
			if ( pPointeeType->isAnyCharacterType() )
			{
				methodDecl.definition += "PyObject* " + paramName + ";\n";
				callArglist += pywrap::to_c( paramType, paramName ) + ", ";
			}
		}
		else
		{
			// Wrapper
			methodDecl.definition += "_PyspotWrapper* " + paramName + " {};\n";
			// Get type as pointer
			if ( paramType.back() == '&' )
			{
				paramType.back() = ' ';
			}
			// Pointer to data
			methodDecl.definition +=
			    "\tauto " + dataName + " = reinterpret_cast<" + paramType + "*>( " + paramName + "->pData );\n";
			callArglist += "*" + dataName + ", ";
		}
		arglist += ", &" + paramName;

		methodDecl.definition += "\tstatic char " + staticParamName + "[" + std::to_string( paramName.length() + 1 ) +
		                         "] { \"" + paramName + "\" };\n\n";
	}

	if ( callArglist.size() > 2 )
	{
		callArglist = callArglist.substr( 0, callArglist.size() - 2 );
	}

	methodDecl.definition += "\t" + kwlist + "nullptr };\n";
	methodDecl.definition += "\t" + fmt + "|\" };\n\n";

	methodDecl.definition += "\tif ( !PyArg_ParseTupleAndKeywords( pArgs, pKwds, " + fmtName + ", " + kwlistName + arglist +
	                         " ) )\n\t{\n\t\tPy_INCREF( Py_None );\n\t\treturn Py_None;\n\t}\n\n";

	// Get pData
	methodDecl.definition += "\tauto pData = reinterpret_cast<" + pyTag.qualifiedName + "*>( pSelf->pData );\n";

	auto call = "pData->" + name + "( " + callArglist + " )";
	// Check return type
	if ( pMethod->getReturnType()->isVoidType() )
	{
		methodDecl.definition += "\t" + call +
		                         ";\n"
		                         "\tPy_INCREF( Py_None );\n\treturn Py_None;\n}\n\n";
	}
	else
	{
		auto trueType   = pywrap::to_type( pMethod->getReturnType(), pyTag.GetTemplateMap() );
		auto returnType = pywrap::to_string( trueType, pyTag.GetTemplateMap(), *pyTag.pContext );
		auto anglePos   = returnType.find( '<' );
		if ( anglePos != std::string::npos )
		{
			if ( owner.qualifiedName.find( returnType.substr( 0, anglePos ) ) != std::string::npos )
			{
				returnType = owner.qualifiedName;
				methodDecl.definition += "\treturn pywrap::Wrapper<" + returnType + ">{ ";
				if ( trueType->isReferenceType() || trueType->isPointerType() )
				{
					methodDecl.definition += "&";
				}
				methodDecl.definition += call + " }.GetIncref();\n}\n\n";
				return;
			}
		}
		methodDecl.definition +=
		    "\treturn " + pywrap::to_python( pMethod->getReturnType(), call, owner.GetTemplateMap(), *owner.pContext ) +
		    ";\n}\n\n";
	}
}

void MatchHandler::PyMethod::Flush( FrontendAction& action )
{
	nameDecl.Flush( action );
	methodDecl.Flush( action );
}

MatchHandler::PyTypeObj::PyTypeObj( const PyTag& pyTag )
{
	name        = "g_" + pyTag.pyName + "TypeObject";
	declaration = "extern PyTypeObject " + name + ";\n\n";
	definition  = "PyTypeObject " + name +
	             " = {\n"
	             "\tPyVarObject_HEAD_INIT( nullptr, 0 )\n\n"
	             "\t\"" +
	             pyTag.qualifiedName +
	             "\",\n"
	             "\tsizeof( _PyspotWrapper ),\n"
	             "\t0,\n\n"
	             "\treinterpret_cast<destructor>( " +
	             pyTag.destructor.name +
	             " ),\n"
	             "\t0,\n"
	             "\t0,\n"
	             "\t0,\n"
	             "\t0,\n"
	             "\t0,\n\n"
	             "\t0,\n"
	             "\t0,\n"
	             "\t0,\n\n"
	             "\t0,\n"
	             "\t0,\n"
	             "\t0,\n"
	             "\t0,\n"
	             "\t0,\n\n"
	             "\t0,\n\n"
	             "\tPy_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,\n\n"
	             "\t\"" +
	             pyTag.pyName +
	             "\",\n\n"
	             "\t0,\n\n"
	             "\t0,\n\n"
	             "\treinterpret_cast<richcmpfunc>( " +
	             pyTag.compare.name +
	             " ),\n\n"
	             "\t0,\n\n"
	             "\t0,\n"
	             "\t0,\n\n"
	             "\t" +
	             pyTag.methods.name +
	             ",\n"
	             "\t" +
	             pyTag.members.name +
	             ",\n"
	             "\t" +
	             pyTag.accessors.name +
	             ",\n"
	             "\t0,\n"
	             "\t0,\n"
	             "\t0,\n"
	             "\t0,\n"
	             "\t0,\n"
	             "\treinterpret_cast<initproc>( " +
	             pyTag.init.name +
	             " ),\n"
	             "\t0,\n"
	             "\tPyspotWrapper_New,\n};\n\n";
}

MatchHandler::PyMembers::PyMembers( const PyTag& pyTag )
{
	name        = "g_" + pyTag.pyName + "_members";
	declaration = "extern PyMemberDef " + name + "[1];\n\n";
	definition  = "PyMemberDef " + name + "[]\n{\n\t{ nullptr } // sentinel\n};\n\n";
}

MatchHandler::PyEnums::PyEnums( const PyTag& pyTag )
{
	if ( auto pEnum = pyTag.AsEnum() )
	{
		for ( auto value : pEnum->enumerators() )
		{
			auto name = value->getNameAsString();

			definition += "\tPyDict_SetItemString( " + pyTag.typeObject.name + ".tp_dict, \"" + name +
			              "\", pywrap::Wrapper<" + pyTag.qualifiedName + ">{ " + pyTag.qualifiedName + "::" + name +
			              " }.GetIncref() );\n";
		}
	}
}

void MatchHandler::PyEnums::Flush( FrontendAction& action )
{
	action.AddEnumerator( definition );
}

MatchHandler::WrapperConstructors::WrapperConstructors( const PyTag& pyTag, const std::string& objectName )
{
	// Generate Wrapper constructors template specializations
	auto constructorName = "template<>\npywrap::Wrapper<" + pyTag.qualifiedName + ">::Wrapper( " + pyTag.qualifiedName;
	auto pyObjectReady   = ":\tpywrap::Object { ( PyType_Ready( &" + objectName +
	                     " ),\n"
	                     " \t                   PyspotWrapper_New( &" +
	                     objectName + ", nullptr, nullptr ) ) }\n";
	std::string getWrapper{ "\tauto pWrapper = reinterpret_cast<_PyspotWrapper*>( mObject );\n" };

	// Pointer constructor
	pointer.name        = constructorName + "* pV )";
	pointer.declaration = pointer.name + ";\n\n";
	pointer.definition  = pointer.name + "\n" + pyObjectReady + ",\tpPayload { pV }\n{\n" + getWrapper +
	                     "\tpWrapper->pData = pPayload;\n}\n\n";

	auto pEnum  = pyTag.AsEnum();
	auto pClass = pyTag.AsClass();

	// Copy constructor
	if ( pEnum || ( pClass && pClass->hasCopyConstructorWithConstParam() ) )
	{
		copy.name        = constructorName + "& v )";
		copy.declaration = copy.name + ";\n\n";
		copy.definition  = copy.name + "\n" + pyObjectReady + ",\tpPayload { new " + pyTag.qualifiedName + "{ v } }\n{\n" +
		                  getWrapper + "\tpWrapper->pData = pPayload;\n\tpWrapper->bOwnData = true;\n}\n\n";
	}

	// Move constructor
	if ( pEnum || ( pClass && pClass->hasMoveConstructor() ) )
	{
		move.name        = constructorName + "&& v )";
		move.declaration = move.name + ";\n\n";
		move.definition  = move.name + "\n" + pyObjectReady + ",\tpPayload { new " + pyTag.qualifiedName +
		                  "{ std::move( v ) } }\n{\n" + getWrapper +
		                  "\tpWrapper->pData = pPayload;\n\tpWrapper->bOwnData = true;\n}\n\n";
	}
}

void MatchHandler::WrapperConstructors::Flush( FrontendAction& action )
{
	action.AddWrapperDeclaration( pointer.declaration );
	action.AddWrapperDefinition( pointer.definition );
	action.AddWrapperDeclaration( copy.declaration );
	action.AddWrapperDefinition( copy.definition );
	action.AddWrapperDeclaration( move.declaration );
	action.AddWrapperDefinition( move.definition );
}

bool MatchHandler::checkHandled( const PyTag& pyTag )
{
	// Check if we have already processed this
	auto found  = pywrap::find( m_Frontend.GetHandled(), pyTag.qualifiedName );
	auto bFound = ( found != std::end( m_Frontend.GetHandled() ) );
	if ( !bFound )
	{
		// Add this to the handled decls
		m_Frontend.AddHandled( pyTag.qualifiedName );
	}
	return bFound;
}

void MatchHandler::PyTag::Flush( FrontendAction& action )
{
	destructor.Flush( action );

	// Get public members
	if ( auto pClass = AsClass() )
	{
		for ( auto pField : pClass->fields() )
		{
			auto bPublic = ( pField->getAccess() == clang::AS_public );
			if ( !bPublic )
			{
				continue;
			}

			// Field
			PyField field{ *this, pField };
			field.decl.Flush( action );

			// Getter
			PyGetter getter{ field };
			getter.decl.Flush( action );

			// Setter
			PySetter setter{ field };
			setter.decl.Flush( action );

			accessors.Add( field, getter, setter );
		}
	}

	accessors.Flush( action );

	if ( auto pClass = AsClass() )
	{
		for ( auto pMethod : pClass->methods() )
		{
			auto bPublic      = ( pMethod->getAccess() == clang::AS_public );
			auto bOverloaded  = pMethod->isOverloadedOperator();
			auto bConstructor = ( pMethod->getKind() == pMethod->CXXConstructor );
			auto bDestructor  = ( pMethod->getKind() == pMethod->CXXDestructor );
			if ( !bPublic || bDestructor )
			{
				continue;
			}
			else if ( bConstructor )
			{
				auto pConstr = clang::dyn_cast<clang::CXXConstructorDecl>( pMethod );
				if ( !pConstr->isCopyOrMoveConstructor() )
				{
					init.Add( pConstr );
				}
			}
			else if ( bOverloaded )
			{
				compare.Add( pMethod->getOverloadedOperator() );
			}
			else
			{
				PyMethod method{ *this, pMethod };
				if ( methods.Add( method ) )
				{
					method.Flush( action );
				}
			}
		}
	}

	init.Flush( action );
	compare.Flush( action );
	methods.Flush( action );
	members.Flush( action );
	typeObject.Flush( action );
	enums.Flush( action );

	// Class registration
	auto reg = "\t// Register " + name +
	           "\n"
	           "\tif ( PyType_Ready( &" +
	           typeObject.name +
	           " ) < 0 )\n\t{\n\t\t"
#if PY_MAJOR_VERSION > 3
	           "Py_INCREF( Py_None );\n\t\treturn Py_None;"
#else
	           "return;"
#endif
	           "\n\t}\n"
	           "\tPy_INCREF( &" +
	           typeObject.name +
	           " );\n"
	           "\tPyModule_AddObject( pModule, \"" +
	           name + "\" , reinterpret_cast<PyObject*>( &" + typeObject.name + " ) );\n\n";
	action.AddClassRegistration( reg );

	WrapperConstructors constructors{ *this, typeObject.name };
	constructors.Flush( action );
}

void MatchHandler::handleTag( const clang::TagDecl* pTag, TemplateMap&& tMap )
{
	PyTag pyTag{ m_pContext, pTag, std::move( tMap ) };

	// Check if we have already processed it
	if ( checkHandled( pyTag ) )
	{
		return;
	}

	// Get include directive
	auto fileName = get_include_path( pTag );
	m_Frontend.AddClassInclude( "#include \"" + fileName + "\"\n" );

	// Flush everything
	pyTag.Flush( m_Frontend );
}


binding::Module& MatchHandler::get_module( const clang::DeclContext* ctx )
{
	assert( ctx && ctx->isNamespace() && "Function should be in a namespace" );
	auto namespace_decl = clang::dyn_cast<clang::NamespaceDecl>( ctx );
	// Find the module
	auto id = namespace_decl->getGlobalID();
	auto it = modules.find( id );
	if ( it == modules.end() )
	{
		// Create it the first time
		auto pr = modules.emplace( id, binding::Module{ namespace_decl } );
		if ( pr.second )  // success
		{
			it = pr.first;
		}
	}
	return it->second;
}

template <typename B, typename D>
B MatchHandler::create_binding( const D* decl )
{
	B binding{ decl };
	binding.set_incl( get_include_path( decl ) );
	return binding;
}

void MatchHandler::generate_bindings( const clang::Decl* decl )
{
	// The function should be in a namespace
	// TODO But an enum can be in a class as well
	auto  ctx    = decl->getDeclContext();
	auto& module = get_module( ctx );

	// Switch according to the decl
	if ( auto func_decl = clang::dyn_cast<clang::FunctionDecl>( decl ) )
	{
		// Add the function to the module
		module.add( create_binding<binding::Function>( func_decl ) );
	}
	// Generate enum bindings
	else if ( auto enum_decl = clang::dyn_cast<clang::EnumDecl>( decl ) )
	{
		// Add the enum to the module
		module.add( create_binding<binding::Enum>( enum_decl ) );
	}
	// Generate struct/union/class bindings
	else if ( auto record_decl = clang::dyn_cast<clang::CXXRecordDecl>( decl ) )
	{
		// Add the record to the module
		module.add( create_binding<binding::CXXRecord>( record_decl ) );
	}
}

void MatchHandler::run( const clang::ast_matchers::MatchFinder::MatchResult& result )
{
	// Save current context
	m_pContext = result.Context;

	if ( auto decl = result.Nodes.getNodeAs<clang::Decl>( "PyspotTag" ) )
	{
		if ( auto annotate = decl->getAttr<clang::AnnotateAttr>() )
		{
			if ( annotate->getAnnotation() == "pyspot" )
			{
				// We have found a decl with our pyspot annotation
				generate_bindings( decl );
			}
		}
	}

	// Get Tag (super class)
	auto pTag = result.Nodes.getNodeAs<clang::TagDecl>( "PyspotTag" );
	if ( pTag == nullptr )
	{
		return;  // Not the right type
	}


	// If it is a class
	if ( auto pClass = clang::dyn_cast<clang::CXXRecordDecl>( pTag ) )
	{
		// It it is a template
		if ( auto pTemplate = pClass->getDescribedClassTemplate() )
		{
			// m_Templates.emplace_back( pTemplate );

			// Get its parameters
			auto params = pTemplate->getTemplateParameters()->asArray();

			// Get its specializations
			for ( auto pSpec : pTemplate->specializations() )
			{
				// if ( pSpec->getPointOfInstantiation().isValid() )
				//{
				//	handleTag( pSpec );
				//}
				// continue;
				pSpec->startDefinition();
				pSpec->completeDefinition();

				TemplateMap templateMap;

				auto args = pSpec->getTemplateArgs().asArray();
				for ( size_t i = 0; i < args.size(); ++i )
				{
					auto paramType = params[i]->getNameAsString();
					auto argType   = args[i];
					templateMap.emplace( paramType, &argType );
				}

				handleTag( pTag, std::move( templateMap ) );
				return;
			}
		}
	}

	handleTag( pTag );
}

}  // namespace pywrap