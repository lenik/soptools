/*
 * Copyright (C) 2026 Lenik <soptools@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#define _POSIX_C_SOURCE 200809L

#include "model/model.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <dirent.h>
#include <fstream>
#include <regex>
#include <sstream>
#include <sys/stat.h>

std::string sop_step_id(const SopStep &step) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%03d%c%s", step.seq, step.variant, step.role_slug.c_str());
    return buf;
}

std::string humanize_name(const std::string &name) {
    std::string out;
    out.reserve(name.size() + 8);
    for (size_t i = 0; i < name.size(); i++) {
        if (i == 0) {
            out.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(name[i]))));
        } else if (name[i] == '_') {
            out.append(" ");
            if (i + 1 < name.size()) {
                out.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(name[i + 1]))));
                i++;
            }
        } else {
            out.push_back(name[i]);
        }
    }
    return out;
}

std::string SopStep::default_title() const {
    return humanize_name(name);
}

std::string SopStep::display_title() const {
    return title.empty() ? default_title() : title;
}

std::string SopStep::role_label() const {
    switch (role) {
    case SopRole::Shell:
        return "Shell";
    case SopRole::Gpt:
        return "GPT";
    case SopRole::Codex:
        return "Codex";
    case SopRole::AltCodex:
        return "Alt Codex";
    default:
        return role_slug;
    }
}

bool SopStep::is_alt_branch() const {
    if (variant == 'z' || variant == 'Z') {
        return true;
    }
    return role == SopRole::AltCodex || role_slug.rfind("alt_", 0) == 0;
}

bool SopStep::is_user() const {
    return role == SopRole::Gpt || role == SopRole::Codex || role == SopRole::AltCodex ||
           kind == SopStepKind::AiOutput;
}

bool SopStep::is_action() const {
    return !is_user();
}

bool SopStep::is_automatable() const {
    return is_action() && kind == SopStepKind::Shell && !shell_script.empty();
}

bool SopStep::is_prompt_copy() const {
    return role == SopRole::Gpt || role == SopRole::Codex || role == SopRole::AltCodex;
}

bool SopStep::is_gpt_get() const {
    return role == SopRole::Gpt &&
           (extension == "get" ||
            (filename.size() >= 4 && filename.compare(filename.size() - 4, 4, ".get") == 0));
}

bool SopStep::has_parse_format(const std::string &fmt) const {
    std::string want = fmt;
    for (char &ch : want) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    for (const auto &f : parse_formats) {
        if (f == want) {
            return true;
        }
    }
    return false;
}

std::vector<std::string> build_active_step_order(
    const SopDefinition &def,
    const std::map<int, std::string> &active_branches,
    const std::map<std::string, bool> &excluded) {
    auto is_excluded = [&](const std::string &id) {
        const auto it = excluded.find(id);
        return it != excluded.end() && it->second;
    };
    std::vector<std::string> order;
    for (int seq : def.seq_order) {
        const auto git = def.branch_groups.find(seq);
        if (git == def.branch_groups.end() || git->second.step_ids.empty()) {
            continue;
        }
        const SopBranchGroup &group = git->second;
        if (group.step_ids.size() == 1) {
            if (!is_excluded(group.step_ids.front())) {
                order.push_back(group.step_ids.front());
            }
            continue;
        }
        if (group.kind == SopBranchKind::Parallel) {
            for (const std::string &id : group.step_ids) {
                if (!is_excluded(id)) {
                    order.push_back(id);
                }
            }
            continue;
        }
        std::string chosen;
        const auto bit = active_branches.find(seq);
        if (bit != active_branches.end() && !is_excluded(bit->second)) {
            chosen = bit->second;
        } else {
            for (const std::string &id : group.step_ids) {
                if (!is_excluded(id)) {
                    chosen = id;
                    break;
                }
            }
        }
        if (!chosen.empty()) {
            order.push_back(chosen);
        }
    }
    return order;
}
