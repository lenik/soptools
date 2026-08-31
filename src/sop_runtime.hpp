#ifndef SOP_RUNTIME_HPP
#define SOP_RUNTIME_HPP

#include "sop_model.hpp"

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

struct SopWrittenFile {
    std::string relative_path;
    std::string absolute_path;
};

struct SopApplyResult {
    bool ok = false;
    std::vector<SopWrittenFile> files;
    std::vector<std::string> dynamic_step_files;
    std::string message;
};

struct SopShellRun {
    bool running = false;
    int exit_code = -1;
    std::string log;
};

enum class SopAutoRunState {
    Idle,
    Running,
    Paused,
};

using SopLogFn = std::function<void(int level, const std::string &message)>;
using SopNotifyFn = std::function<void()>;

class SopEngine {
public:
    explicit SopEngine(SopRuntimeOptions opts);

    SopRuntimeOptions &options() { return opts_; }
    const SopRuntimeOptions &options() const { return opts_; }
    SopDefinition &definition() { return def_; }
    const SopDefinition &definition() const { return def_; }

    void set_log_fn(SopLogFn fn) { log_fn_ = std::move(fn); }
    void set_notify_fn(SopNotifyFn fn) { notify_fn_ = std::move(fn); }

    bool load_sop();
    bool reload_sop(const std::string &sop_dir);
    void set_project_dir(const std::string &project_dir);
    void reset_session_defaults();
    void refresh_step_order();
    const std::vector<std::string> &active_steps() const { return active_steps_; }
    const std::map<int, std::string> &active_branches() const { return active_branches_; }
    const std::map<std::string, bool> &excluded_steps() const { return excluded_steps_; }

    SopStep *step_at(size_t index);
    const SopStep *step_at(size_t index) const;
    size_t current_index() const { return current_index_; }
    void set_current_index(size_t index);
    std::string current_step_id() const;
    void set_current_step_id(const std::string &step_id);

    bool is_included(const std::string &step_id) const;
    bool is_excluded(const std::string &step_id) const;
    void set_step_excluded(const std::string &step_id, bool excluded);

    bool activate_branch(int seq, const std::string &step_id);
    bool can_advance() const;
    bool advance();
    bool retreat();
    bool try_advance_with_run();
    bool execute_current(bool force);
    bool execute_step(const std::string &step_id, bool force);

    void poll_completion();
    SopApplyResult apply_ai_output(const std::string &step_id, const std::string &text);
    bool run_shell(const std::string &step_id);
    void stop_shell(const std::string &step_id);
    bool is_shell_running(const std::string &step_id) const;

    void mark_complete(const std::string &step_id, bool complete);
    bool is_complete(const std::string &step_id) const;
    bool is_started(const std::string &step_id) const;

    SopAutoRunState auto_run_state() const { return auto_run_state_; }
    void start_auto_run();
    void pause_auto_run();
    void tick_auto_run();

    void log(int level, const std::string &message) const;

private:
    SopRuntimeOptions opts_;
    SopDefinition def_;
    std::map<int, std::string> active_branches_;
    std::map<std::string, bool> excluded_steps_;
    std::vector<std::string> active_steps_;
    size_t current_index_ = 0;
    SopAutoRunState auto_run_state_ = SopAutoRunState::Idle;
    bool pending_advance_ = false;
    SopLogFn log_fn_;
    SopNotifyFn notify_fn_;
    mutable std::mutex shell_mu_;
    std::map<std::string, std::shared_ptr<SopShellRun>> shell_runs_;

    void update_step_status(SopStep &step);
    void check_pending_advance();
    bool file_exists(const std::string &rel) const;
    std::string project_path(const std::string &rel) const;
    void maybe_add_dynamic_steps(const std::vector<std::string> &paths);
    void notify_changed() const;
};

#endif /* SOP_RUNTIME_HPP */
