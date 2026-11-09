#ifndef ENGINE_STAGE_FILES_HPP
#define ENGINE_STAGE_FILES_HPP

#include "model/model.hpp"

#include <string>
#include <vector>

/* Expected project-relative paths for a step (Save-As + `backtick` paths). */
std::vector<std::string> expected_files_for_step(const SopStep &step);

/* Strip download suffixes: TODO(1).md / TUC(20260912-060309).md → TODO.md / TUC.md */
std::string normalize_stage_basename(const std::string &filename);

/* Simple glob: only '*' wildcards (e.g. TUC*.md). */
bool match_simple_glob(const std::string &pattern, const std::string &name);

struct SopStageClaimResult {
    std::vector<std::string> moved;   /* "src -> dest" */
    std::vector<std::string> extras;  /* leftover basenames in stagedir */
    std::string message;
};

/* Move matching stagedir files into project_dir/<expected>. Returns leftovers. */
SopStageClaimResult claim_stage_files(const std::string &project_dir,
                                      const std::string &stage_dir,
                                      const std::vector<std::string> &expected);

/* List non-directory entries in stagedir (basenames). */
std::vector<std::string> list_stage_basenames(const std::string &stage_dir);

/* Move every leftover file in stagedir to project_dir/sop/pending/. */
bool move_stage_extras_to_pending(const std::string &project_dir,
                                  const std::string &stage_dir,
                                  std::string *error = nullptr);

/* Delete every leftover file in stagedir. */
bool delete_stage_extras(const std::string &stage_dir, std::string *error = nullptr);

#endif /* ENGINE_STAGE_FILES_HPP */
