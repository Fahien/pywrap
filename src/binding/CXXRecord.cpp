#include "pywrap/binding/CXXRecord.h"

namespace pywrap
{
namespace binding
{
CXXRecord::CXXRecord( const clang::CXXRecordDecl* rec, const Binding& parent ) : Tag{ rec, parent }, record{ rec }
{
}

}  // namespace binding
}  // namespace pywrap
