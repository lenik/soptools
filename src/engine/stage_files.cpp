/*
 * Copyright (C) 2026 Lenik <soptools@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "engine/stage_files.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <map>
#include <regex>
#include <set>
#include <system_error>

namespace fs = std::filesystem;

namespace {

bool looks_like_path(const std::string &s) {
    if (s.empty() || s.size() > 240) {
        return false;
    }
    if (s.find("://") != std::string::npos) {
        return false;
    }
    /* Require an extension or a slash. */
    const bool has_slash = s.find('/') != std::string::npos;
    const auto dot = s.rfind('.');
    const bool has_ext = dot != std::string::npos && dot + 1 < s.size() &&
                         std::isalnum(static_cast<unsigned char>(s[dot + 1]));
    if (!has_slash && !has_ext) {
        return false;
    }
    for (unsigned char c : s) {
        if (std::isalnum(c) || c == '/' || c == '.' || c == '_' || c == '-' || c == '+') {
            continue;
        }
        return false;
    }
    return true;
}

std::string normalize_expected_path(const std::string &raw) {
    std::string s = raw;
    while (!s.empty() && (s.front() == '/' || s.front() == '.')) {
        if (s.rfind("./", 0) == 0) {
            s.erase(0, 2);
            continue;
        }
        if (s.front() == '/') {
            s.erase(0, 1);
            continue;
        }
        break;
    }
    if (s.find('/') == std::string::npos) {
        /* Bare name mentioned in SOP prompts usually lands under sop/. */
        s = (fs::path("sop") / s).string();
    }
    return s;
}

} /* namespace */

std::string normalize_stage_basename(const std::string &filename) {
    const fs::path p(filename);
    const std::string stem = p.stem().string();
    const std::string ext = p.extension().string(); /* includes leading '.' */

    /* TODO(1) or TUC(20260912-060309) */
    static const std::regex numbered(R"(^(.+)\((\d+)\)$)");
    static const std::regex stamped(R"(^(.+)\((\d{8}-\d{6})\)$)");
    std::smatch m;
    if (std::regex_match(stem, m, stamped) || std::regex_match(stem, m, numbered)) {
        return m[1].str() + ext;
    }
    return filename;
}

bool match_simple_glob(const std::string &pattern, const std::string &name) {
    if (pattern.empty()) {
        return false;
    }
    size_t pi = 0;
    size_t ni = 0;
    size_t star_p = std::string::npos;
    size_t star_n = std::string::npos;
    while (ni < name.size()) {
        if (pi < pattern.size() && pattern[pi] == '*') {
            star_p = pi++;
            star_n = ni;
        } else if (pi < pattern.size() &&
                   (pattern[pi] == name[ni] || pattern[pi] == '?')) {
            pi++;
            ni++;
        } else if (star_p != std::string::npos) {
            pi = star_p + 1;
            ni = ++star_n;
        } else {
            return false;
        }
    }
    while (pi < pattern.size() && pattern[pi] == '*') {
        pi++;
    }
    return pi == pattern.size();
}

std::vector<std::string> expected_files_for_step(const SopStep &step) {
    std::vector<std::string> out;
    std::set<std::string> seen;

    auto add = [&](const std::string &raw) {
        if (raw.empty() || !looks_like_path(raw)) {
            return;
        }
        const std::string norm = normalize_expected_path(raw);
        if (seen.insert(norm).second) {
            out.push_back(norm);
        }
    };

    if (!step.save_as.empty()) {
        add(step.save_as);
    }
    for (const auto &rule : step.append_tos) {
        add(rule.dest);
    }

    /* Backtick paths: `sop/TODO.md` or `TODO.md` */
    static const std::regex tick(R"(`([^`\n]{1,200})`)");
    std::sregex_iterator it(step.body.begin(), step.body.end(), tick);
    const std::sregex_iterator end;
    for (; it != end; ++it) {
        add((*it)[1].str());
    }

    return out;
}

