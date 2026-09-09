/*
 * Copyright (C) 2026 Lenik <soptools@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#define _POSIX_C_SOURCE 200809L

#include "engine/gpt_response.hpp"

#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <regex>
#include <set>
#include <sstream>
#include <sys/statfs.h>
#include <sys/wait.h>
#include <unistd.h>

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

bool write_text_file(const fs::path &abs, const std::string &content, std::string *err) {
    if (!ensure_parent_dirs(abs)) {
        if (err) {
            *err = "failed to create directory for " + abs.string();
        }
        return false;
    }
    std::ofstream out(abs, std::ios::binary);
    if (!out) {
        if (err) {
            *err = "failed to write " + abs.string();
        }
        return false;
    }
    out << content;
    if (!content.empty() && content.back() != '\n') {
        out << '\n';
    }
    return true;
}

std::string ascii_lower(std::string s) {
    for (char &c : s) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return s;
}

bool looks_like_multipart_doc(const std::string &text) {
    const std::string lower = ascii_lower(text);
    if (lower.find("multiple parts") != std::string::npos) {
        return true;
    }
    if (text.find("多部分") != std::string::npos) {
        return true;
    }
    if (text.find("複数パート") != std::string::npos) {
        return true;
    }
    static const std::regex part_re(R"((?:^|\n)#{1,3}\s*Part\s+\d+)", std::regex::icase);
    return std::regex_search(text, part_re);
}

std::string basename_from_url(const std::string &url) {
    std::string u = url;
    const size_t q = u.find('?');
    if (q != std::string::npos) {
        u = u.substr(0, q);
    }
    const size_t hash = u.find('#');
    if (hash != std::string::npos) {
        u = u.substr(0, hash);
    }
    const size_t slash = u.find_last_of('/');
    std::string name = slash == std::string::npos ? u : u.substr(slash + 1);
    if (name.empty()) {
        name = "download.bin";
    }
    for (char &c : name) {
        if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '.' || c == '-' || c == '_')) {
            c = '_';
        }
    }
    return name;
}

std::string unique_name(const fs::path &dir, const std::string &base) {
    fs::path candidate = dir / base;
    if (!fs::exists(candidate)) {
        return base;
    }
    const size_t dot = base.find_last_of('.');
    const std::string stem = dot == std::string::npos ? base : base.substr(0, dot);
    const std::string ext = dot == std::string::npos ? "" : base.substr(dot);
    for (int i = 2; i < 1000; i++) {
        const std::string name = stem + "_" + std::to_string(i) + ext;
        if (!fs::exists(dir / name)) {
            return name;
        }
    }
    return stem + "_x" + ext;
}

#include <sys/statfs.h>
#include <sys/vfs.h>
#ifndef EXT4_SUPER_MAGIC
#define EXT4_SUPER_MAGIC 0xEF53
#endif
#ifndef EXT3_SUPER_MAGIC
#define EXT3_SUPER_MAGIC 0xEF53
#endif
#ifndef EXT2_SUPER_MAGIC
#define EXT2_SUPER_MAGIC 0xEF53
#endif

bool is_ext_filesystem(const fs::path &path) {
    struct statfs st {};
    std::string probe = path.string();
    while (!probe.empty()) {
        if (statfs(probe.c_str(), &st) == 0) {
            return st.f_type == EXT4_SUPER_MAGIC || st.f_type == EXT2_SUPER_MAGIC;
        }
        const auto parent = fs::path(probe).parent_path();
        if (parent == probe || parent.empty()) {
            break;
        }
        probe = parent.string();
    }
    if (statfs("/", &st) == 0) {
        return st.f_type == EXT4_SUPER_MAGIC || st.f_type == EXT2_SUPER_MAGIC;
    }
    return false;
}

SopFileLink resolve_file_link(SopFileLink link, const fs::path &project_dir) {
    if (link != SopFileLink::Default) {
        return link;
    }
    return is_ext_filesystem(project_dir) ? SopFileLink::Inode : SopFileLink::None;
}

std::string numbered_save_as(const std::string &save_as, size_t index1_based, size_t total) {
    if (total <= 1) {
        return save_as;
    }
    const fs::path p(save_as);
    const std::string stem = p.stem().string();
    const std::string ext = p.extension().string();
    const fs::path parent = p.parent_path();
    const std::string name = stem + std::to_string(index1_based) + ext;
    if (parent.empty()) {
        return name;
    }
    return (parent / name).string();
}

bool is_blank_text(const std::string &s) {
    for (unsigned char c : s) {
        if (!std::isspace(c)) {
            return false;
        }
    }
    return true;
}

bool apply_save_as_link(const fs::path &project_dir,
                        const fs::path &source_abs,
                        const std::string &save_as,
                        SopFileLink link,
                        std::string *err) {
    if (save_as.empty()) {
        return true;
    }
    const SopFileLink resolved = resolve_file_link(link, project_dir);
    const fs::path dest = project_dir / save_as;
    if (!ensure_parent_dirs(dest)) {
        if (err) {
            *err = "failed to create directory for " + save_as;
        }
        return false;
    }
    std::error_code ec;
    fs::remove(dest, ec);
    ec.clear();

    if (resolved == SopFileLink::Inode) {
        fs::create_hard_link(source_abs, dest, ec);
        if (!ec) {
            return true;
        }
        ec.clear();
        /* fall through to copy */
    } else if (resolved == SopFileLink::Sym) {
        const fs::path rel = fs::relative(source_abs, dest.parent_path(), ec);
        if (ec) {
            ec.clear();
            fs::create_symlink(source_abs, dest, ec);
        } else {
            fs::create_symlink(rel, dest, ec);
        }
        if (!ec) {
            return true;
        }
        if (err) {
            *err = "symlink failed for " + save_as + ": " + ec.message();
        }
        return false;
    }

    /* none or hard-link fallback: copy */
    fs::copy_file(source_abs, dest, fs::copy_options::overwrite_existing, ec);
    if (ec) {
        if (err) {
            *err = "copy Save-As failed for " + save_as + ": " + ec.message();
        }
        return false;
    }
    return true;
}

} /* namespace */

