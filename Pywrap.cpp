#include "Pywrap.h"

#include "clang\Lex\HeaderSearch.h"


Printer g_Printer;


PyspotMatchHandler::PyspotMatchHandler( PyspotFrontendAction& frontend )
: m_Frontend { frontend }
, m_Cwd { getCwd() }
{}


std::string PyspotMatchHandler::getCwd()
{
	llvm::SmallString<64> aCwd;
	llvm::sys::fs::current_path( aCwd );
	return pyspot::replace_all( aCwd.str().str(), "\\", "/" );
}


std::string PyspotMatchHandler::toPyspotName( std::string name )
{
	while ( true )
	{
		// Search for "::"
		auto found = name.find( "::" );
		if ( found == std::string::npos )
		{
			break;
		}

		name = "Py" + name.substr( 0, found ) + name.substr( found + 2, name.length() - found - 2 );
	}

	return name;
}


std::string PyspotMatchHandler::getIncludePath( const Decl* const pDecl )
{
	auto location = pDecl->getLocation().printToString( m_pContext->getSourceManager() );
	pyspot::replace_all( location, "\\", "/" );

	// Remove include directories from path
	for ( auto& path : m_Frontend.GetGlobalIncludes() )
	{
		auto found = location.find( path );

		if ( found == 0 )
		{
			auto cwdLen = path.size() + 1; // remove '/' as well
			location = location.substr( cwdLen, location.length() - cwdLen );
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


PyspotMatchHandler::PyTag::PyTag( const TagDecl* const pTagDecl )
:	pDecl { pTagDecl }
,	name          { pDecl->getNameAsString() }
,	qualifiedName { pDecl->getQualifiedNameAsString() }
,	pyName        { toPyspotName( qualifiedName ) }
,	init       { *this }
,	destructor { *this }
,	compare    { *this }
,	methods    { *this }
,	accessors  { *this }
,	members    { *this }
,	typeObject { *this }
,	enums      { *this }
{}


const EnumDecl* PyspotMatchHandler::PyTag::AsEnum() const
{
	return dyn_cast<EnumDecl>( pDecl );
}


const CXXRecordDecl* PyspotMatchHandler::PyTag::AsClass() const
{
	return dyn_cast<CXXRecordDecl>( pDecl );
}


PyspotMatchHandler::PyClass::PyClass( const CXXRecordDecl* const pClass )
:	PyTag { pClass }
,	bHasDefaultConstructor { pClass->hasDefaultConstructor() }
,	bHasCopyConstructor    { pClass->hasCopyConstructorWithConstParam() }
,	bHasMoveConstructor    { pClass->hasMoveConstructor() }
{}


void PyspotMatchHandler::PyDecl::Flush( PyspotFrontendAction& action )
{
	action.AddClassDeclaration( declaration );
	action.AddClassDefinition( definition );
}


PyspotMatchHandler::PyDestructor::PyDestructor( const PyTag& pyTag )
{
	name = pyTag.pyName + "_Dealloc";
	declaration = "void " + name + "( _PyspotWrapper* pSelf );\n\n";
	definition  = "void " + name + "( _PyspotWrapper* pSelf )\n{\n" + "\tif ( pSelf->bOwnData )\n\t{\n"
		+ "\t\tdelete reinterpret_cast<" + pyTag.qualifiedName + "*>( pSelf->pData );\n\t}\n"
		+ "\tPy_TYPE( pSelf )->tp_free( reinterpret_cast<PyObject*>( pSelf ) );\n}\n\n";
}


PyspotMatchHandler::PyField::PyField( const PyTag& pyTag, const FieldDecl* const pField )
:	owner { pyTag }
,	name  { pField->getNameAsString() }
,	type  { pField->getType().getAsString() }
{
	decl.name = "g_a" + pyTag.pyName + "_" + name;
	decl.declaration = "extern char " + decl.name + "[" + std::to_string( name.length() + 1) + "];\n\n";
	decl.definition  = "char " + decl.name + "[" + std::to_string( name.length() + 1 ) + "] { \"" + name + "\" };\n";
}


PyspotMatchHandler::PyGetter::PyGetter( PyField& field )
{
	decl.name = field.owner.pyName + "_Get" + field.name;
	auto sign = "PyObject* " + decl.name + "( _PyspotWrapper* pSelf, void* pClosure )";
	decl.declaration = sign + ";\n\n";
	decl.definition  = sign + "\n{\n\tauto pData = reinterpret_cast<" + field.owner.qualifiedName
		+ "*>( pSelf->pData );\n\treturn "
		+ pyspot::to_python( field.type, "pData->" + field.name ) + ";\n}\n\n";
}


PyspotMatchHandler::PySetter::PySetter( PyField& field )
{
	decl.name = field.owner.pyName + "_Set" + field.name;
	auto sign = "int " + decl.name + "( _PyspotWrapper* pSelf, PyObject* pValue, void* pClosure )";
	decl.declaration = sign + ";\n\n";
	decl.definition  = sign
		+ "\n{\n\tif ( pValue == nullptr )\n\t{\n\t\tPyErr_SetString( PyExc_TypeError, \"Cannot delete "
		+ field.name + " pField\" );\n\t\treturn -1;\n\t}\n";

	if ( field.type == "std::string" )
	{
		decl.definition += "\n\tif( !PyUnicode_Check( pValue ) )\n\t{\n\t\tPyErr_SetString( PyExc_TypeError, \"Field "
			+ field.name + " expects a string\" );\n\t\treturn -1;\n\t}\n";
	}

	decl.definition += "\n\tauto pData = reinterpret_cast<" + field.owner.qualifiedName
		+ "*>( pSelf->pData );\n\tpData->" + field.name + " = " + pyspot::to_c( field.type, "pValue" )
		+ ";\n\treturn 0;\n}\n\n";
}


PyspotMatchHandler::PyAccessors::PyAccessors( const PyTag& pyTag )
{
	name = "g_" + pyTag.pyName + "_accessors";
	declaration = "extern PyGetSetDef " + name + "[";
	definition  = "PyGetSetDef " + name + "[]\n{\n";
}



void PyspotMatchHandler::PyAccessors::Add( const PyField& field, const PyGetter& getter, const PySetter& setter )
{
	assert( !closed && "Cannot add field to an accessor which has been closed" );
	++count;
	definition += "\t{ " + field.decl.name + ", reinterpret_cast<getter>( " + getter.decl.name
		+ " ), reinterpret_cast<setter>( " + setter.decl.name + " ), " + field.decl.name + ", nullptr },\n";
}


void PyspotMatchHandler::PyAccessors::Flush( PyspotFrontendAction& action )
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


PyspotMatchHandler::PyInit::PyInit( const PyTag& pyTag )
:	owner { pyTag }
{
	name = pyTag.pyName + "_Init";
	auto sign = "int " + name + "( _PyspotWrapper* pSelf, PyObject* pArgs, PyObject* pKwds )";
	declaration = sign + ";\n\n";
	definition  = sign + "\n{\n\t" + pyTag.qualifiedName
		+ "* pData { nullptr };\n\n\tif ( pSelf->pData )\n\t{\n\t\tpData = reinterpret_cast<"
		+ pyTag.qualifiedName + "*>( pSelf->pData );\n\t\treturn 0;\n\t}\n\n";

	definition += "\tauto argsSize = PyTuple_Size( pArgs );\n"
		"\tauto kwdsSize = pKwds ? PyDict_Size( pKwds ) : 0;\n\n";

	auto pClass = pyTag.AsClass();
	if ( pClass && pClass->hasDefaultConstructor() )
	{
		definition += "\tif ( argsSize == 0 && kwdsSize == 0 )\n"
			"\t{\n\t\tpData = new " + pyTag.qualifiedName + "{};\n"
			"\t\tpSelf->pData = pData;\n"
			"\t\tpSelf->bOwnData = true;\n"
			"\t\treturn 0;\n\t}\n\n";
	}
}

void PyspotMatchHandler::PyInit::Add( const CXXConstructorDecl* pConstructor )
{
	size_t positionalCount { 0 };
	size_t keywordCount    { 0 };
	// Get positional and keyword argument size
	for ( auto pParam : pConstructor->parameters() )
	{
		pParam->hasDefaultArg() ? ++keywordCount : ++positionalCount;
	}

	definition += "\tif ( ( argsSize + kwdsSize ) == "
		+ std::to_string( pConstructor->parameters().size() ) + " )\n\t{\n";

	auto kwlistName = "a" + name + "_kvlist";
	auto kwlist = "static char* " + kwlistName + "[] { ";
	auto fmtName = "a" + name + "_fmt";
	auto fmt = "static const char* " + fmtName + " { \"";
	std::string arglist;
	std::string callArglist;

	for ( auto pParam : pConstructor->parameters() )
	{
		auto paramName = "pw" + pParam->getNameAsString();
		auto dataName = "p" + pParam->getNameAsString();
		auto staticParamName = "a" + paramName;

		kwlist += staticParamName + ", ";

		auto pType = pParam->getType().getTypePtr();

		if ( positionalCount == 0 )
		{
			fmt += "|";
		}

		if ( positionalCount >= 0 )
		{
			--positionalCount;
		}

		fmt += pyspot::to_parser( pType );

		definition += "\t\t";
		if ( pType->isBuiltinType() )
		{
			definition += pParam->getType().withoutLocalFastQualifiers().getAsString();
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

		definition += "\t\tstatic char " + staticParamName + "["
			+ std::to_string( paramName.length() + 1 ) + "] { \""
			+ pParam->getNameAsString() + "\" };\n\n";
	}

	if ( positionalCount == 0 )
	{
		fmt += "|";
	}

	if ( callArglist.size() > 2 )
	{
		callArglist = callArglist.substr(0, callArglist.size() - 2);
	}

	definition += "\t\t" + kwlist + "nullptr };\n";
	definition += "\t\t" + fmt + "\" };\n\n";

	definition += "\t\tif ( PyArg_ParseTupleAndKeywords( pArgs, pKwds, "
		+ fmtName + ", " + kwlistName + arglist + " ) )\n\t\t{\n";

	std::string constructorArgList {};

	for ( size_t i { 0 }; i < pConstructor->parameters().size(); ++i )
	{
		auto pParam = pConstructor->parameters()[i];
		auto paramName = "pw" + pParam->getNameAsString();
		auto dataName = "p" + pParam->getNameAsString();

		// Get param type
		auto pType = pParam->getType().getTypePtr();

		if ( pType->isBuiltinType() )
		{
			constructorArgList += paramName;
		}
		else
		{
			// Get type as pointer
			auto paramType = pParam->getType().withoutLocalFastQualifiers().getAsString();
			if (paramType.back() == '&')
			{
				paramType.back() = ' ';
			}

			// Pointer to data
			definition += "\t\t\tauto " + dataName + " = " + pyspot::to_c( paramType, paramName ) + ";\n";
			constructorArgList += dataName;
		}

		if ( i < pConstructor->parameters().size() - 1 )
		{
			constructorArgList += ", ";
		}
	}

	definition += "\t\t\tpData = new " + owner.qualifiedName + "{ " + constructorArgList
		+ " };\n\t\t\tpSelf->pData = pData;\n\t\t\tpSelf->bOwnData = true;\n\t\t\treturn 0;\n\t\t}\n\t}\n";
}

void PyspotMatchHandler::PyInit::Flush( PyspotFrontendAction& action )
{
	definition += "\n\treturn -1;\n}\n\n";

	PyDecl::Flush( action );
}

PyspotMatchHandler::PyCompare::PyCompare( const PyTag& pyTag )
:	owner { pyTag }
{
	name = pyTag.pyName + "_Cmp";
	auto sign = "PyObject* " + name + "( _PyspotWrapper* pLhs, _PyspotWrapper* pRhs, int op )";
	declaration = sign + ";\n\n";
	definition  = sign + "\n{\n";

	if ( pyTag.AsEnum() )
	{
		Add( OO_EqualEqual );
	}
}


void PyspotMatchHandler::PyCompare::Add( const OverloadedOperatorKind op )
{
	if ( op == OO_EqualEqual )
	{
		definition += "\tif ( op == Py_EQ )\n\t{\n"
			"\t\tauto& lhs = *reinterpret_cast<" + owner.qualifiedName + "*>( pLhs->pData );\n"
			"\t\tauto& rhs = *reinterpret_cast<" + owner.qualifiedName + "*>( pRhs->pData );\n\n"
			"\t\tif ( lhs == rhs )\n\t\t{\n"
			"\t\t\tPy_INCREF( Py_True );\n\t\t\treturn Py_True;\n\t\t}\n"
			"\t\telse\n\t\t{\n"
			"\t\t\tPy_INCREF( Py_False );\n\t\t\treturn Py_False;\n\t\t}\n\t}\n\n";
	}
}


void PyspotMatchHandler::PyCompare::Flush( PyspotFrontendAction& action )
{
	// Compare-not-implemented tail
	definition += "\tPy_INCREF( Py_NotImplemented );\n\treturn Py_NotImplemented;\n}\n\n";
	PyDecl::Flush( action );
}


PyspotMatchHandler::PyMethods::PyMethods( const PyTag& pyTag )
{
	name = "g_" + pyTag.pyName + "_methods";
	auto sign = "PyMethodDef " + name;
	declaration = "extern " + sign + "[";
	definition  = sign + "[] {\n";
}


void PyspotMatchHandler::PyMethods::Add( const PyMethod& method )
{
	// Add it to the methods def
	definition += "\t{\n\t\t" + method.nameDecl.name + ",\n"
		+ "\t\treinterpret_cast<PyCFunction>( " + method.methodDecl.name + " ),\n"
		+ "\t\tMETH_VARARGS | METH_KEYWORDS,\n"
		+ "\t\t\"" + method.qualifiedName + "\"\n\t},\n";
	++count;
}


void PyspotMatchHandler::PyMethods::Flush( PyspotFrontendAction& action )
{
	declaration += std::to_string( count + 1 ) + "];\n\n";
	definition += "\t{ nullptr } // sentinel\n};\n\n";
	PyDecl::Flush( action );
}


PyspotMatchHandler::PyMethod::PyMethod( const PyTag& pyTag, const CXXMethodDecl* const pMethod )
:	owner         { pyTag }
,	name          { pMethod->getNameAsString() }
,	qualifiedName { pMethod->getQualifiedNameAsString() }
{
	methodDecl.name = pyTag.pyName + "_Method_" + name;

	nameDecl.name = "g_a" + methodDecl.name;
	auto nameSign = "char " + nameDecl.name + "[" + std::to_string( name.length() + 1 ) + "]";
	nameDecl.declaration = "extern " + nameSign + ";\n\n";
	nameDecl.definition  = nameSign + " { \"" + name + "\" };\n\n";

	// Method wrapper
	auto sign = "PyObject* " + methodDecl.name + "( _PyspotWrapper* pSelf, PyObject* pArgs, PyObject* pKwds )";
	methodDecl.declaration = sign + ";\n\n";
	methodDecl.definition  = sign + "\n{\n";

	auto kwlistName = "a" + methodDecl.name + "_kvlist";
	auto kwlist = "static char* " + kwlistName + "[] { ";
	auto fmtName = "a" + methodDecl.name + "_fmt";
	auto fmt = "static const char* " + fmtName + " { \"";
	std::string arglist;
	std::string callArglist;

	for ( auto pParam : pMethod->parameters() )
	{
		auto paramName = "pw" + pParam->getNameAsString();
		auto dataName = "p" + pParam->getNameAsString();
		auto staticParamName = "a" + paramName;

		kwlist += staticParamName + ", ";

		auto pType = pParam->getType().getTypePtr();
		fmt += pyspot::to_parser( pType );

		methodDecl.definition += "\t";
		if ( pType->isBuiltinType() )
		{
			methodDecl.definition += pParam->getType().withoutLocalFastQualifiers().getAsString();
			methodDecl.definition += " " + paramName + " {};\n";
			callArglist += paramName + ", ";
		}
		else
		{
			// Wrapper
			methodDecl.definition += "_PyspotWrapper*";
			methodDecl.definition += " " + paramName + " {};\n";
			// Get type as pointer
			auto paramType = pParam->getType().withoutLocalFastQualifiers().getAsString();
			if (paramType.back() == '&')
			{
				paramType.back() = ' ';
			}
			// Pointer to data
			methodDecl.definition += "\tauto " + dataName + " = reinterpret_cast<"
				+ paramType + "*>( " + paramName + "->pData );\n";
			callArglist += "*" + dataName + ", ";
		}
		arglist += ", &" + paramName;

		methodDecl.definition += "\tstatic char " + staticParamName + "["
			+ std::to_string( paramName.length() + 1 ) + "] { \"" + paramName + "\" };\n\n";
	}

	if ( callArglist.size() > 2 )
	{
		callArglist = callArglist.substr(0, callArglist.size() - 2);
	}

	methodDecl.definition += "\t" + kwlist + "nullptr };\n";
	methodDecl.definition += "\t" + fmt + "|\" };\n\n";

	methodDecl.definition += "\tif ( !PyArg_ParseTupleAndKeywords( pArgs, pKwds, "
		+ fmtName + ", " + kwlistName + arglist + " ) )\n\t{\n\t\treturn Py_None;\n\t}\n\n";

	// Get pData
	methodDecl.definition += "\tauto pData = reinterpret_cast<" + pyTag.qualifiedName + "*>( pSelf->pData );\n";

	auto call = "pData->" + name + "( " + callArglist + " )";
	// Check return type
	if ( pMethod->getReturnType().getTypePtr()->isVoidType() )
	{
		methodDecl.definition += "\t" + call + ";\n"
			"\treturn Py_None;\n}\n\n";
	}
	else
	{
		methodDecl.definition += "\treturn "
			+ pyspot::to_python( pMethod->getReturnType().getAsString(), call ) + ";\n}\n\n";
	}
}


void PyspotMatchHandler::PyMethod::Flush( PyspotFrontendAction& action )
{
	nameDecl.Flush( action );
	methodDecl.Flush( action );
}


PyspotMatchHandler::PyTypeObj::PyTypeObj( const PyTag& pyTag )
{
	name = "g_" + pyTag.pyName + "TypeObject";
	declaration = "extern PyTypeObject " + name + ";\n\n";
	definition  = "PyTypeObject " + name + " = {\n"
		"\tPyVarObject_HEAD_INIT( nullptr, 0 )\n\n"
		"\t\"" + pyTag.qualifiedName + "\",\n"
		"\tsizeof( _PyspotWrapper ),\n"
		"\t0,\n\n"
		"\treinterpret_cast<destructor>( " + pyTag.destructor.name + " ),\n"
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
		"\t\"" + pyTag.pyName + "\",\n\n"
		"\t0,\n\n"
		"\t0,\n\n"
		"\treinterpret_cast<richcmpfunc>( " + pyTag.compare.name + " ),\n\n"
		"\t0,\n\n"
		"\t0,\n"
		"\t0,\n\n"
		"\t" + pyTag.methods.name + ",\n"
		"\t" + pyTag.members.name + ",\n"
		"\t" + pyTag.accessors.name + ",\n"
		"\t0,\n"
		"\t0,\n"
		"\t0,\n"
		"\t0,\n"
		"\t0,\n"
		"\treinterpret_cast<initproc>( " + pyTag.init.name + " ),\n"
		"\t0,\n"
		"\tPyspotWrapper_New,\n};\n\n";
}

PyspotMatchHandler::PyMembers::PyMembers( const PyTag& pyTag )
{
	name = "g_" + pyTag.pyName + "_members";
	declaration = "extern PyMemberDef " + name + "[1];\n\n";
	definition  = "PyMemberDef " + name + "[]\n{\n\t{ nullptr } // sentinel\n};\n\n";
}


PyspotMatchHandler::PyEnums::PyEnums( const PyTag& pyTag )
{
	if ( auto pEnum = pyTag.AsEnum() )
	{
		for ( auto value : pEnum->enumerators() )
		{
			auto name = value->getNameAsString();

			definition += "\tPyDict_SetItemString( " + pyTag.typeObject.name
				+ ".tp_dict, \"" + name + "\", pyspot::Wrapper<" + pyTag.qualifiedName + ">{ "
				+ pyTag.qualifiedName + "::" + name + " }.GetIncref() );\n";
		}
	}
}


void PyspotMatchHandler::PyEnums::Flush( PyspotFrontendAction& action )
{
	action.AddEnumerator( definition );
}


PyspotMatchHandler::WrapperConstructors::WrapperConstructors( const PyTag& pyTag, const std::string& objectName )
{
	// Generate Wrapper constructors template specializations
	auto constructorName = "template<>\npyspot::Wrapper<" + pyTag.qualifiedName + ">::Wrapper( " + pyTag.qualifiedName;
	auto pyObjectReady   = ":\tpyspot::Object { ( PyType_Ready( &" + objectName + " ),\n"
		" \t                   PyspotWrapper_New( &" + objectName + ", nullptr, nullptr ) ) }\n";
	std::string getWrapper { "\tauto pWrapper = reinterpret_cast<_PyspotWrapper*>( mObject );\n" };

	// Pointer constructor
	pointer.name = constructorName + "* pV )";
	pointer.declaration = pointer.name + ";\n\n";
	pointer.definition  = pointer.name + "\n" + pyObjectReady
		+ ",\tpPayload { pV }\n{\n" + getWrapper + "\tpWrapper->pData = pPayload;\n}\n\n";

	auto pEnum  = pyTag.AsEnum();
	auto pClass = pyTag.AsClass();

	// Copy constructor
	if ( pEnum || ( pClass && pClass->hasCopyConstructorWithConstParam() ) )
	{
		copy.name = constructorName + "& v )";
		copy.declaration = copy.name + ";\n\n";
		copy.definition  = copy.name + "\n" + pyObjectReady
			+ ",\tpPayload { new " + pyTag.qualifiedName + "{ v } }\n{\n"
			+ getWrapper
			+ "\tpWrapper->pData = pPayload;\n\tpWrapper->bOwnData = true;\n}\n\n";
	}

	// Move constructor
	if ( pEnum || ( pClass && pClass->hasMoveConstructor() ) )
	{
		move.name = constructorName + "&& v )";
		move.declaration = move.name + ";\n\n";
		move.definition  = move.name + "\n" + pyObjectReady + ",\tpPayload { new "
			+ pyTag.qualifiedName + "{ std::move( v ) } }\n{\n" + getWrapper
			+ "\tpWrapper->pData = pPayload;\n\tpWrapper->bOwnData = true;\n}\n\n";
	}
}


void PyspotMatchHandler::WrapperConstructors::Flush( PyspotFrontendAction& action )
{
	action.AddWrapperDeclaration( pointer.declaration );
	action.AddWrapperDefinition( pointer.definition );
	action.AddWrapperDeclaration( copy.declaration );
	action.AddWrapperDefinition( copy.definition );
	action.AddWrapperDeclaration( move.declaration );
	action.AddWrapperDefinition( move.definition );
}


bool PyspotMatchHandler::checkHandled( const PyTag& pyTag )
{
	// Check if we have already processed this
	auto found = pyspot::find( m_Frontend.GetHandled(), pyTag.qualifiedName );
	auto bFound = ( found != std::end( m_Frontend.GetHandled() ) );
	if ( !bFound )
	{
		// Add this to the handled decls
		m_Frontend.AddHandled( pyTag.qualifiedName );
	}
	return bFound;
}


void PyspotMatchHandler::PyTag::Flush( PyspotFrontendAction& action )
{
	destructor.Flush( action );

	// Get public members
	if ( auto pClass = AsClass() )
	{
		for ( auto pField : pClass->fields() )
		{
			auto bPublic = ( pField->getAccess() == AS_public );
			if ( !bPublic )
			{
				continue;
			}

			// Field
			PyField field { *this, pField };
			field.decl.Flush( action );

			// Getter
			PyGetter getter { field };
			getter.decl.Flush( action );

			// Setter
			PySetter setter { field };
			setter.decl.Flush( action );

			accessors.Add( field, getter, setter );
		}
	}

	accessors.Flush( action );

	if ( auto pClass = AsClass() )
	{
		for ( auto pMethod : pClass->methods() )
		{
			auto bPublic = ( pMethod->getAccess() == AS_public );
			auto bOverloaded = pMethod->isOverloadedOperator();
			auto bConstructor = ( pMethod->getKind() == pMethod->CXXConstructor );
			auto bDestructor = ( pMethod->getKind() == pMethod->CXXDestructor );
			if ( !bPublic || bDestructor )
			{
				continue;
			}
			else if ( bConstructor )
			{
				auto pConstr = dyn_cast<CXXConstructorDecl>( pMethod );
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
				PyMethod method { *this, pMethod };
				methods.Add( method );
				method.Flush( action );
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
	auto reg = "\t// Register " + name + "\n"
		"\tif ( PyType_Ready( &" + typeObject.name + " ) < 0 )\n"
		"\t{\n\t\treturn Py_None;\n\t}\n"
		"\tPy_INCREF( &" + typeObject.name + " );\n"
		"\tPyModule_AddObject( pModule, \"" + name
		+ "\" , reinterpret_cast<PyObject*>( &" + typeObject.name + " ) );\n\n";
	action.AddClassRegistration( reg );

	WrapperConstructors constructors { *this, typeObject.name };
	constructors.Flush( action );
}


void PyspotMatchHandler::run( const ast_matchers::MatchFinder::MatchResult& result )
{
	// Save current context
	m_pContext = result.Context;

	// Get Tag (super class)
	auto pTag = result.Nodes.getNodeAs<TagDecl>( "PyspotTag" );
	if ( pTag == nullptr)
	{
		return; // Not the right type
	}

	PyTag pyTag { pTag };

	// Check if we have already processed it
	if ( checkHandled( pyTag ) )
	{
		return;
	}

	// Get include directive
	auto fileName = getIncludePath( pTag );
	m_Frontend.AddClassInclude( "#include \"" + fileName + "\"\n" );

	// Flush everything
	pyTag.Flush( m_Frontend );
}


PyspotConsumer::PyspotConsumer( PyspotFrontendAction& frontend )
:	m_HandlerForClassMatcher { frontend }
{
	// Match classes with pyspot attribute
	auto hasPyspot = ast_matchers::hasAttr( attr::Pyspot );
	auto pyMatcher = ast_matchers::decl( hasPyspot ).bind( "PyspotTag" );
	m_Matcher.addMatcher( pyMatcher, &m_HandlerForClassMatcher );
}


void PyspotConsumer::HandleTranslationUnit( ASTContext& context )
{
	// Run the matchers when we have the whole TU parsed.
	m_Matcher.matchAST( context );
}


Printer::Printer()
{}


PyspotFrontendAction::PyspotFrontendAction()
:	m_Printer { g_Printer }
{
}


void PyspotFrontendAction::AddGlobalInclude( const std::string& searchPath )
{
	m_GlobalIncludes.emplace_back( searchPath );
}


std::unique_ptr<ASTConsumer> PyspotFrontendAction::CreateASTConsumer( CompilerInstance& compiler, StringRef file )
{
	return llvm::make_unique<PyspotConsumer>( *this );
}


bool PyspotFrontendAction::BeginSourceFileAction( CompilerInstance& compiler )
{
	// Before executing the action get the global includes
	auto& preprocessor = compiler.getPreprocessor();
	auto& info = preprocessor.getHeaderSearchInfo();
	for ( auto dir = info.search_dir_begin(); dir != info.search_dir_end(); ++dir )
	{
		std::string directory = dir->getName();
		pyspot::replace_all( directory, "\\", "/" );
		AddGlobalInclude( directory );
	}

	return FrontendAction::BeginSourceFileAction( compiler );
}


#define OPEN_FILE_STREAM( file, name ) \
	std::error_code error; \
	llvm::raw_fd_ostream file { name, error, llvm::sys::fs::F_None };\
	if ( error ) \
	{ \
		llvm::errs() << " while opening '" << name << "': " << error.message() << '\n'; \
		exit( 1 ); \
	} \


void Printer::printBindingsHeader( StringRef name )
{
	OPEN_FILE_STREAM( file, name );

	// Guards
	std::string source { "#ifndef PYSPOT_BINDINGS_H_\n#define PYSPOT_BINDINGS_H_\n\n" };

	// Includes
	for ( auto& name : m_ClassIncludes )
	{
		source += name + "\n";
	}
	
	// Tail includes
	source += "\n#include <pyspot/Wrapper.h>\n#include <structmember.h>\n\n";

	// Extern C
	source += "\n#ifdef __cplusplus\nextern \"C\" {\n#endif // __cplusplus\n\n";

	for ( auto& decl : m_ClassDeclarations )
	{
		source += decl;
	}

	// End extern C
	source += "\n#ifdef __cplusplus\n} // extern \"C\"\n#endif // __cplusplus\n\n";

	// Wrapper constructors declarations
	for ( auto& constr : m_WrapperDeclarations )
	{
		source += constr;
	}

	// End guards
	source += "\n#endif // PYSPOT_BINDINGS_H_\n";

	file << source;
}


void Printer::printBindingsSource( StringRef name )
{
	OPEN_FILE_STREAM( file, name );

	auto include = "#include \"" + name.slice(4, name.size() - 3 ) + "h\"\n\n#include <Python.h>\n\n";
	auto source = include.str();

	// Class binding definitions
	for ( auto& def : m_ClassDefinitions )
	{
		source += def;
	}

	// Wrapper constructors definitions
	for ( auto& constr : m_WrapperDefinitions )
	{
		source += constr;
	}

	file << source;
}


void Printer::printExtensionHeader( StringRef name )
{
	OPEN_FILE_STREAM( file, name );

	std::string source {
		// Guards
		"#ifndef PYSPOT_EXTENSION_H_\n"
		"#define PYSPOT_EXTENSION_H_\n\n"
		"#include <Python.h>\n\n"

		// Extern C
		"\n#ifdef __cplusplus\nextern \"C\" {\n#endif // __cplusplus\n\n"

		"extern PyObject* g_pPyspotError;\n"
		"extern char g_aPyspotErrorName[13];\n"
		"extern char g_aErrorName[6];\n\n"
		"struct ModuleState\n{\n\tPyObject* error;\n};\n\n"
		"extern char g_aPyspotDescription[7];\n\n"
		"extern PyModuleDef g_sModuleDef;\n\n"

		// End extern C
		"\n#ifdef __cplusplus\n} // extern \"C\"\n#endif // __cplusplus\n\n"

		"PyMODINIT_FUNC PyInit_extension();\n\n"

		// End guards
		"#endif // PYSPOT_EXTENSION_H_\n"
	};

	file << source;
}


void Printer::printExtensionSource( StringRef name )
{
	OPEN_FILE_STREAM( file, name );

	std::string source {
		"#include \"" + name.slice(4, name.size() - 3 ).str() + "h\"\n\n"
		"#include \"pyspot/Bindings.h\"\n\n"
		"PyObject* g_pPyspotError = nullptr;\n"
		"char g_aPyspotErrorName[] { \"pyspot.error\" };\n"
		"char g_aErrorName[] { \"error\" };\n"
		"char g_aPyspotDescription[] { \"Pyspot\" };\n\n"
		"PyModuleDef g_sModuleDef {\n"
		"\tPyModuleDef_HEAD_INIT,\n"
		"\tg_aPyspotDescription,\n"
		"\tnullptr,\n"
		"\tsizeof( ModuleState ),\n"
		"\tnullptr,\n"
		"\tnullptr,\n"
		"\tnullptr,\n"
		"\tnullptr,\n"
		"\tnullptr,\n"
		"};\n\n"
		// Extension function
		"PyMODINIT_FUNC PyInit_extension()\n{\n"
		"\t// Create the module\n"
		"\tPyObject* pModule { PyModule_Create( &g_sModuleDef ) };\n"
		"\tif ( pModule == nullptr )\n\t{\n\t\treturn Py_None;\n\t}\n\n"
		"\t// Module exception\n"
		"\tg_pPyspotError = PyErr_NewException( g_aPyspotErrorName, nullptr, nullptr );\n"
		"\tPy_INCREF( g_pPyspotError );\n"
		"\tPyModule_AddObject( pModule, g_aErrorName, g_pPyspotError );\n\n" };

	// Add objects to module
	for ( auto& reg : m_ClassRegistrations )
	{
		source += reg;
	}

	// Add enumerators
	for ( auto& item : m_Enumerators )
	{
		source += item;
	}

	// Return the module
	source += "\treturn pModule;\n}\n";

	file << source;
}


void Printer::PrintOut()
{
	llvm::sys::fs::create_directory( "include" );
	llvm::sys::fs::create_directory( "src" );
	llvm::sys::fs::create_directory( "include/pyspot" );
	llvm::sys::fs::create_directory( "src/pyspot" );
	printBindingsHeader( "include/pyspot/Bindings.h" );
	printBindingsSource( "src/pyspot/Bindings.cpp" );
	printExtensionHeader( "include/pyspot/Extension.h" );
	printExtensionSource( "src/pyspot/Extension.cpp" );
}


int main( int argc, const char** argv )
{
	// Applied to all command-line options
	llvm::cl::OptionCategory pyspotCategory { "Pyspot options" };
	// Parse the command-line args passed to your code
	tooling::CommonOptionsParser op { argc, argv, pyspotCategory };
	// Create a new Clang Tool instance (a LibTooling environment)
	tooling::ClangTool tool { op.getCompilations(), op.getSourcePathList() };

	// Run the Clang Tool, creating a new FrontendAction
	auto result = tool.run( tooling::newFrontendActionFactory<PyspotFrontendAction>().get() );
	if ( result == EXIT_SUCCESS )
	{
		g_Printer.PrintOut();
	}

	return result;
}
