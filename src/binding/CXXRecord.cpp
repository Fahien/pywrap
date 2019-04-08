#include "pywrap/binding/CXXRecord.h"

namespace pywrap
{
namespace binding
{

CXXRecord::CXXRecord( const clang::CXXRecordDecl* rec ) : Tag{ rec }, record{ rec }
{
	init();
}

}  // namespace binding
}  // namespace pywrap
