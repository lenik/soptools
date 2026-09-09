/*
 * Copyright (C) 2026 Lenik <soptools@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#define _POSIX_C_SOURCE 200809L

#include "sop_runtime.hpp"
#include "sop_ai_output.hpp"
#include "sop_gpt_response.hpp"
#include "sop_paths.hpp"
#include "sop_project.hpp"

#include "config.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

namespace fs = std::filesystem;

SopEngine::SopEngine(SopRuntimeOptions opts) : opts_(std::move(opts)) {}

void SopEngine::reset_session_defaults() {
    active_branches_.clear();
    excluded_steps_.clear();
    current_index_ = 0;
    pending_advance_ = false;
    auto_run_state_ = SopAutoRunState::Idle;
    {
        std::lock_guard<std::mutex> lock(shell_mu_);
        shell_runs_.clear();
    }
    for (auto &kv : def_.steps) {
        SopStep &step = kv.second;
        step.user_marked_complete = false;
        step.status_message.clear();
        if (step.is_user()) {
            step.status = SopStepStatus::User;
            step.status_message = "user instruction";
        } else {
            step.status = SopStepStatus::Pending;
        }
    }
    for (const auto &kv : def_.branch_groups) {
        if (kv.second.step_ids.size() <= 1) {
            continue;
        }
        for (const std::string &id : kv.second.step_ids) {
            if (!def_.steps.at(id).is_alt_branch()) {
                active_branches_[kv.first] = id;
                break;
            }
        }
        if (!active_branches_.count(kv.first)) {
            active_branches_[kv.first] = kv.second.step_ids.front();
        }
    }
    for (const auto &kv : def_.branch_groups) {
        if (kv.second.kind != SopBranchKind::Select || kv.second.step_ids.size() <= 1) {
            continue;
        }
        const auto ait = active_branches_.find(kv.first);
        const std::string active = ait != active_branches_.end() ? ait->second : kv.second.step_ids.front();
        for (const std::string &id : kv.second.step_ids) {
            excluded_steps_[id] = (id != active);
        }
    }
    refresh_step_order();
    for (auto &kv : def_.steps) {
        update_step_status(kv.second);
    }
}

bool SopEngine::load_sop() {
    def_ = load_sop_directory(opts_.sop_dir);
    if (def_.steps.empty()) {
        log(0, "no SOP steps found in " + opts_.sop_dir);
        return false;
    }
    reset_session_defaults();
    std::string config_error;
    if (!load_project_config(*this, &config_error)) {
        log(0, "project config: " + config_error);
    }
    log(1, "loaded " + std::to_string(def_.steps.size()) + " SOP steps from " + opts_.sop_dir);
    log(1, "project directory: " + opts_.project_dir);
    return true;
}

bool SopEngine::reload_sop(const std::string &sop_dir) {
    opts_.sop_dir = fs::absolute(sop_dir).string();
    return load_sop();
}

void SopEngine::set_project_dir(const std::string &project_dir) {
    opts_.project_dir = find_project_dir(project_dir);
}

void SopEngine::refresh_step_order() {
    active_steps_ = build_active_step_order(def_, active_branches_, excluded_steps_);
    if (current_index_ >= active_steps_.size()) {
        current_index_ = active_steps_.empty() ? 0 : active_steps_.size() - 1;
    }
}

SopStep *SopEngine::step_at(size_t index) {
    if (index >= active_steps_.size()) {
        return nullptr;
    }
    return &def_.steps.at(active_steps_[index]);
}

const SopStep *SopEngine::step_at(size_t index) const {
    if (index >= active_steps_.size()) {
        return nullptr;
    }
    return &def_.steps.at(active_steps_[index]);
}

void SopEngine::set_current_index(size_t index) {
    if (active_steps_.empty()) {
        current_index_ = 0;
        return;
    }
    if (index >= active_steps_.size()) {
        index = active_steps_.size() - 1;
    }
    current_index_ = index;
    log(2, "current step: " + active_steps_[current_index_]);
}

std::string SopEngine::current_step_id() const {
    if (current_index_ >= active_steps_.size()) {
        return {};
    }
    return active_steps_[current_index_];
}

void SopEngine::set_current_step_id(const std::string &step_id) {
    for (size_t i = 0; i < active_steps_.size(); i++) {
        if (active_steps_[i] == step_id) {
            set_current_index(i);
            return;
        }
    }
    log(0, "unknown position id: " + step_id);
}

bool SopEngine::activate_branch(int seq, const std::string &step_id) {
    const auto git = def_.branch_groups.find(seq);
    if (git == def_.branch_groups.end() || git->second.step_ids.size() <= 1) {
        return false;
    }
    if (!def_.steps.count(step_id)) {
        return false;
    }
    set_step_excluded(step_id, false);
    log(1, "activated branch " + step_id + " at seq " + std::to_string(seq));
    return true;
}

bool SopEngine::is_included(const std::string &step_id) const {
    return !is_excluded(step_id);
}

bool SopEngine::is_excluded(const std::string &step_id) const {
    const auto it = excluded_steps_.find(step_id);
    return it != excluded_steps_.end() && it->second;
}

void SopEngine::set_step_excluded(const std::string &step_id, bool excluded) {
    if (!def_.steps.count(step_id)) {
        return;
    }
    excluded_steps_[step_id] = excluded;
    const int seq = def_.steps.at(step_id).seq;
    const auto git = def_.branch_groups.find(seq);
    if (!excluded && git != def_.branch_groups.end() && git->second.kind == SopBranchKind::Select &&
        git->second.step_ids.size() > 1) {
        for (const std::string &id : git->second.step_ids) {
            if (id != step_id) {
                excluded_steps_[id] = true;
            }
        }
        active_branches_[seq] = step_id;
    }
    refresh_step_order();
    log(1, std::string(excluded ? "excluded " : "included ") + step_id);
    notify_changed();
}

bool SopEngine::is_started(const std::string &step_id) const {
    for (size_t i = 0; i <= current_index_ && i < active_steps_.size(); i++) {
        if (active_steps_[i] == step_id) {
            return true;
        }
    }
    return false;
}

bool SopEngine::is_complete(const std::string &step_id) const {
    const auto it = def_.steps.find(step_id);
    if (it == def_.steps.end()) {
        return false;
    }
    return it->second.status == SopStepStatus::Complete || it->second.user_marked_complete;
}

bool SopEngine::can_advance() const {
    return current_index_ + 1 < active_steps_.size();
}

bool SopEngine::advance() {
    if (!can_advance()) {
        return false;
    }
    set_current_index(current_index_ + 1);
    return true;
}

bool SopEngine::try_advance_with_run() {
    poll_completion();
    check_pending_advance();
    if (pending_advance_) {
        return false;
    }
    if (!can_advance()) {
        return false;
    }
    const SopStep *step = step_at(current_index_);
    if (!step) {
        return false;
    }
    const std::string sid = active_steps_[current_index_];
    if (is_complete(sid) || step->is_user()) {
        return advance();
    }
    if (is_shell_running(sid)) {
        pending_advance_ = true;
        return false;
    }
    if (step->status == SopStepStatus::Error) {
        return false;
    }
    if (!step->is_automatable()) {
        return advance();
    }
    pending_advance_ = true;
    return run_shell(sid);
}

void SopEngine::notify_changed() const {
    if (notify_fn_) {
        notify_fn_();
    }
}

bool SopEngine::execute_step(const std::string &step_id, bool force) {
    poll_completion();
    if (!def_.steps.count(step_id)) {
        return false;
    }
    SopStep &step = def_.steps[step_id];
    if (step.is_user()) {
        log(1, "user step requires manual action: " + step_id);
        return true;
    }
    if (!step.is_automatable()) {
        log(0, "step is not executable: " + step_id);
        return false;
    }
    if (is_shell_running(step_id)) {
        return false;
    }
    if (force) {
        step.user_marked_complete = false;
        {
            std::lock_guard<std::mutex> lock(shell_mu_);
            shell_runs_.erase(step_id);
        }
        update_step_status(step);
    } else if (is_complete(step_id)) {
        return true;
    }
    return run_shell(step_id);
}

bool SopEngine::execute_current(bool force) {
    if (current_index_ >= active_steps_.size()) {
        return false;
    }
    return execute_step(active_steps_[current_index_], force);
}

void SopEngine::check_pending_advance() {
    if (!pending_advance_) {
        return;
    }
    const SopStep *step = step_at(current_index_);
    if (!step) {
        pending_advance_ = false;
        return;
    }
    const std::string sid = active_steps_[current_index_];
    if (step->status == SopStepStatus::Error) {
        pending_advance_ = false;
        log(0, "advance cancelled due to error on " + sid);
        return;
    }
    if (is_shell_running(sid)) {
        return;
    }
    if (!is_complete(sid)) {
        return;
    }
    pending_advance_ = false;
    advance();
    if (auto_run_state_ == SopAutoRunState::Running) {
        tick_auto_run();
    }
}

bool SopEngine::retreat() {
    if (current_index_ == 0) {
        return false;
    }
    pending_advance_ = false;
    set_current_index(current_index_ - 1);
    return true;
}

void SopEngine::start_auto_run() {
    poll_completion();
    const bool from_pause = auto_run_state_ == SopAutoRunState::Paused;
    const SopStep *step = step_at(current_index_);
    if (!step) {
        auto_run_state_ = SopAutoRunState::Idle;
        return;
    }

    if (auto_run_state_ == SopAutoRunState::Idle && step->is_user()) {
        auto_run_state_ = SopAutoRunState::Paused;
        log(1, "auto-run paused on user step " + sop_step_id(*step));
        return;
    }

    auto_run_state_ = SopAutoRunState::Running;
    if (from_pause && step->is_user()) {
        if (current_index_ + 1 >= active_steps_.size()) {
            auto_run_state_ = SopAutoRunState::Idle;
            log(1, "auto-run finished");
            return;
        }
        set_current_index(current_index_ + 1);
        log(1, "auto-run resumed past user step");
    } else {
        log(1, "auto-run started");
    }
    tick_auto_run();
}

void SopEngine::pause_auto_run() {
    if (auto_run_state_ != SopAutoRunState::Running) {
        return;
    }
    auto_run_state_ = SopAutoRunState::Paused;
    log(1, "auto-run paused");
}

void SopEngine::tick_auto_run() {
    if (auto_run_state_ != SopAutoRunState::Running) {
        return;
    }
    poll_completion();
    const SopStep *step = step_at(current_index_);
    if (!step) {
        auto_run_state_ = SopAutoRunState::Idle;
        return;
    }

    if (step->is_user()) {
        auto_run_state_ = SopAutoRunState::Paused;
        log(1, "auto-run paused on user step " + sop_step_id(*step));
        return;
    }

    if (!step->is_automatable()) {
        auto_run_state_ = SopAutoRunState::Paused;
        log(1, "auto-run paused: step is not automatable " + sop_step_id(*step));
        return;
    }

    const std::string sid = active_steps_[current_index_];
    if (is_shell_running(sid)) {
        return;
    }

    if (step->status == SopStepStatus::Error) {
        auto_run_state_ = SopAutoRunState::Paused;
        log(0, "auto-run paused on error: " + sid);
        return;
    }

    if (is_complete(sid)) {
        if (current_index_ + 1 >= active_steps_.size()) {
            auto_run_state_ = SopAutoRunState::Idle;
            log(1, "auto-run finished");
            return;
        }
        set_current_index(current_index_ + 1);
        tick_auto_run();
        return;
    }

    run_shell(sid);
}

void SopEngine::log(int level, const std::string &message) const {
    if (opts_.verbose < 0 && level > 0) {
        return;
    }
    if (level > opts_.verbose && opts_.verbose >= 0) {
        return;
    }
    if (log_fn_) {
        log_fn_(level, message);
    }
}

std::string SopEngine::project_path(const std::string &rel) const {
    return (fs::path(opts_.project_dir) / rel).string();
}

bool SopEngine::file_exists(const std::string &rel) const {
    std::error_code ec;
    return fs::exists(project_path(rel), ec);
}

void SopEngine::update_step_status(SopStep &step) {
    if (step.is_user()) {
        step.status = SopStepStatus::User;
        step.status_message = "user instruction";
        return;
    }

    if (step.user_marked_complete) {
        step.status = SopStepStatus::Complete;
        step.status_message = "marked complete";
        return;
    }

    const std::string sid = sop_step_id(step);
    {
        std::lock_guard<std::mutex> lock(shell_mu_);
        auto sit = shell_runs_.find(sid);
        if (sit != shell_runs_.end() && sit->second) {
            if (sit->second->running) {
                step.status = SopStepStatus::Running;
                step.status_message = "shell running";
                return;
            }
            if (step.completion.wait_shell) {
                if (sit->second->exit_code == 0) {
                    step.status = SopStepStatus::Complete;
                    step.status_message = "shell exited 0";
                } else if (sit->second->exit_code >= 0) {
                    step.status = SopStepStatus::Error;
                    step.status_message = "shell exited " + std::to_string(sit->second->exit_code);
                } else {
                    step.status = SopStepStatus::Pending;
                    step.status_message = "ready to run shell";
                }
                return;
            }
        }
    }

    if (step.kind == SopStepKind::Shell && step.completion.wait_shell) {
        step.status = SopStepStatus::Pending;
        step.status_message = "ready to run shell";
        return;
    }

    if (!step.completion.files_exist.empty()) {
        bool all_exist = true;
        for (const std::string &f : step.completion.files_exist) {
            if (!file_exists(f)) {
                all_exist = false;
                break;
            }
        }
        if (all_exist) {
            step.status = SopStepStatus::Complete;
            step.status_message = "expected files present";
            return;
        }
        step.status = SopStepStatus::Waiting;
        step.status_message = "waiting for output files";
        return;
    }

    if (step.kind == SopStepKind::Shell && !step.shell_script.empty()) {
        step.status = SopStepStatus::Pending;
        step.status_message = "ready to run shell";
        return;
    }

    if (step.kind == SopStepKind::AiOutput && !step.output_paths.empty()) {
        step.status = SopStepStatus::Waiting;
        step.status_message = "waiting for AI output";
        return;
    }

    step.status = SopStepStatus::Pending;
    step.status_message = "pending";
}

void SopEngine::poll_completion() {
    for (auto &kv : def_.steps) {
        update_step_status(kv.second);
    }
    check_pending_advance();
}

void SopEngine::mark_complete(const std::string &step_id, bool complete) {
    if (!def_.steps.count(step_id)) {
        return;
    }
    def_.steps[step_id].user_marked_complete = complete;
    update_step_status(def_.steps[step_id]);
    log(1, complete ? "marked complete: " + step_id : "marked incomplete: " + step_id);
}

bool SopEngine::is_shell_running(const std::string &step_id) const {
    std::lock_guard<std::mutex> lock(shell_mu_);
    auto it = shell_runs_.find(step_id);
    return it != shell_runs_.end() && it->second && it->second->running;
}

double SopEngine::shell_progress_pct(const std::string &step_id) const {
    std::lock_guard<std::mutex> lock(shell_mu_);
    auto it = shell_runs_.find(step_id);
    if (it == shell_runs_.end() || !it->second) {
        return -1.0;
    }
    return it->second->progress_pct;
}

std::string SopEngine::shell_progress_label(const std::string &step_id) const {
    std::lock_guard<std::mutex> lock(shell_mu_);
    auto it = shell_runs_.find(step_id);
    if (it == shell_runs_.end() || !it->second) {
        return {};
    }
    return it->second->progress_label;
}

bool SopEngine::run_shell(const std::string &step_id) {
    if (!def_.steps.count(step_id)) {
        return false;
    }
    SopStep &step = def_.steps[step_id];
    if (step.shell_script.empty()) {
        log(0, "no shell script for " + step_id);
        return false;
    }
    if (is_shell_running(step_id)) {
        log(1, "shell already running for " + step_id);
        return false;
    }

    auto run = std::make_shared<SopShellRun>();
    {
        std::lock_guard<std::mutex> lock(shell_mu_);
        shell_runs_[step_id] = run;
    }

    step.status = SopStepStatus::Running;
    step.status_message = "starting shell";
    log(1, "running shell for " + step_id);
    notify_changed();

    const std::string project_dir = opts_.project_dir;
    const std::string sop_dir = opts_.sop_dir;
    const int loglevel = opts_.verbose;
    const std::string script_path = step.filepath;
    const bool run_file = script_path.size() >= 3 &&
                          script_path.compare(script_path.size() - 3, 3, ".sh") == 0;

    std::thread([this, step_id, run, script = step.shell_script, project_dir, sop_dir, loglevel,
                 script_path, run_file]() {
        run->running = true;

#ifdef SOURCE_ROOT
        const std::string source_root = SOURCE_ROOT;
#else
        const std::string source_root;
#endif
        const std::string sopenv_bash = resolve_sopenv_bash_dir(source_root);
        const char *old_path = getenv("PATH");
        const std::string path_value =
            sopenv_bash + ":" + (old_path && old_path[0] ? old_path : "/usr/bin:/bin");

        /* Named pipe for script set_progress <-> sopwin (this script only). */
        char fifo_tmpl[] = "/tmp/sop-progress-XXXXXX";
        const int fifo_dir_fd = mkstemp(fifo_tmpl);
        std::string fifo_path;
        int fifo_hold_fd = -1;
        if (fifo_dir_fd >= 0) {
            close(fifo_dir_fd);
            unlink(fifo_tmpl);
            fifo_path = std::string(fifo_tmpl) + ".fifo";
            if (mkfifo(fifo_path.c_str(), 0600) == 0) {
                /* Keep RDWR open so writers do not block/fail when we are between reads. */
                fifo_hold_fd = open(fifo_path.c_str(), O_RDWR | O_CLOEXEC);
                run->progress_fifo = fifo_path;
            } else {
                fifo_path.clear();
            }
        }

        std::thread progress_reader;
        if (fifo_hold_fd >= 0) {
            progress_reader = std::thread([this, step_id, run, fifo_hold_fd]() {
                std::string pending;
                std::array<char, 256> buf{};
                for (;;) {
                    fd_set rfds;
                    FD_ZERO(&rfds);
                    FD_SET(fifo_hold_fd, &rfds);
                    timeval tv{};
                    tv.tv_sec = 0;
                    tv.tv_usec = 200000; /* 200ms */
                    const int ready = select(fifo_hold_fd + 1, &rfds, nullptr, nullptr, &tv);
                    if (ready > 0 && FD_ISSET(fifo_hold_fd, &rfds)) {
                        const ssize_t n = read(fifo_hold_fd, buf.data(), buf.size());
                        if (n > 0) {
                            pending.append(buf.data(), static_cast<size_t>(n));
                        } else if (n == 0 && !run->running) {
                            break;
                        } else if (n < 0 && errno != EINTR && errno != EAGAIN) {
                            break;
                        }
                    } else if (ready < 0 && errno != EINTR) {
                        break;
                    }

                    size_t pos;
                    while ((pos = pending.find('\n')) != std::string::npos) {
                        std::string line = pending.substr(0, pos);
                        pending.erase(0, pos + 1);
                        if (line.rfind("progress ", 0) != 0) {
                            continue;
                        }
                        std::string value = line.substr(9);
                        while (!value.empty() &&
                               (value.back() == '%' || value.back() == ' ' || value.back() == '\r')) {
                            value.pop_back();
                        }
                        char *end = nullptr;
                        const double pct = std::strtod(value.c_str(), &end);
                        if (end == value.c_str()) {
                            continue;
                        }
                        double clamped = pct;
                        if (clamped < 0) {
                            clamped = 0;
                        }
                        if (clamped > 100) {
                            clamped = 100;
                        }
                        {
                            std::lock_guard<std::mutex> lock(shell_mu_);
                            run->progress_pct = clamped;
                            char label[32];
                            if (clamped == static_cast<double>(static_cast<int>(clamped))) {
                                std::snprintf(label, sizeof(label), "%d%%", static_cast<int>(clamped));
                            } else {
                                std::snprintf(label, sizeof(label), "%.2f%%", clamped);
                            }
                            run->progress_label = label;
                            auto sit = def_.steps.find(step_id);
                            if (sit != def_.steps.end()) {
                                sit->second.status_message = std::string("progress ") + label;
                            }
                        }
                        notify_changed();
                    }

                    if (!run->running && pending.empty()) {
                        break;
                    }
                }
            });
        }

        std::ostringstream env_prefix;
        env_prefix << "cd " << shell_single_quote(project_dir)
                   << " && PATH=" << shell_single_quote(path_value)
                   << " SOP_PROJECT_DIR=" << shell_single_quote(project_dir)
                   << " SOP_DIR=" << shell_single_quote(sop_dir)
                   << " SOP_STEP_ID=" << shell_single_quote(step_id)
                   << " SOP_LOGLEVEL=" << loglevel << " LOGLEVEL=" << loglevel << " ";
        if (!fifo_path.empty()) {
            env_prefix << "SOP_PROGRESS_FIFO=" << shell_single_quote(fifo_path) << " ";
        }

        std::string cmd;
        if (run_file) {
            cmd = env_prefix.str() + "bash -- " + shell_single_quote(script_path);
            run->log += "$ cd " + project_dir + "\n";
            run->log += "$ # PATH includes " + sopenv_bash + "\n";
            if (!fifo_path.empty()) {
                run->log += "$ # SOP_PROGRESS_FIFO=" + fifo_path + "\n";
            }
            run->log += "$ bash -- " + script_path + "\n";
        } else {
            std::string wrapped = ". sop-script\n" + script;
            cmd = env_prefix.str() + "bash -lc " + shell_single_quote(wrapped);
            run->log += "$ cd " + project_dir + "\n";
            run->log += "$ # PATH includes " + sopenv_bash + "\n";
            run->log += script;
            if (!script.empty() && script.back() != '\n') {
                run->log += "\n";
            }
        }

        FILE *pipe = popen(cmd.c_str(), "r");
        if (!pipe) {
            run->log += "failed to start shell\n";
            run->exit_code = 127;
            run->running = false;
            if (progress_reader.joinable()) {
                if (fifo_hold_fd >= 0) {
                    close(fifo_hold_fd);
                    fifo_hold_fd = -1;
                }
                progress_reader.join();
            }
            if (!fifo_path.empty()) {
                unlink(fifo_path.c_str());
            }
            ensure_sop_config_dirs(project_dir, sop_dir);
            const std::string log_path = sop_step_log_path(project_dir, sop_dir, step_id);
            std::ofstream log_out(log_path, std::ios::trunc);
            if (log_out) {
                log_out << run->log;
            }
            notify_changed();
            return;
        }

        std::array<char, 4096> buf{};
        while (fgets(buf.data(), static_cast<int>(buf.size()), pipe) != nullptr) {
            run->log += buf.data();
        }
        run->exit_code = pclose(pipe);
        if (run->exit_code == -1) {
            run->exit_code = 127;
        } else if (WIFEXITED(run->exit_code)) {
            run->exit_code = WEXITSTATUS(run->exit_code);
        } else {
            run->exit_code = 1;
        }
        run->running = false;

        if (fifo_hold_fd >= 0) {
            close(fifo_hold_fd);
            fifo_hold_fd = -1;
        }
        if (progress_reader.joinable()) {
            progress_reader.join();
        }
        if (!fifo_path.empty()) {
            unlink(fifo_path.c_str());
        }

        ensure_sop_config_dirs(project_dir, sop_dir);
        const std::string log_path = sop_step_log_path(project_dir, sop_dir, step_id);
        std::ofstream log_out(log_path, std::ios::trunc);
        if (log_out) {
            log_out << run->log;
        }
        log(1, "shell finished for " + step_id + " exit=" + std::to_string(run->exit_code));
        notify_changed();
    }).detach();

    return true;
}

