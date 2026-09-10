/*
 * Copyright (C) 2026 Lenik <soptools@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#define _POSIX_C_SOURCE 200809L

#include "engine/engine.hpp"
#include "engine/ai_output.hpp"
#include "engine/gpt_response.hpp"

#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

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
    persist_project_status();
    notify_changed();
    return result;
}
