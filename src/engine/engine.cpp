/*
 * Copyright (C) 2026 Lenik <soptools@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#define _POSIX_C_SOURCE 200809L

#include "engine/engine.hpp"
#include "engine/ai_output.hpp"
#include "engine/gpt_response.hpp"
#include "util/paths.hpp"
#include "engine/project.hpp"

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

