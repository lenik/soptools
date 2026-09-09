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

std::string ascii_lower(std::string s) {
    for (char &c : s) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return s;
}

std::string strip_role_padding(const std::string &raw) {
    size_t i = 0;
    while (i < raw.size() && raw[i] == '_') {
        i++;
    }
    return raw.substr(i);
}

SopFileLink parse_file_link(const std::string &value) {
    const std::string v = ascii_lower(trim(value));
    if (v.empty() || v == "default" || v == "auto") {
        return SopFileLink::Default;
    }
    if (v == "none" || v == "copy") {
        return SopFileLink::None;
    }
    if (v == "inode" || v == "hard" || v == "hardlink") {
        return SopFileLink::Inode;
    }
    if (v == "sym" || v == "symlink" || v == "symbolic") {
        return SopFileLink::Sym;
    }
    return SopFileLink::Default;
}

SopInteraction parse_interaction(const std::string &value) {
    const std::string v = ascii_lower(trim(value));
    if (v == "select" || v == "ui" || v == "dialog") {
        return SopInteraction::Select;
    }
    return SopInteraction::None;
}

bool is_editor_modeline(const std::string &line) {
    const std::string t = trim(line);
    if (t.empty()) {
        return false;
    }
    /* Emacs: -*- mode: markdown -*-  or  -*- Mode: Markdown; -*- */
    if (t.find("-*-") != std::string::npos) {
        const std::string lower = ascii_lower(t);
        if (lower.find("mode:") != std::string::npos || lower.find("filetype:") != std::string::npos) {
            return true;
        }
        /* bare -*- markdown -*- also common */
        if (lower.find("markdown") != std::string::npos) {
            return true;
        }
    }
    /* Vim: vim: set ft=markdown :  /  vi: set filetype=markdown : */
    static const std::regex vim_re(
        R"(^(?:#\s*)?(?:vi|vim|ex):\s*.*\b(?:ft|filetype|syntax)\s*=\s*markdown\b.*)",
        std::regex::icase);
    if (std::regex_match(t, vim_re)) {
        return true;
    }
    /* Also ignore shorter "vim: set ft=markdown :" without requiring markdown in regex above —
     * already covered. Accept any vim modeline mentioning ft=/filetype=. */
    static const std::regex vim_any(
        R"(^(?:#\s*)?(?:vi|vim|ex):\s*set?\s+.*)",
        std::regex::icase);
    return std::regex_match(t, vim_any);
}

void parse_get_header_line(const std::string &line, SopStep &step) {
    if (is_editor_modeline(line)) {
        return;
    }
    const size_t colon = line.find(':');
    if (colon == std::string::npos) {
        return;
    }
    const std::string key = ascii_lower(trim(line.substr(0, colon)));
    const std::string value = trim(line.substr(colon + 1));
    if (key == "save-as" || key == "save_as" || key == "saveas") {
        step.save_as = value;
        if (!value.empty() &&
            std::find(step.output_paths.begin(), step.output_paths.end(), value) ==
                step.output_paths.end()) {
            step.output_paths.push_back(value);
        }
        if (!value.empty() &&
            std::find(step.completion.files_exist.begin(), step.completion.files_exist.end(),
                      value) == step.completion.files_exist.end()) {
            step.completion.files_exist.push_back(value);
        }
    } else if (key == "file-link" || key == "file_link" || key == "filelink") {
        step.file_link = parse_file_link(value);
    } else if (key == "parse") {
        std::stringstream ss(value);
        std::string item;
        while (std::getline(ss, item, ',')) {
            item = ascii_lower(trim(item));
            if (!item.empty()) {
                step.parse_formats.push_back(item);
            }
        }
    } else if (key == "interaction") {
        step.interaction = parse_interaction(value);
    } else if (key == "discard") {
        /* Editor modelines / ignored metadata, e.g. Discard: vim: set ft=markdown : */
        return;
    }
}

