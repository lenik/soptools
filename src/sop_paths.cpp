/*
 * Copyright (C) 2026 Lenik <soptools@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#define _POSIX_C_SOURCE 200809L

#include "sop_paths.hpp"

#include <filesystem>

namespace fs = std::filesystem;

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

std::string resolve_sop_dir(const std::string &requested, const std::string &source_root) {
    if (!requested.empty()) {
        return fs::absolute(requested).string();
    }
#ifdef SOP_BUILTIN_DIR
    if (fs::is_directory(SOP_BUILTIN_DIR)) {
        return fs::absolute(SOP_BUILTIN_DIR).string();
    }
#endif
    const fs::path candidates[] = {
        fs::path(source_root) / "figma.sop",
        fs::current_path() / "figma.sop",
    };
    for (const auto &c : candidates) {
        std::error_code ec;
        if (fs::is_directory(c, ec)) {
            return fs::absolute(c).string();
        }
    }
    return fs::absolute(fs::path(source_root) / "figma.sop").string();
}
