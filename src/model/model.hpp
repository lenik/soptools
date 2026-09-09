#ifndef MODEL_MODEL_HPP
#define MODEL_MODEL_HPP

#include <map>
#include <optional>
#include <string>
#include <vector>

enum class SopRole {
    Shell,
    Gpt,
    Codex,
    AltCodex,
    Other,
};

enum class SopStepKind {
    Shell,
    AiOutput,
    Manual,
};

enum class SopStepStatus {
    Pending,
    Running,
    Waiting,
    Complete,
    Error,
    User,
};

enum class SopBranchKind {
    None,
    Select,
    Parallel,
};

struct SopCompletionRule {
    std::vector<std::string> files_exist;
    bool wait_shell = false;
};

enum class SopFileLink {
    Default, /* resolve at save: inode on ext2/3/4, else copy */
    None,
    Inode,
    Sym,
};

enum class SopInteraction {
    None,   /* save only; no post-save dialog */
    Select, /* show rendered response / part picker after save */
};

struct SopStep {
    int seq = 0;
    /* 'a' = default / preferred; 'z' = optional / alternative branch. */
    char variant = 'a';
    std::string role_slug;
    SopRole role = SopRole::Other;
    std::string name;
    std::string filename;
    std::string filepath;
    std::string extension;

    std::string title;
    std::string description;
    std::string body;
    SopStepKind kind = SopStepKind::Manual;

    std::string shell_script;
    std::vector<std::string> output_paths;
    SopCompletionRule completion;
    SopBranchKind branch_kind = SopBranchKind::None;

    /* .get headers (GPT steps). */
    std::string save_as;
    SopFileLink file_link = SopFileLink::Default;
    std::vector<std::string> parse_formats;
    SopInteraction interaction = SopInteraction::None;

    SopStepStatus status = SopStepStatus::Pending;
    std::string status_message;
    bool user_marked_complete = false;

    std::string default_title() const;
    std::string display_title() const;
    std::string role_label() const;
    bool is_alt_branch() const;
    bool is_user() const;
    bool is_action() const;
    bool is_automatable() const;
    bool is_prompt_copy() const;
    bool is_gpt_get() const;
    bool has_parse_format(const std::string &fmt) const;
};

struct SopBranchGroup {
    int seq = 0;
    SopBranchKind kind = SopBranchKind::Select;
    std::vector<std::string> step_ids;
};

struct SopDefinition {
    std::string sop_dir;
    std::map<std::string, SopStep> steps;
    std::vector<int> seq_order;
    std::map<int, SopBranchGroup> branch_groups;
};

struct SopRuntimeOptions {
    std::string project_dir;
    std::string sop_dir;
    int verbose = 0;
    bool gui = true;
    bool console = false;
};

SopRole parse_role_slug(const std::string &slug);
SopStepKind infer_step_kind(const SopStep &step);
std::string sop_step_id(const SopStep &step);
std::string humanize_name(const std::string &name);
std::optional<SopStep> parse_sop_file(const std::string &path);
SopDefinition load_sop_directory(const std::string &dir);
std::vector<std::string> build_active_step_order(
    const SopDefinition &def,
    const std::map<int, std::string> &active_branches,
    const std::map<std::string, bool> &excluded = {});

#endif /* MODEL_MODEL_HPP */
