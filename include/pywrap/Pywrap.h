#ifndef PYSPOT_PYWRAP_H_
#define PYSPOT_PYWRAP_H_

#include <sstream>
#include <string>

#include <llvm/Support/FileSystem.h>
#include <clang/Tooling/CommonOptionsParser.h>

#include "pywrap/Printer.h"
#include "pywrap/FrontendAction.h"
#include "pywrap/Consumer.h"
#include "pywrap/Util.h"


// Classes to be mapped to Pyspot
struct OutputStreams
{
	std::string headerString { "" };
	llvm::raw_string_ostream headerOS { headerString };
};




#endif // PYSPOT_PYWRAP_H_
