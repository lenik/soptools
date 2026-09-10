/*
 * Copyright (C) 2026 Lenik <soptools@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#define _POSIX_C_SOURCE 200809L

#include "util/paths.hpp"

#include "config.h"

#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

std::string locale_tag_from_env() {
    const char *keys[] = {"LC_ALL", "LC_MESSAGES", "LANG"};
    for (const char *key : keys) {
        const char *v = getenv(key);
        if (v && v[0] && std::string(v) != "C" && std::string(v) != "POSIX") {
            return v;
        }
    }
    return {};
}

bool path_is_directory(const fs::path &p) {
    std::error_code ec;
    return fs::is_directory(p, ec);
}

} /* namespace */

std::string find_project_dir(const std::string &start_dir) {
    fs::path cur = fs::absolute(start_dir.empty() ? fs::current_path() : fs::path(start_dir));
    std::error_code ec;
    while (!cur.empty()) {
        if (fs::exists(cur / ".git", ec)) {
            return cur.string();
        }
        if (!cur.has_parent_path() || cur == cur.parent_path()) {
            break;
        }
        cur = cur.parent_path();
    }
    return fs::absolute(start_dir.empty() ? fs::current_path() : fs::path(start_dir)).string();
}

std::string suite_branch_from_env() {
    const std::string tag = locale_tag_from_env();
    if (tag.empty()) {
        return "default";
    }
    /* zh_CN, zh_CN.UTF-8, zh-CN, zh → zh_CN */
    if (tag.rfind("zh_CN", 0) == 0 || tag.rfind("zh-CN", 0) == 0 || tag.rfind("zh_Hans", 0) == 0 ||
        tag == "zh" || tag.rfind("zh.", 0) == 0) {
        return "zh_CN";
    }
    if (tag.rfind("ja", 0) == 0) {
        return "ja";
    }
    return "default";
}

std::string resolve_suite_sop_dir(const std::string &suite, const std::string &branch,
                                  const std::string &source_root) {
    const std::string suite_name = suite.empty() ? "worldman" : suite;
    std::string br = branch.empty() ? suite_branch_from_env() : branch;
    std::vector<fs::path> candidates = {
        fs::path(source_root) / "suite" / suite_name / br,
        fs::current_path() / "suite" / suite_name / br,
    };
#ifdef SOP_PKGDATADIR
    candidates.insert(candidates.begin(), fs::path(SOP_PKGDATADIR) / "suite" / suite_name / br);
#endif
#ifdef SOP_BUILTIN_DIR
    if (suite_name == "worldman" && br == "default" && path_is_directory(SOP_BUILTIN_DIR)) {
        candidates.insert(candidates.begin(), SOP_BUILTIN_DIR);
    }
#endif
    /* Fall back to default branch if LANG branch is missing. */
    if (br != "default") {
        candidates.push_back(fs::path(source_root) / "suite" / suite_name / "default");
        candidates.push_back(fs::current_path() / "suite" / suite_name / "default");
#ifdef SOP_PKGDATADIR
        candidates.push_back(fs::path(SOP_PKGDATADIR) / "suite" / suite_name / "default");
#endif
    }
    for (const auto &c : candidates) {
        if (path_is_directory(c)) {
            return fs::absolute(c).string();
        }
    }
    return fs::absolute(fs::path(source_root) / "suite" / suite_name / "default").string();
}

std::string resolve_sop_dir(const std::string &sop_dir_override, const std::string &suite,
                            const std::string &branch, const std::string &source_root) {
    if (!sop_dir_override.empty()) {
        return fs::absolute(sop_dir_override).string();
    }
    return resolve_suite_sop_dir(suite, branch, source_root);
}

std::string resolve_extension_bash_dir(const std::string &source_root) {
    std::error_code ec;
#ifdef SOP_PKGDATADIR
    {
        const fs::path installed = fs::path(SOP_PKGDATADIR) / "extension" / "bash";
        if (fs::is_directory(installed, ec)) {
            return fs::absolute(installed).string();
        }
    }
#endif
    const fs::path candidates[] = {
        fs::path(source_root) / "extension" / "bash",
        fs::current_path() / "extension" / "bash",
    };
    for (const auto &c : candidates) {
        if (fs::is_directory(c, ec)) {
            return fs::absolute(c).string();
        }
    }
    return fs::absolute(fs::path(source_root) / "extension" / "bash").string();
}

std::string resolve_suite_branch(const std::string &branch, const std::string &beside_sop_dir,
                                 const std::string &source_root) {
    std::string suite = "worldman";
    if (!beside_sop_dir.empty()) {
        const fs::path cur = fs::path(beside_sop_dir).lexically_normal();
        const fs::path parent = cur.parent_path();
        if (parent.filename() != "." && !parent.filename().empty()) {
            /* .../suite/<suite>/<branch> */
            if (parent.parent_path().filename() == "suite") {
                suite = parent.filename().string();
            } else {
                suite = parent.filename().string();
            }
        }
    }
    return resolve_suite_sop_dir(suite, branch, source_root);
}

std::string shell_single_quote(const std::string &value) {
    std::string out = "'";
    for (char ch : value) {
        if (ch == '\'') {
            out += "'\\''";
        } else {
            out.push_back(ch);
        }
    }
    out.push_back('\'');
    return out;
}
