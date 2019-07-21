#pragma once
#include <string>
#include <vector>

#include <clang/Tooling/Tooling.h>

#include <pywrap/binding/Module.h>

namespace pywrap
{
class Printer
{
  public:
	/// Adds an include path to the set of processed ones
	/// @param[in] include Include path
	void add_include( const std::string& include )
	{
		processed_includes.emplace( include );
	}

	/// @brief Finishes handling the files
	void print_out( const std::unordered_map<std::string, binding::Module>& modules );

  private:
	/// @brief Prints bindings header
	/// @param[in] name Path of the output file
	void print_bindings_header( llvm::StringRef name );

	/// @brief Prints bindings source
	/// @param[in] name Path of the output file
	void print_bindings_source( llvm::StringRef name );

	/// @brief Prints extension header
	/// @param[in] name Path of the output file
	void print_extension_header( llvm::StringRef name );

	/// @brief Prints extension source
	/// @param[in] name Path of the output file
	void print_extension_source( llvm::StringRef name );

	/// Recursively process includes for a module and its submodules
	/// @param[in] file The current output stream
	/// @param[in] module The current module to process
	void process_includes( llvm::raw_fd_ostream& file, const binding::Module& module );

	/// Recursively process declarations for a module and its submodules
	/// @param[in] file The current output stream
	/// @param[in] module The current module to process
	void process_decls( llvm::raw_fd_ostream& file, const binding::Module& module );

	/// Recursively process wrappers for a module and its submodules
	/// @param[in] file The current output stream
	/// @param[in] module The current module to process
	void process_wrappers( llvm::raw_fd_ostream& file, const binding::Module& module );

	/// Recursively process definitions for a module and its submodules
	/// @param[in] file The current output stream
	/// @param[in] module The current module to process
	void process_defs( llvm::raw_fd_ostream& file, const binding::Module& module );

	const std::unordered_map<std::string, binding::Module>* modules;

	std::set<std::string> processed_includes;
};


}  // namespace pywrap
