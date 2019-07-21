#include "pywrap/MatchHandler.h"

#include <algorithm>
#include <sstream>

#include "pywrap/binding/CXXRecord.h"
#include "pywrap/binding/Enum.h"
#include "pywrap/binding/Function.h"
#include "pywrap/binding/Module.h"

namespace pywrap
{
std::string get_cwd()
{
	llvm::SmallString<64> cwd;
	llvm::sys::fs::current_path( cwd );
	return pywrap::replace_all( cwd.str().str(), "\\", "/" );
}

MatchHandler::MatchHandler( std::unordered_map<std::string, binding::Module>& m, FrontendAction& f )
    : modules{ m }, frontend{ f }
{
}

std::string MatchHandler::get_include_path( const clang::Decl& decl )
{
	auto location = decl.getLocation().printToString( context->getSourceManager() );
	pywrap::replace_all( location, "\\", "/" );

	// Remove include directories from path
	for ( auto& path : frontend.get_global_includes() )
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


binding::Module& MatchHandler::get_module( const clang::DeclContext& ctx )
{
	auto named_decl = clang::dyn_cast<clang::NamedDecl>( &ctx );

	auto id = named_decl->getQualifiedNameAsString();

	// Nested thing
	auto named_context = named_decl->getDeclContext();
	if ( !named_context->isTranslationUnit() )
	{
		auto& parent = get_module( *named_context );
		// Find the module within the children of the parent
		auto& children = parent.get_children();
		auto  it       = std::find_if( std::begin( children ), std::end( children ),
                                [id]( binding::Module& child ) { return child.get_id() == id; } );
		if ( it == std::end( children ) )
		{
			// Create if not found
			parent.add( binding::Module{ *named_decl, &parent } );
			return children.back();
		}
		// Return the module
		return *it;
	}

	// Find it between the root modules
	auto it = modules.find( id );
	if ( it == modules.end() )
	{
		// Create it the first time
		auto pr = modules.emplace( id, binding::Module{ *named_decl } );
		if ( pr.second )  // success
		{
			it = pr.first;
		}
	}
	return it->second;
}

template <typename B, typename D>
B MatchHandler::create_binding( const D& decl, const binding::Binding& parent )
{
	B binding{ decl, parent };
	binding.set_incl( get_include_path( decl ) );
	return binding;
}

void MatchHandler::generate_bindings( const clang::Decl& decl )
{
	// The decl should have a declaration context
	auto ctx = decl.getDeclContext();
	assert( ctx && "Decl should have a declaration context" );
	auto& module = get_module( *ctx );

	// Switch according to the decl
	if ( auto func_decl = clang::dyn_cast<clang::FunctionDecl>( &decl ) )
	{
		auto it = find_if( std::begin( module.get_functions() ), std::end( module.get_functions() ),
		                   [func_decl]( const binding::Function& func ) {
			                   return func.get_id() == func_decl->getQualifiedNameAsString();
		                   } );
		if ( it == std::end( module.get_functions() ) )
		{
			// Add the function to the module
			module.add( create_binding<binding::Function>( *func_decl, module ) );
		}
	}
	// Generate enum bindings
	else if ( auto enum_decl = clang::dyn_cast<clang::EnumDecl>( &decl ) )
	{
		auto it = find_if(
		    std::begin( module.get_enums() ), std::end( module.get_enums() ),
		    [enum_decl]( const binding::Enum& enu ) { return enu.get_id() == enum_decl->getQualifiedNameAsString(); } );
		if ( it == std::end( module.get_enums() ) )
		{
			// Add the enum to the module
			module.add( create_binding<binding::Enum>( *enum_decl, module ) );
		}
	}
	// Generate struct/union/class bindings
	else if ( auto record_decl = clang::dyn_cast<clang::CXXRecordDecl>( &decl ) )
	{
		auto it = find_if( std::begin( module.get_records() ), std::end( module.get_records() ),
		                   [record_decl]( const binding::CXXRecord& record ) {
			                   return record.get_id() == record_decl->getQualifiedNameAsString();
		                   } );
		if ( it == std::end( module.get_records() ) )
		{
			// It it is a template
			if ( record_decl->isTemplated() )
			{
				auto template_decl = record_decl->getDescribedClassTemplate();
				auto templ         = create_binding<binding::Template>( *template_decl, module );
				templ.init();

				// TODO handle specialization
				for ( auto spec_decl : template_decl->specializations() )
				{
					spec_decl->startDefinition();
					spec_decl->completeDefinition();
					auto spec = create_binding<binding::Specialization>( *spec_decl, module );
					spec.init();
					templ.add( spec );
					module.add( std::move( spec ) );
				}

				module.add( std::move( templ ) );
			}
			else
			{
				// Add the record to the module
				auto record = create_binding<binding::CXXRecord>( *record_decl, module );
				record.init();

				module.add( std::move( record ) );

				// Generate bindings for fields with custom types if not yet generated
				auto& rec = module.get_records().back();
				for ( auto& field : rec.get_fields() )
				{
					auto type = field.get_type();
					auto name = type.getAsString();

					// Skip std types
					if ( name.find( "std::" ) != std::string::npos )
					{
						continue;
					}

					if ( auto tag = field.get_type()->getAsTagDecl() )
					{
						generate_bindings( *tag );
					}
				}
			}
		}
	}
}

void MatchHandler::run( const clang::ast_matchers::MatchFinder::MatchResult& result )
{
	// Save current context
	context = result.Context;

	if ( auto decl = result.Nodes.getNodeAs<clang::Decl>( "PyspotTag" ) )
	{
		if ( auto annotate = decl->getAttr<clang::AnnotateAttr>() )
		{
			if ( annotate->getAnnotation() == "pyspot" )
			{
				// Generate bindings for a decl with pyspot annotation
				generate_bindings( *decl );
			}
		}
	}
}

}  // namespace pywrap