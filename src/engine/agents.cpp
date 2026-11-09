/*
 * Copyright (C) 2026 Lenik <soptools@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#define _POSIX_C_SOURCE 200809L

#include "engine/agents.hpp"
#include "engine/gpt_response.hpp"

#include "config.h"

#include <array>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

namespace fs = std::filesystem;

namespace {

std::string trim(std::string s) {
    while (!s.empty() && (s.back() == '\r' || s.back() == '\n' || s.back() == ' ' || s.back() == '\t')) {
        s.pop_back();
    }
    size_t i = 0;
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) {
        i++;
    }
    if (i) {
        s.erase(0, i);
    }
    return s;
}

std::string self_sopwin() {
#ifdef SOURCE_ROOT
    /* Prefer PATH-installed sopwin; fall back to sibling of this process if set. */
#endif
    const char *env = getenv("SOPWIN");
    if (env && env[0]) {
        return env;
    }
    return "sopwin";
}

} /* namespace */

std::string default_agent_cmdline(const std::string &role_slug) {
    const std::string bin = self_sopwin();
    if (role_slug == "chat" || role_slug == "gpt") {
        return bin + " sopdlg --paste -s \"Prompt has been copied to the clipboard\"";
    }
    if (role_slug == "code" || role_slug == "codex") {
        return bin + " sopdlg --confirm -t \"Code\" -s \"Prompt copied — forward to Code\"";
    }
    if (role_slug == "shell" || role_slug == "sh") {
        return "/bin/bash";
    }
    return {};
}

std::string agent_cmdline_for_role(const SopAgents &agents, const std::string &role_slug) {
    auto it = agents.by_role.find(role_slug);
    if (it != agents.by_role.end() && !it->second.empty()) {
        return it->second;
    }
    /* Legacy aliases in AGENTS file. */
    if (role_slug == "chat") {
        it = agents.by_role.find("gpt");
        if (it != agents.by_role.end() && !it->second.empty()) {
            return it->second;
        }
    }
    if (role_slug == "code") {
        it = agents.by_role.find("codex");
        if (it != agents.by_role.end() && !it->second.empty()) {
            return it->second;
        }
    }
    return default_agent_cmdline(role_slug);
}

SopAgents load_agents(const std::string &project_dir) {
    SopAgents out;
    const fs::path path = fs::path(project_dir) / "sop" / "AGENTS";
    std::ifstream in(path);
    if (!in) {
        return out;
    }
    std::string line;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }
        const size_t colon = line.find(':');
        if (colon == std::string::npos) {
            continue;
        }
        std::string role = trim(line.substr(0, colon));
        std::string cmd = trim(line.substr(colon + 1));
        if (!role.empty() && !cmd.empty()) {
            out.by_role[role] = cmd;
        }
    }
    return out;
}