std::vector<std::string> list_stage_basenames(const std::string &stage_dir) {
    std::vector<std::string> names;
    if (stage_dir.empty()) {
        return names;
    }
    std::error_code ec;
    if (!fs::is_directory(stage_dir, ec)) {
        return names;
    }
    for (const auto &entry : fs::directory_iterator(stage_dir, ec)) {
        if (ec) {
            break;
        }
        if (!entry.is_regular_file(ec)) {
            continue;
        }
        names.push_back(entry.path().filename().string());
    }
    std::sort(names.begin(), names.end());
    return names;
}

SopStageClaimResult claim_stage_files(const std::string &project_dir,
                                      const std::string &stage_dir,
                                      const std::vector<std::string> &expected) {
    SopStageClaimResult result;
    if (stage_dir.empty() || project_dir.empty()) {
        result.message = "stage or project directory not set";
        return result;
    }
    std::error_code ec;
    if (!fs::is_directory(stage_dir, ec)) {
        result.message = "stage directory missing";
        return result;
    }

    /* basename (normalized) → expected relative path (first wins). */
    std::map<std::string, std::string> by_base;
    for (const auto &rel : expected) {
        const std::string base = fs::path(rel).filename().string();
        if (by_base.find(base) == by_base.end()) {
            by_base[base] = rel;
        }
    }

    const auto staged = list_stage_basenames(stage_dir);
    std::set<std::string> claimed_stage;

    for (const auto &name : staged) {
        const std::string norm = normalize_stage_basename(name);
        const auto it = by_base.find(norm);
        if (it == by_base.end()) {
            continue;
        }
        const fs::path src = fs::path(stage_dir) / name;
        const fs::path dest = fs::path(project_dir) / it->second;
        fs::create_directories(dest.parent_path(), ec);
        ec.clear();
        fs::remove(dest, ec); /* replace if present */
        ec.clear();
        fs::rename(src, dest, ec);
        if (ec) {
            /* Cross-device: copy + remove. */
            ec.clear();
            fs::copy_file(src, dest, fs::copy_options::overwrite_existing, ec);
            if (!ec) {
                fs::remove(src, ec);
            }
        }
        if (!ec) {
            result.moved.push_back(name + " -> " + it->second);
            claimed_stage.insert(name);
        }
    }

    for (const auto &name : list_stage_basenames(stage_dir)) {
        result.extras.push_back(name);
    }
    if (!result.moved.empty()) {
        result.message = "claimed " + std::to_string(result.moved.size()) + " file(s) from stage";
    } else if (!result.extras.empty()) {
        result.message = "no matching stage files; " + std::to_string(result.extras.size()) +
                         " leftover(s)";
    } else {
        result.message = "stage empty";
    }
    return result;
}

bool move_stage_extras_to_pending(const std::string &project_dir, const std::string &stage_dir,
                                  std::string *error) {
    if (stage_dir.empty()) {
        return true;
    }
    std::error_code ec;
    const fs::path pending = fs::path(project_dir) / "sop" / "pending";
    fs::create_directories(pending, ec);
    if (ec) {
        if (error) {
            *error = "cannot create sop/pending: " + ec.message();
        }
        return false;
    }
    for (const auto &name : list_stage_basenames(stage_dir)) {
        const fs::path src = fs::path(stage_dir) / name;
        fs::path dest = pending / name;
        if (fs::exists(dest, ec)) {
            dest = pending / (name + "." + std::to_string(fs::file_size(src, ec)));
        }
        ec.clear();
        fs::rename(src, dest, ec);
        if (ec) {
            ec.clear();
            fs::copy_file(src, dest, fs::copy_options::overwrite_existing, ec);
            if (!ec) {
                fs::remove(src, ec);
            }
        }
        if (ec) {
            if (error) {
                *error = "failed to move " + name + ": " + ec.message();
            }
            return false;
        }
    }
    return true;
}

bool delete_stage_extras(const std::string &stage_dir, std::string *error) {
    if (stage_dir.empty()) {
        return true;
    }
    std::error_code ec;
    for (const auto &name : list_stage_basenames(stage_dir)) {
        fs::remove(fs::path(stage_dir) / name, ec);
        if (ec) {
            if (error) {
                *error = "failed to delete " + name + ": " + ec.message();
            }
            return false;
        }
    }
    return true;
}
