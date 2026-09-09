/*
 * Copyright (C) 2026 Lenik <soptools@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "engine/project.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>

namespace fs = std::filesystem;

std::string sop_config_name(const std::string &sop_dir) {
    const fs::path name = fs::path(sop_dir).filename();
    return name.empty() ? "sop" : name.string();
}

std::string sop_config_dir(const std::string &project_dir) {
    return (fs::path(project_dir) / ".config" / "sopwin").string();
}

std::string sop_config_path(const std::string &project_dir, const std::string &sop_dir) {
    return (fs::path(sop_config_dir(project_dir)) / (sop_config_name(sop_dir) + ".conf")).string();
}

std::string sop_log_dir(const std::string &project_dir, const std::string &sop_dir) {
    return (fs::path(sop_config_dir(project_dir)) / sop_config_name(sop_dir)).string();
}

std::string sop_step_log_path(const std::string &project_dir, const std::string &sop_dir,
                              const std::string &step_id) {
    return (fs::path(sop_log_dir(project_dir, sop_dir)) / (step_id + ".log")).string();
}

bool ensure_sop_config_dirs(const std::string &project_dir, const std::string &sop_dir) {
    std::error_code ec;
    if (!fs::create_directories(sop_log_dir(project_dir, sop_dir), ec)) {
        return !ec;
    }
    return true;
}

namespace {

bool trim_line(std::string &line) {
    while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' ')) {
        line.pop_back();
    }
    size_t start = 0;
    while (start < line.size() && line[start] == ' ') {
        start++;
    }
    if (start > 0) {
        line.erase(0, start);
    }
    return !line.empty() && line[0] != '#';
}

bool parse_state_key(const std::string &key, std::string &step_id) {
    static const std::string prefix = "state.";
    if (key.rfind(prefix, 0) != 0) {
        return false;
    }
    step_id = key.substr(prefix.size());
    return !step_id.empty();
}

bool apply_config_to_engine(SopEngine &engine, const std::map<std::string, std::string> &values,
                            std::string *error) {
    const SopDefinition &def = engine.definition();
    auto set_error = [&](const std::string &msg) {
        if (error) {
            *error = msg;
        }
        return false;
    };

    for (const auto &kv : values) {
        std::string step_id;
        if (!parse_state_key(kv.first, step_id)) {
            continue;
        }
        if (!def.steps.count(step_id)) {
            continue;
        }
        if (kv.second == "excluded") {
            engine.set_step_excluded(step_id, true);
        } else if (kv.second == "included") {
            engine.set_step_excluded(step_id, false);
        } else if (kv.second == "complete") {
            engine.mark_complete(step_id, true);
        } else if (kv.second == "incomplete") {
            engine.mark_complete(step_id, false);
        } else {
            return set_error("unknown state for " + step_id + ": " + kv.second);
        }
    }

    const auto pos_it = values.find("position");
    if (pos_it != values.end() && !pos_it->second.empty()) {
        if (!def.steps.count(pos_it->second)) {
            return set_error("unknown position id: " + pos_it->second);
        }
        engine.set_current_step_id(pos_it->second);
    }
    return true;
}

bool read_config_file(const std::string &path, std::map<std::string, std::string> &values, std::string *error) {
    std::ifstream in(path);
    if (!in) {
        if (error) {
            *error = "cannot read " + path;
        }
        return false;
    }
    values.clear();
    std::string line;
    while (std::getline(in, line)) {
        if (!trim_line(line)) {
            continue;
        }
        const size_t eq = line.find('=');
        if (eq == std::string::npos) {
            continue;
        }
        std::string key = line.substr(0, eq);
        std::string value = line.substr(eq + 1);
        trim_line(key);
        trim_line(value);
        if (!key.empty()) {
            values[key] = value;
        }
    }
    return true;
}

} /* namespace */

bool save_project_config(const SopEngine &engine, std::string *error) {
    const std::string &project_dir = engine.options().project_dir;
    const std::string &sop_dir = engine.options().sop_dir;
    if (!ensure_sop_config_dirs(project_dir, sop_dir)) {
        if (error) {
            *error = "cannot create config directory";
        }
        return false;
    }

    const std::string path = sop_config_path(project_dir, sop_dir);
    std::ofstream out(path, std::ios::trunc);
    if (!out) {
        if (error) {
            *error = "cannot write " + path;
        }
        return false;
    }

    const std::string current = engine.current_step_id();
    if (!current.empty()) {
        out << "position=" << current << '\n';
    }

    for (const auto &kv : engine.definition().steps) {
        const std::string &id = kv.first;
        if (engine.is_excluded(id)) {
            out << "state." << id << "=excluded\n";
        }
        if (engine.is_complete(id)) {
            out << "state." << id << "=complete\n";
        }
    }
    return true;
}

bool load_project_config(SopEngine &engine, std::string *error) {
    const std::string path = sop_config_path(engine.options().project_dir, engine.options().sop_dir);
    std::error_code ec;
    if (!fs::exists(path, ec)) {
        return true;
    }

    std::map<std::string, std::string> values;
    if (!read_config_file(path, values, error)) {
        return false;
    }
    return apply_config_to_engine(engine, values, error);
}

bool revert_project_config(SopEngine &engine, std::string *error) {
    engine.reset_session_defaults();
    return load_project_config(engine, error);
}