std::string sop_seq_dir_name(int seq) {
    return std::to_string(seq);
}

std::string sniff_response_extension(const std::string &text) {
    const std::string t = trim(text);
    if (!t.empty() && (t[0] == '{' || t[0] == '[')) {
        return ".json";
    }
    const std::string lower = ascii_lower(t.substr(0, std::min<size_t>(t.size(), 400)));
    if (lower.rfind("#", 0) == 0 || lower.find("\n#") != std::string::npos ||
        lower.find("```") != std::string::npos || lower.find("\n- ") != std::string::npos) {
        return ".md";
    }
    return ".md";
}

std::vector<std::string> extract_download_urls(const std::string &text) {
    std::vector<std::string> urls;
    std::set<std::string> seen;

    const std::regex md_link(R"(\[[^\]]*\]\((https?://[^)\s]+)\))", std::regex::icase);
    for (std::sregex_iterator it(text.begin(), text.end(), md_link), end; it != end; ++it) {
        const std::string u = trim((*it)[1].str());
        if (seen.insert(u).second) {
            urls.push_back(u);
        }
    }

    const std::regex bare(R"((https?://[^\s<>"')\]]+))", std::regex::icase);
    for (std::sregex_iterator it(text.begin(), text.end(), bare), end; it != end; ++it) {
        std::string u = trim((*it)[1].str());
        while (!u.empty() && (u.back() == '.' || u.back() == ',' || u.back() == ';' || u.back() == ')')) {
            u.pop_back();
        }
        if (seen.insert(u).second) {
            urls.push_back(u);
        }
    }
    return urls;
}

std::vector<std::pair<std::string, std::string>> split_multi_parts(const std::string &text) {
    std::vector<std::pair<std::string, std::string>> parts;
    if (!looks_like_multipart_doc(text)) {
        return parts;
    }

    static const std::regex part_header(
        R"((?:^|\n)(#{1,3}\s*Part\s+(\d+)\s*[^\n]*|Part\s+(\d+)\s*:))",
        std::regex::icase);

    std::vector<size_t> positions;
    std::vector<int> numbers;
    for (std::sregex_iterator it(text.begin(), text.end(), part_header), end; it != end; ++it) {
        size_t pos = static_cast<size_t>(it->position());
        if (pos > 0 && text[pos] == '\n') {
            pos += 1;
        }
        const int n = std::stoi(!(*it)[2].str().empty() ? (*it)[2].str() : (*it)[3].str());
        positions.push_back(pos);
        numbers.push_back(n);
    }

    if (positions.size() < 2) {
        return parts;
    }

    for (size_t i = 0; i < positions.size(); i++) {
        const size_t start = positions[i];
        const size_t end = (i + 1 < positions.size()) ? positions[i + 1] : text.size();
        std::string content = trim(text.substr(start, end - start));
        char label[32];
        std::snprintf(label, sizeof(label), "Part %d", numbers[i]);
        parts.emplace_back(label, content);
    }
    return parts;
}

bool download_url_to_file(const std::string &url, const std::string &abs_path, std::string *err) {
    if (!ensure_parent_dirs(abs_path)) {
        if (err) {
            *err = "failed to create parent for download";
        }
        return false;
    }
    const pid_t pid = fork();
    if (pid < 0) {
        if (err) {
            *err = "fork failed";
        }
        return false;
    }
    if (pid == 0) {
        execlp("curl", "curl", "-fsSL", "--connect-timeout", "20", "--max-time", "300", "-o",
               abs_path.c_str(), url.c_str(), static_cast<char *>(nullptr));
        _exit(127);
    }
    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        if (err) {
            *err = "waitpid failed";
        }
        return false;
    }
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        if (err) {
            *err = "curl failed for " + url;
        }
        std::error_code ec;
        fs::remove(abs_path, ec);
        return false;
    }
    return true;
}