/* Leading RFC822-style headers on .get files until a blank line. */
std::string strip_get_headers(std::string content, SopStep &step) {
    std::istringstream in(content);
    std::string line;
    std::ostringstream rest;
    bool in_headers = true;
    bool saw_any = false;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (in_headers) {
            if (trim(line).empty()) {
                if (saw_any) {
                    in_headers = false;
                    continue;
                }
                /* Leading blank before headers — keep scanning. */
                continue;
            }
            if (is_editor_modeline(line)) {
                saw_any = true;
                continue;
            }
            if (line.find(':') != std::string::npos && line.rfind("#", 0) != 0) {
                parse_get_header_line(line, step);
                saw_any = true;
                continue;
            }
            /* Not a header line — treat remainder as body including this line. */
            in_headers = false;
            rest << line << '\n';
            continue;
        }
        /* Skip trailing/leading modelines that leaked into body. */
        if (is_editor_modeline(line) && rest.tellp() == 0) {
            continue;
        }
        rest << line << '\n';
    }
    if (!in.eof() && !content.empty() && content.back() != '\n') {
        /* keep trailing content from stream already handled */
    }
    std::string body = rest.str();
    if (body.empty() && !saw_any) {
        return content;
    }
    return body;
}


void extract_shell_script_metadata(SopStep &step) {
    std::istringstream in(step.body);
    std::string line;
    bool saw_shebang = false;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        const std::string t = trim(line);
        if (t.empty()) {
            continue;
        }
        if (!saw_shebang && t.rfind("#!", 0) == 0) {
            saw_shebang = true;
            continue;
        }
        if (step.title.empty() && t.rfind("#", 0) == 0) {
            std::string title = trim(t.substr(1));
            if (!title.empty()) {
                step.title = title;
            }
            break;
        }
        if (t.rfind("#", 0) != 0) {
            break;
        }
    }
    step.shell_script = step.body;
    step.kind = SopStepKind::Shell;
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

std::optional<SopStep> parse_sop_file(const std::string &path) {
    std::ifstream in(path);
    if (!in) {
        return std::nullopt;
    }

    const size_t slash = path.find_last_of("/\\");
    const std::string filename = slash == std::string::npos ? path : path.substr(slash + 1);

    /* New: 020a.___gpt.create_prd.get / 000a._shell.refactor_figma.sh
     * Legacy: 020gpt.create_prd.md / 020alt_codex.create_prd.md */
    static const std::regex name_re_new(R"(^(\d{3})([a-zA-Z])\.(_*[^.]+)\.([^.]+)\.(md|sh|get)$)");
    static const std::regex name_re_legacy(R"(^(\d{3})([^.]+)\.([^.]+)\.md$)");
    std::smatch m;

    SopStep step;
    step.filename = filename;
    step.filepath = path;

    std::string ext;
    if (std::regex_match(filename, m, name_re_new)) {
        step.seq = std::stoi(m[1].str());
        step.variant = static_cast<char>(std::tolower(static_cast<unsigned char>(m[2].str()[0])));
        step.role_slug = strip_role_padding(m[3].str());
        step.name = m[4].str();
        ext = m[5].str();
    } else if (std::regex_match(filename, m, name_re_legacy)) {
        step.seq = std::stoi(m[1].str());
        step.role_slug = m[2].str();
        step.name = m[3].str();
        step.variant = step.role_slug.rfind("alt_", 0) == 0 ? 'z' : 'a';
        ext = "md";
    } else {
        return std::nullopt;
    }

    step.extension = ext;
    step.role = parse_role_slug(step.role_slug);

    std::ostringstream body;
    body << in.rdbuf();
    std::string content = body.str();

    if (ext == "sh") {
        step.body = content;
        extract_shell_script_metadata(step);
        step.completion.wait_shell = true;
        return step;
    }

    if (ext == "get") {
        content = strip_get_headers(std::move(content), step);
    }

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

