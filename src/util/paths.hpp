#ifndef UTIL_PATHS_HPP
#define UTIL_PATHS_HPP

#include <string>

std::string find_project_dir(const std::string &start_dir);
std::string resolve_sop_dir(const std::string &requested, const std::string &source_root);
std::string resolve_extension_bash_dir(const std::string &source_root);
/* Locate a shipped WorldMan pack by language id: "en", "zh_CN", "ja". */
std::string resolve_worldman_sop_pack(const std::string &lang, const std::string &beside_sop_dir,
                                     const std::string &source_root);
std::string shell_single_quote(const std::string &value);

#endif /* UTIL_PATHS_HPP */