void SopEngine::stop_shell(const std::string &step_id) {
    (void)step_id;
}

void SopEngine::maybe_add_dynamic_steps(const std::vector<std::string> &paths) {
    for (const std::string &rel : paths) {
        if (rel.find("request-for-refactor.md") == std::string::npos) {
            continue;
        }
        const fs::path dyn_path = fs::path(opts_.sop_dir) / "dynamic" / "refactor_schema.md";
        if (!fs::exists(dyn_path)) {
            continue;
        }
        auto step = parse_sop_file(dyn_path.string());
        if (!step) {
            continue;
        }
        const std::string id = sop_step_id(*step);
        if (def_.steps.count(id)) {
            continue;
        }
        def_.steps.emplace(id, *step);
        def_.branch_groups[step->seq].step_ids.push_back(id);
        log(1, "added dynamic step " + id);
    }
    refresh_step_order();
}

SopApplyResult SopEngine::apply_ai_output(const std::string &step_id, const std::string &text) {
    SopApplyResult result;
    if (!def_.steps.count(step_id)) {
        result.message = "unknown step";
        return result;
    }
    log(1, "applying AI output for " + step_id);
    result = split_and_write_ai_output(opts_.project_dir, text);
    for (const auto &f : result.files) {
        log(2, "wrote " + f.relative_path);
    }
    if (result.ok) {
        SopStep &step = def_.steps[step_id];
        for (const auto &f : result.files) {
            if (std::find(step.completion.files_exist.begin(), step.completion.files_exist.end(),
                          f.relative_path) == step.completion.files_exist.end()) {
                step.completion.files_exist.push_back(f.relative_path);
            }
        }
        std::vector<std::string> written;
        for (const auto &f : result.files) {
            written.push_back(f.relative_path);
        }
        maybe_add_dynamic_steps(written);
        update_step_status(step);
        notify_changed();
    }
    return result;
}

SopApplyResult SopEngine::apply_gpt_save_result(const std::string &step_id,
                                                const SopGptSaveResult &saved) {
    SopApplyResult result;
    if (!def_.steps.count(step_id)) {
        result.message = "unknown step";
        return result;
    }
    if (!saved.ok) {
        result.message = saved.message.empty() ? "GPT save failed" : saved.message;
        return result;
    }
    result.ok = true;
    result.message = saved.message;
    for (const auto &f : saved.files) {
        result.files.push_back({f.relative_path, f.absolute_path});
    }
    log(1, "applied GPT response for " + step_id + ": " + saved.message);
    SopStep &step = def_.steps[step_id];
    for (const auto &f : result.files) {
        log(2, "wrote " + f.relative_path);
        if (std::find(step.completion.files_exist.begin(), step.completion.files_exist.end(),
                      f.relative_path) == step.completion.files_exist.end()) {
            step.completion.files_exist.push_back(f.relative_path);
        }
    }
    std::vector<std::string> written;
    for (const auto &f : result.files) {
        written.push_back(f.relative_path);
    }
    maybe_add_dynamic_steps(written);
    step.user_marked_complete = true;
    update_step_status(step);
    notify_changed();
    return result;
}
