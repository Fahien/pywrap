#ifndef PYWRAP_BINDING_CXXRECORD_H_
#define PYWRAP_BINDING_CXXRECORD_H_

#include "pywrap/binding/Tag.h"

namespace pywrap
{
namespace binding
{
/// Represents the Python bindings of a struct/union/class
class CXXRecord : public Tag
{
  public:
	/// Generates definition and declaration of Python bindings for a struct/union/class
	/// @param[in] rec CXXRecord to wrap
	CXXRecord( const clang::CXXRecordDecl* rec );

  private:
	/// CXXRecord decl
	const clang::CXXRecordDecl* record;
};

}  // namespace binding
}  // namespace pywrap

#endif  // PYWRAP_BINDING_CXXRECORD_H_
