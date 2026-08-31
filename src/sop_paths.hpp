#ifndef SOP_PATHS_HPP
#define SOP_PATHS_HPP

#include <string>

std::string find_project_dir(const std::string &start_dir);
std::string resolve_sop_dir(const std::string &requested, const std::string &source_root);

#endif /* SOP_PATHS_HPP */
