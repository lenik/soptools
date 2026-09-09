/*
 * Copyright (C) 2026 Lenik <soptools@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#define _POSIX_C_SOURCE 200809L

#include "sop_model.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <dirent.h>
#include <fstream>
#include <regex>
#include <sstream>
#include <sys/stat.h>

namespace {

std::string trim(const std::string &s) {
    size_t b = 0;
    while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b]))) {
        b++;
    }
    size_t e = s.size();
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) {
        e--;
    }
    return s.substr(b, e - b);
}

std::string strip_quotes(const std::string &s) {
    if (s.size() >= 2 &&
        ((s.front() == '"' && s.back() == '"') || (s.front() == '\'' && s.back() == '\''))) {
        return s.substr(1, s.size() - 2);
    }
    return s;
}

bool looks_like_path(const std::string &line) {
    if (line.empty() || line.find('\n') != std::string::npos) {
        return false;
    }
    if (line.find(' ') != std::string::npos) {
        return false;
    }
    if (line.find('#') != std::string::npos) {
        return false;
    }
    static const std::regex ext(
        R"(^[\w./-]+\.(md|ts|tsx|js|jsx|json|sql|prisma|yaml|yml|txt|sh)$)",
        std::regex::icase);
    if (std::regex_match(line, ext)) {
        return true;
    }
    if (line.find('/') == std::string::npos) {
        return false;
    }
    static const std::regex re(R"(^[\w./-]+$)");
    return std::regex_match(line, re);
}

void parse_frontmatter_line(const std::string &line, SopStep &step) {
    const size_t colon = line.find(':');
    if (colon == std::string::npos) {
        return;
    }
    const std::string key = trim(line.substr(0, colon));
    const std::string value = strip_quotes(trim(line.substr(colon + 1)));
    if (key == "title") {
        step.title = value;
    } else if (key == "description") {
        step.description = value;
    } else if (key == "type") {
        if (value == "shell") {
            step.kind = SopStepKind::Shell;
        } else if (value == "ai") {
            step.kind = SopStepKind::AiOutput;
        } else if (value == "manual") {
            step.kind = SopStepKind::Manual;
        }
    } else if (key == "output" || key == "outputs") {
        std::stringstream ss(value);
        std::string item;
        while (std::getline(ss, item, ',')) {
            item = trim(item);
            if (!item.empty()) {
                step.output_paths.push_back(item);
            }
        }
    } else if (key == "complete" || key == "complete.files") {
        std::stringstream ss(value);
        std::string item;
        while (std::getline(ss, item, ',')) {
            item = trim(item);
            if (!item.empty()) {
                step.completion.files_exist.push_back(item);
            }
        }
    } else if (key == "complete.shell") {
        step.completion.wait_shell = value == "true" || value == "1" || value == "yes";
    } else if (key == "branch") {
        if (value == "parallel") {
            step.branch_kind = SopBranchKind::Parallel;
        } else if (value == "select") {
            step.branch_kind = SopBranchKind::Select;
        }
    }
}

void extract_markdown_metadata(SopStep &step) {
    const std::regex heading_re(R"(^#\s+(.+)$)", std::regex::multiline);
    std::smatch m;
    if (step.title.empty() && std::regex_search(step.body, m, heading_re)) {
        step.title = trim(m[1].str());
    }

    if (step.description.empty()) {
        std::istringstream in(step.body);
        std::string line;
        bool past_title = false;
        while (std::getline(in, line)) {
            line = trim(line);
            if (line.empty()) {
                continue;
            }
            if (!past_title) {
                if (line.rfind("#", 0) == 0) {
                    past_title = true;
                }
                continue;
            }
            if (line.rfind("#", 0) == 0 || line.rfind("```", 0) == 0) {
                break;
            }
            step.description = line;
            break;
        }
    }

    const std::regex bash_re(R"(```(?:bash|sh)\s*\n([\s\S]*?)```)", std::regex::icase);
    if (std::regex_search(step.body, m, bash_re)) {
        step.shell_script = trim(m[1].str());
    }

    const std::regex text_block_re(R"(```text\s*\n([\s\S]*?)```)", std::regex::icase);
    auto begin = std::sregex_iterator(step.body.begin(), step.body.end(), text_block_re);
    auto end = std::sregex_iterator();
    for (auto it = begin; it != end; ++it) {
        std::istringstream block(trim((*it)[1].str()));
        std::string line;
        while (std::getline(block, line)) {
            line = trim(line);
            if (line.empty() || line == "# only if needed") {
                continue;
            }
            if (looks_like_path(line)) {
                step.output_paths.push_back(line);
                if (step.completion.files_exist.empty()) {
                    step.completion.files_exist.push_back(line);
                }
            }
        }
    }

    step.output_paths.erase(
        std::unique(step.output_paths.begin(), step.output_paths.end()),
        step.output_paths.end());
    step.completion.files_exist.erase(
        std::unique(step.completion.files_exist.begin(), step.completion.files_exist.end()),
        step.completion.files_exist.end());
}

std::string strip_role_padding(const std::string &raw) {
    size_t i = 0;
    while (i < raw.size() && raw[i] == '_') {
        i++;
    }
    return raw.substr(i);
}

} /* namespace */

