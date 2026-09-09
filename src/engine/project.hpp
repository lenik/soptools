#ifndef ENGINE_PROJECT_HPP
#define ENGINE_PROJECT_HPP

#include "engine/engine.hpp"

#include <string>

std::string sop_config_name(const std::string &sop_dir);
std::string sop_config_dir(const std::string &project_dir);
std::string sop_config_path(const std::string &project_dir, const std::string &sop_dir);
std::string sop_log_dir(const std::string &project_dir, const std::string &sop_dir);
std::string sop_step_log_path(const std::string &project_dir, const std::string &sop_dir,
                              const std::string &step_id);

bool ensure_sop_config_dirs(const std::string &project_dir, const std::string &sop_dir);

bool save_project_config(const SopEngine &engine, std::string *error = nullptr);
bool load_project_config(SopEngine &engine, std::string *error = nullptr);
bool revert_project_config(SopEngine &engine, std::string *error = nullptr);

#endif /* ENGINE_PROJECT_HPP */