SopAgentRunResult run_agent_blocking(const std::string &project_dir,
                                     const std::string &sop_dir,
                                     const SopStep &step,
                                     const std::string &cmdline,
                                     const std::string &prompt_text,
                                     int loglevel,
                                     void *progress_user,
                                     SopAgentProgressFn progress_cb,
                                     std::string *log_out) {
    SopAgentRunResult result;
    if (cmdline.empty()) {
        result.message = "no agent cmdline for role " + step.role_slug;
        return result;
    }

    const std::string seq_name = sop_seq_dir_name(step.seq);
    const fs::path work = fs::path(project_dir) / "sop" / seq_name;
    std::error_code ec;
    fs::create_directories(work, ec);

    const fs::path prompt_path = work / "prompt.txt";
    const fs::path response_path = work / "response.out";
    const fs::path prog_path = work / "agent.prog";
    const fs::path log_path = work / "agent.log";
    const fs::path env_update = work / "env.update";

    {
        std::ofstream out(prompt_path, std::ios::binary | std::ios::trunc);
        if (!out) {
            result.message = "failed to write prompt file";
            return result;
        }
        out << prompt_text;
    }
    fs::remove(response_path, ec);
    fs::remove(prog_path, ec);
    fs::remove(log_path, ec);

    result.prompt_path = prompt_path.string();
    result.response_path = response_path.string();

    const pid_t pid = fork();
    if (pid < 0) {
        result.message = "fork failed";
        return result;
    }
    if (pid == 0) {
        const int in_fd = open(prompt_path.c_str(), O_RDONLY);
        const int out_fd = open(response_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
        if (in_fd < 0 || out_fd < 0) {
            _exit(127);
        }
        dup2(in_fd, STDIN_FILENO);
        dup2(out_fd, STDOUT_FILENO);
        close(in_fd);
        close(out_fd);

        setenv("PROGFILE", prog_path.c_str(), 1);
        setenv("LOGFILE", log_path.c_str(), 1);
        char lvl[32];
        std::snprintf(lvl, sizeof(lvl), "%d", loglevel);
        setenv("LOGLEVEL", lvl, 1);
        setenv("KEEP_ALIVE", "0", 1);
        setenv("ENV_UPDATE", env_update.c_str(), 1);
        setenv("SOP_PROJECT_DIR", project_dir.c_str(), 1);
        setenv("SOP_DIR", sop_dir.c_str(), 1);
        setenv("SOP_STEP_ID", sop_step_id(step).c_str(), 1);
        setenv("PROMPT_FILE", prompt_path.c_str(), 1);
        setenv("RESPONSE_FILE", response_path.c_str(), 1);
        if (!step.save_as.empty()) {
            setenv("SOP_SAVE_AS", step.save_as.c_str(), 1);
        } else {
            unsetenv("SOP_SAVE_AS");
        }
        if (!step.parse_formats.empty()) {
            std::ostringstream ps;
            for (size_t i = 0; i < step.parse_formats.size(); i++) {
                if (i) {
                    ps << ',';
                }
                ps << step.parse_formats[i];
            }
            setenv("SOP_PARSE", ps.str().c_str(), 1);
        } else {
            unsetenv("SOP_PARSE");
        }
        {
            const char *fl = "default";
            if (step.file_link == SopFileLink::Inode) {
                fl = "inode";
            } else if (step.file_link == SopFileLink::Sym) {
                fl = "sym";
            } else if (step.file_link == SopFileLink::None) {
                fl = "none";
            }
            setenv("SOP_FILE_LINK", fl, 1);
        }
        if (!step.append_tos.empty()) {
            std::ostringstream ap;
            for (const auto &r : step.append_tos) {
                ap << r.dest << '|' << r.pattern << '|' << (r.delete_src ? '1' : '0') << '\n';
            }
            setenv("SOP_APPEND_TO", ap.str().c_str(), 1);
        } else {
            unsetenv("SOP_APPEND_TO");
        }
        if (chdir(project_dir.c_str()) != 0) {
            /* continue anyway */
        }
        execl("/bin/sh", "sh", "-c", cmdline.c_str(), static_cast<char *>(nullptr));
        _exit(127);
    }

    /* Parent: wait and poll progress. */
    int status = 0;
    for (;;) {
        const pid_t w = waitpid(pid, &status, WNOHANG);
        if (w == pid) {
            break;
        }
        if (w < 0 && errno != EINTR) {
            result.message = "waitpid failed";
            return result;
        }
        /* Progress: "total current" */
        std::ifstream pin(prog_path);
        if (pin) {
            double total = 0;
            double current = 0;
            if (pin >> total >> current && total > 0) {
                double pct = 100.0 * current / total;
                if (pct < 0) {
                    pct = 0;
                }
                if (pct > 100) {
                    pct = 100;
                }
                char label[32];
                std::snprintf(label, sizeof(label), "%.0f%%", pct);
                if (progress_cb) {
                    progress_cb(progress_user, pct, label);
                }
            }
        }
        if (log_out) {
            std::ifstream lin(log_path);
            if (lin) {
                std::ostringstream ss;
                ss << lin.rdbuf();
                *log_out = ss.str();
            }
        }
        usleep(100000);
    }

    if (WIFEXITED(status)) {
        result.exit_code = WEXITSTATUS(status);
    } else {
        result.exit_code = -1;
    }
    result.ok = result.exit_code == 0;

    {
        std::ifstream rin(response_path, std::ios::binary);
        if (rin) {
            std::ostringstream ss;
            ss << rin.rdbuf();
            result.response_text = ss.str();
        }
    }
    if (log_out) {
        std::ifstream lin(log_path);
        if (lin) {
            std::ostringstream ss;
            ss << lin.rdbuf();
            *log_out = ss.str();
        }
    }
    result.message = result.ok ? "agent finished" : ("agent failed with exit " + std::to_string(result.exit_code));
    return result;
}
