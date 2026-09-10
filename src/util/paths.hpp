#ifndef UTIL_PATHS_HPP
#define UTIL_PATHS_HPP

#include <string>

std::string find_project_dir(const std::string &start_dir);

/* Map LANG / LC_ALL / LC_MESSAGES to a suite branch: "default", "zh_CN", "ja". */
std::string suite_branch_from_env();

/* Resolve suite/<name>/<branch>/ under source, install prefix, or cwd. */
std::string resolve_suite_sop_dir(const std::string &suite, const std::string &branch,
                                  const std::string &source_root);

/* Explicit -S/--sop-dir wins; otherwise resolve suite + LANG branch. */
std::string resolve_sop_dir(const std::string &sop_dir_override, const std::string &suite,
                            const std::string &branch, const std::string &source_root);

std::string resolve_extension_bash_dir(const std::string &source_root);

/* Switch branch within the current suite (e.g. worldman default↔zh_CN↔ja). */
std::string resolve_suite_branch(const std::string &branch, const std::string &beside_sop_dir,
                                 const std::string &source_root);

std::string shell_single_quote(const std::string &value);

#endif /* UTIL_PATHS_HPP */
