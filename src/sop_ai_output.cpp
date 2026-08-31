/*
 * Copyright (C) 2026 Lenik <soptools@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#define _POSIX_C_SOURCE 200809L

#include "sop_ai_output.hpp"

#include <filesystem>
#include <fstream>
#include <regex>

namespace fs = std::filesystem;

namespace {

std::string trim(const std::string &s) {
    size_t b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) {
        return {};
    }
    size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

bool ensure_parent_dirs(const fs::path &path) {
    std::error_code ec;
    fs::create_directories(path.parent_path(), ec);
    return !ec;
}

} /* namespace */

SopApplyResult split_and_write_ai_output(const std::string &project_dir, const std::string &text) {
    SopApplyResult result;
    struct Segment {
        std::string path;
        std::string content;
    };
    std::vector<Segment> segments;

    const std::regex file_header(
        R"(^---+\s*(?:file:\s*)?([^\s-]+.*?)\s*---+\s*$)",
        std::regex::multiline);
    const std::regex fence_re(
        R"(```(?:[\w.+-]+(?::([^\s`]+))?|([^\s`]+))?\s*\n([\s\S]*?)```)",
        std::regex::multiline);

    std::sregex_iterator it(text.begin(), text.end(), file_header);
    std::sregex_iterator end;
    std::vector<std::pair<size_t, size_t>> header_spans;
    std::vector<std::string> header_paths;
    for (; it != end; ++it) {
        header_spans.emplace_back(static_cast<size_t>(it->position()), static_cast<size_t>(it->length()));
        header_paths.push_back(trim(it->str(1)));
    }

    if (!header_paths.empty()) {
        for (size_t i = 0; i < header_paths.size(); i++) {
            const size_t content_start = header_spans[i].first + header_spans[i].second;
            const size_t content_end =
                (i + 1 < header_spans.size()) ? header_spans[i + 1].first : text.size();
            segments.push_back({header_paths[i], trim(text.substr(content_start, content_end - content_start))});
        }
    } else {
        it = std::sregex_iterator(text.begin(), text.end(), fence_re);
        for (; it != end; ++it) {
            std::string path = trim(it->str(1));
            if (path.empty()) {
                path = trim(it->str(2));
            }
            const std::string content = it->str(3);
            if (!path.empty()) {
                segments.push_back({path, content});
            }
        }
    }

    if (segments.empty()) {
        segments.push_back({"sop-output/generated.txt", text});
    }

    for (const auto &seg : segments) {
        fs::path rel = seg.path;
        if (rel.is_absolute()) {
            rel = rel.lexically_relative(project_dir);
            if (rel.empty() || rel.string().rfind("..", 0) == 0) {
                rel = fs::path("sop-output") / fs::path(seg.path).filename();
            }
        }
        const fs::path abs = fs::path(project_dir) / rel;
        if (!ensure_parent_dirs(abs)) {
            result.message = "failed to create directory for " + rel.string();
            return result;
        }
        std::ofstream out(abs, std::ios::binary);
        if (!out) {
            result.message = "failed to write " + rel.string();
            return result;
        }
        out << seg.content;
        if (!seg.content.empty() && seg.content.back() != '\n') {
            out << '\n';
        }
        SopWrittenFile wf;
        wf.relative_path = rel.string();
        wf.absolute_path = abs.string();
        result.files.push_back(wf);
    }

    result.ok = !result.files.empty();
    result.message = "wrote " + std::to_string(result.files.size()) + " file(s)";
    return result;
}