SopRole parse_role_slug(const std::string &slug) {
    if (slug == "sh" || slug == "shell") {
        return SopRole::Shell;
    }
    if (slug == "gpt") {
        return SopRole::Gpt;
    }
    if (slug == "codex") {
        return SopRole::Codex;
    }
    if (slug.rfind("alt_", 0) == 0) {
        return SopRole::AltCodex;
    }
    return SopRole::Other;
}

SopStepKind infer_step_kind(const SopStep &step) {
    if (step.kind != SopStepKind::Manual) {
        return step.kind;
    }
    if (step.role == SopRole::Shell || !step.shell_script.empty()) {
        return SopStepKind::Shell;
    }
    if (step.role == SopRole::Gpt || step.role == SopRole::Codex || step.role == SopRole::AltCodex) {
        return SopStepKind::AiOutput;
    }
    return SopStepKind::Manual;
}

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

std::optional<SopStep> parse_sop_file(const std::string &path) {
    std::ifstream in(path);
    if (!in) {
        return std::nullopt;
    }

    const size_t slash = path.find_last_of("/\\");
    const std::string filename = slash == std::string::npos ? path : path.substr(slash + 1);

    /* New: 020a.___gpt.create_prd.md / 020z._codex.create_prd.md
     * Legacy: 020gpt.create_prd.md / 020alt_codex.create_prd.md */
    static const std::regex name_re_new(R"(^(\d{3})([a-zA-Z])\.(_*[^.]+)\.([^.]+)\.md$)");
    static const std::regex name_re_legacy(R"(^(\d{3})([^.]+)\.([^.]+)\.md$)");
    std::smatch m;

    SopStep step;
    step.filename = filename;
    step.filepath = path;

    if (std::regex_match(filename, m, name_re_new)) {
        step.seq = std::stoi(m[1].str());
        step.variant = static_cast<char>(std::tolower(static_cast<unsigned char>(m[2].str()[0])));
        step.role_slug = strip_role_padding(m[3].str());
        step.name = m[4].str();
    } else if (std::regex_match(filename, m, name_re_legacy)) {
        step.seq = std::stoi(m[1].str());
        step.role_slug = m[2].str();
        step.name = m[3].str();
        step.variant = step.role_slug.rfind("alt_", 0) == 0 ? 'z' : 'a';
    } else {
        return std::nullopt;
    }

    step.role = parse_role_slug(step.role_slug);

    std::ostringstream body;
    body << in.rdbuf();
    std::string content = body.str();

    if (content.rfind("---", 0) == 0) {
        const size_t end = content.find("\n---", 3);
        if (end != std::string::npos) {
            std::istringstream fm(content.substr(3, end - 3));
            std::string line;
            while (std::getline(fm, line)) {
                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }
                parse_frontmatter_line(line, step);
            }
            content = content.substr(end + 4);
            if (!content.empty() && content[0] == '\n') {
                content.erase(0, 1);
            }
        }
    }

    step.body = content;
    extract_markdown_metadata(step);
    step.kind = infer_step_kind(step);
    if (step.kind == SopStepKind::Shell) {
        step.completion.wait_shell = true;
    }
    return step;
}

SopDefinition load_sop_directory(const std::string &dir) {
    SopDefinition def;
    def.sop_dir = dir;

    DIR *d = opendir(dir.c_str());
    if (!d) {
        return def;
    }

    struct dirent *ent;
    while ((ent = readdir(d)) != nullptr) {
        if (ent->d_name[0] == '.') {
            continue;
        }
        const std::string path = dir + "/" + ent->d_name;
        struct stat st;
        if (stat(path.c_str(), &st) != 0 || !S_ISREG(st.st_mode)) {
            continue;
        }
        auto step = parse_sop_file(path);
        if (!step) {
            continue;
        }
        const std::string id = sop_step_id(*step);
        if (def.steps.count(id) > 0) {
            continue;
        }
        def.steps.emplace(id, *step);
        if (std::find(def.seq_order.begin(), def.seq_order.end(), step->seq) == def.seq_order.end()) {
            def.seq_order.push_back(step->seq);
        }
        def.branch_groups[step->seq].seq = step->seq;
        def.branch_groups[step->seq].step_ids.push_back(id);
    }
    closedir(d);

    std::sort(def.seq_order.begin(), def.seq_order.end());
    for (auto &kv : def.branch_groups) {
        auto &ids = kv.second.step_ids;
        std::sort(ids.begin(), ids.end(), [&](const std::string &a, const std::string &b) {
            const SopStep &sa = def.steps.at(a);
            const SopStep &sb = def.steps.at(b);
            if (sa.variant != sb.variant) {
                return sa.variant < sb.variant;
            }
            if (sa.is_alt_branch() != sb.is_alt_branch()) {
                return !sa.is_alt_branch();
            }
            return sa.role_slug < sb.role_slug;
        });
        if (ids.size() <= 1) {
            kv.second.kind = SopBranchKind::None;
        } else {
            kv.second.kind = SopBranchKind::Select;
            for (const std::string &id : ids) {
                if (def.steps.at(id).branch_kind == SopBranchKind::Parallel) {
                    kv.second.kind = SopBranchKind::Parallel;
                    break;
                }
            }
        }
    }
    return def;
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
