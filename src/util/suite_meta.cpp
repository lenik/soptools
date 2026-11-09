/*
 * Copyright (C) 2026 Lenik <soptools@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "util/suite_meta.hpp"

#include "config.h"

#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

namespace {

std::string read_first_line(const fs::path &path) {
    std::ifstream in(path);
    if (!in) {
        return {};
    }
    std::string line;
    std::getline(in, line);
    while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' ' ||
                             line.back() == '\t')) {
        line.pop_back();
    }
    size_t i = 0;
    while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) {
        i++;
    }
    return line.substr(i);
}

bool path_is_file(const fs::path &p) {
    std::error_code ec;
    return fs::is_regular_file(p, ec);
}

std::string resolve_suite_icon_png(const std::string &suite_name) {
    if (suite_name.empty()) {
        return {};
    }
    const std::string file = suite_name + ".png";
    const fs::path candidates[] = {
#ifdef SOP_PKGDATADIR
        fs::path(SOP_PKGDATADIR) / "icons" / "suite" / file,
#endif
#ifdef SOP_BUILD_ICONDIR
        fs::path(SOP_BUILD_ICONDIR) / file,
#endif
        fs::path(SOURCE_ROOT) / "assets" / "icons" / "suite" / file,
        fs::current_path() / "assets" / "icons" / "suite" / file,
    };
    for (const auto &c : candidates) {
        if (path_is_file(c)) {
            return fs::absolute(c).string();
        }
    }
    return {};
}

} /* namespace */

SuiteMeta load_suite_meta(const std::string &sop_dir) {
    SuiteMeta meta;
    if (sop_dir.empty()) {
        return meta;
    }
    const fs::path branch_dir = fs::path(sop_dir).lexically_normal();
    const fs::path suite_dir = branch_dir.parent_path();
    const fs::path suite_root = suite_dir.parent_path();
    if (suite_root.filename() == "suite" && !suite_dir.filename().empty()) {
        meta.name = suite_dir.filename().string();
        meta.version = read_first_line(suite_dir / "VERSION");
        meta.icon_png = resolve_suite_icon_png(meta.name);
        return meta;
    }
    /* Custom -S/--sop-dir: show leaf directory name, no version. */
    if (!branch_dir.filename().empty() && branch_dir.filename() != "." &&
        branch_dir.filename() != "/") {
        meta.name = branch_dir.filename().string();
    }
    return meta;
}

std::string suite_badge_label(const SuiteMeta &meta) {
    if (meta.name.empty()) {
        return "SOP";
    }
    if (meta.version.empty()) {
        return meta.name;
    }
    return meta.name + " " + meta.version;
}