SopGptSaveResult save_gpt_response(const std::string &project_dir,
                                   const SopStep &step,
                                   const std::string &response_text,
                                   const std::vector<SopGptAttachment> &downloaded) {
    SopGptSaveResult result;
    result.full_content = response_text;
    result.attachments = downloaded;

    const std::string seq_name = sop_seq_dir_name(step.seq);
    result.seq_dir_rel = (fs::path("sop") / seq_name).string();
    const fs::path seq_dir = fs::path(project_dir) / "sop" / seq_name;

    std::error_code ec;
    fs::create_directories(seq_dir, ec);
    if (ec) {
        result.message = "failed to create " + result.seq_dir_rel;
        return result;
    }

    /* Materialize attachments under sop/<seq>/ first (stable paths for Save-As). */
    std::vector<SopGptAttachment *> ok_atts;
    for (auto &att : result.attachments) {
        if (!att.downloaded || att.absolute_path.empty()) {
            continue;
        }
        fs::path src = att.absolute_path;
        const std::string base = unique_name(
            seq_dir, basename_from_url(att.url.empty() ? src.filename().string() : att.url));
        const fs::path dest = seq_dir / base;
        if (fs::absolute(src) != fs::absolute(dest)) {
            ec.clear();
            fs::copy_file(src, dest, fs::copy_options::overwrite_existing, ec);
            if (ec) {
                att.error = ec.message();
                continue;
            }
            att.absolute_path = dest.string();
        }
        att.relative_path = (fs::path(result.seq_dir_rel) / base).string();
        result.files.push_back({att.relative_path, att.absolute_path});
        ok_atts.push_back(&att);
    }

    const bool response_blank = is_blank_text(response_text);
    std::string err;

    if (!response_blank) {
        const std::string ext = sniff_response_extension(response_text);
        const std::string response_name = "response" + ext;
        result.response_rel = (fs::path(result.seq_dir_rel) / response_name).string();
        result.response_abs = (seq_dir / response_name).string();

        if (!write_text_file(result.response_abs, response_text, &err)) {
            result.message = err;
            return result;
        }
        result.files.push_back({result.response_rel, result.response_abs});

        SopGptPart full;
        full.label = "Full";
        full.relative_path = result.response_rel;
        full.absolute_path = result.response_abs;
        full.content = response_text;
        result.parts.push_back(full);

        if (step.has_parse_format("multi-parts") || step.has_parse_format("multipart") ||
            step.has_parse_format("multi_parts")) {
            auto split = split_multi_parts(response_text);
            for (size_t i = 0; i < split.size(); i++) {
                char fname[32];
                std::snprintf(fname, sizeof(fname), "part%02zu.md", i + 1);
                const fs::path part_abs = seq_dir / fname;
                if (!write_text_file(part_abs, split[i].second, &err)) {
                    result.message = err;
                    return result;
                }
                SopGptPart part;
                part.label = split[i].first;
                part.relative_path = (fs::path(result.seq_dir_rel) / fname).string();
                part.absolute_path = part_abs.string();
                part.content = split[i].second;
                result.parts.push_back(part);
                result.files.push_back({part.relative_path, part.absolute_path});
            }
        }

        if (!step.save_as.empty()) {
            if (!apply_save_as_link(project_dir, result.response_abs, step.save_as, step.file_link,
                                    &err)) {
                result.message = err;
                return result;
            }
            const fs::path save_abs = fs::path(project_dir) / step.save_as;
            result.files.push_back({step.save_as, save_abs.string()});
        }
    } else {
        result.response_rel = (fs::path(result.seq_dir_rel) / "response.md").string();
        result.response_abs = (seq_dir / "response.md").string();
        if (!write_text_file(result.response_abs, "", &err)) {
            result.message = err;
            return result;
        }
        result.files.push_back({result.response_rel, result.response_abs});
        SopGptPart full;
        full.label = "Full";
        full.relative_path = result.response_rel;
        full.absolute_path = result.response_abs;
        result.parts.push_back(full);

        if (!step.save_as.empty()) {
            if (ok_atts.empty()) {
                result.message = "response is empty and no downloaded attachments for Save-As";
                return result;
            }
            const size_t total = ok_atts.size();
            for (size_t i = 0; i < total; i++) {
                const std::string dest_rel = numbered_save_as(step.save_as, i + 1, total);
                if (!apply_save_as_link(project_dir, ok_atts[i]->absolute_path, dest_rel,
                                        step.file_link, &err)) {
                    result.message = err;
                    return result;
                }
                const fs::path save_abs = fs::path(project_dir) / dest_rel;
                result.files.push_back({dest_rel, save_abs.string()});
            }
        } else if (ok_atts.empty()) {
            result.message = "response is empty";
            return result;
        }
    }

    result.ok = true;
    result.message =
        "saved " + std::to_string(result.files.size()) + " file(s) under " + result.seq_dir_rel;
    return result;
}
