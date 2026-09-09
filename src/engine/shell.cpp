/*
 * Copyright (C) 2026 Lenik <soptools@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#define _POSIX_C_SOURCE 200809L

#include "engine/engine.hpp"
#include "engine/project.hpp"
#include "util/paths.hpp"

#include "config.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

namespace fs = std::filesystem;

bool SopEngine::is_shell_running(const std::string &step_id) const {
    std::lock_guard<std::mutex> lock(shell_mu_);
    auto it = shell_runs_.find(step_id);
    return it != shell_runs_.end() && it->second && it->second->running;
}

double SopEngine::shell_progress_pct(const std::string &step_id) const {
    std::lock_guard<std::mutex> lock(shell_mu_);
    auto it = shell_runs_.find(step_id);
    if (it == shell_runs_.end() || !it->second) {
        return -1.0;
    }
    return it->second->progress_pct;
}

std::string SopEngine::shell_progress_label(const std::string &step_id) const {
    std::lock_guard<std::mutex> lock(shell_mu_);
    auto it = shell_runs_.find(step_id);
    if (it == shell_runs_.end() || !it->second) {
        return {};
    }
    return it->second->progress_label;
}

bool SopEngine::run_shell(const std::string &step_id) {
    if (!def_.steps.count(step_id)) {
        return false;
    }
    SopStep &step = def_.steps[step_id];
    if (step.shell_script.empty()) {
        log(0, "no shell script for " + step_id);
        return false;
    }
    if (is_shell_running(step_id)) {
        log(1, "shell already running for " + step_id);
        return false;
    }

    auto run = std::make_shared<SopShellRun>();
    {
        std::lock_guard<std::mutex> lock(shell_mu_);
        shell_runs_[step_id] = run;
    }

    step.status = SopStepStatus::Running;
    step.status_message = "starting shell";
    log(1, "running shell for " + step_id);
    notify_changed();

    const std::string project_dir = opts_.project_dir;
    const std::string sop_dir = opts_.sop_dir;
    const int loglevel = opts_.verbose;
    const std::string script_path = step.filepath;
    const bool run_file = script_path.size() >= 3 &&
                          script_path.compare(script_path.size() - 3, 3, ".sh") == 0;

    std::thread([this, step_id, run, script = step.shell_script, project_dir, sop_dir, loglevel,
                 script_path, run_file]() {
        run->running = true;

#ifdef SOURCE_ROOT
        const std::string source_root = SOURCE_ROOT;
#else
        const std::string source_root;
#endif
        const std::string extension_bash = resolve_extension_bash_dir(source_root);
        const char *old_path = getenv("PATH");
        const std::string path_value =
            extension_bash + ":" + (old_path && old_path[0] ? old_path : "/usr/bin:/bin");

        /* Named pipe for script set_progress <-> sopwin (this script only). */
        char fifo_tmpl[] = "/tmp/sop-progress-XXXXXX";
        const int fifo_dir_fd = mkstemp(fifo_tmpl);
        std::string fifo_path;
        int fifo_hold_fd = -1;
        if (fifo_dir_fd >= 0) {
            close(fifo_dir_fd);
            unlink(fifo_tmpl);
            fifo_path = std::string(fifo_tmpl) + ".fifo";
            if (mkfifo(fifo_path.c_str(), 0600) == 0) {
                /* Keep RDWR open so writers do not block/fail when we are between reads. */
                fifo_hold_fd = open(fifo_path.c_str(), O_RDWR | O_CLOEXEC);
                run->progress_fifo = fifo_path;
            } else {
                fifo_path.clear();
            }
        }

        std::thread progress_reader;
        if (fifo_hold_fd >= 0) {
            progress_reader = std::thread([this, step_id, run, fifo_hold_fd]() {
                std::string pending;
                std::array<char, 256> buf{};
                for (;;) {
                    fd_set rfds;
                    FD_ZERO(&rfds);
                    FD_SET(fifo_hold_fd, &rfds);
                    timeval tv{};
                    tv.tv_sec = 0;
                    tv.tv_usec = 200000; /* 200ms */
                    const int ready = select(fifo_hold_fd + 1, &rfds, nullptr, nullptr, &tv);
                    if (ready > 0 && FD_ISSET(fifo_hold_fd, &rfds)) {
                        const ssize_t n = read(fifo_hold_fd, buf.data(), buf.size());
                        if (n > 0) {
                            pending.append(buf.data(), static_cast<size_t>(n));
                        } else if (n == 0 && !run->running) {
                            break;
                        } else if (n < 0 && errno != EINTR && errno != EAGAIN) {
                            break;
                        }
                    } else if (ready < 0 && errno != EINTR) {
                        break;
                    }

                    size_t pos;
                    while ((pos = pending.find('\n')) != std::string::npos) {
                        std::string line = pending.substr(0, pos);
                        pending.erase(0, pos + 1);
                        if (line.rfind("progress ", 0) != 0) {
                            continue;
                        }
                        std::string value = line.substr(9);
                        while (!value.empty() &&
                               (value.back() == '%' || value.back() == ' ' || value.back() == '\r')) {
                            value.pop_back();
                        }
                        char *end = nullptr;
                        const double pct = std::strtod(value.c_str(), &end);
                        if (end == value.c_str()) {
                            continue;
                        }
                        double clamped = pct;
                        if (clamped < 0) {
                            clamped = 0;
                        }
                        if (clamped > 100) {
                            clamped = 100;
                        }
                        {
                            std::lock_guard<std::mutex> lock(shell_mu_);
                            run->progress_pct = clamped;
                            char label[32];
                            if (clamped == static_cast<double>(static_cast<int>(clamped))) {
                                std::snprintf(label, sizeof(label), "%d%%", static_cast<int>(clamped));
                            } else {
                                std::snprintf(label, sizeof(label), "%.2f%%", clamped);
                            }
                            run->progress_label = label;
                            auto sit = def_.steps.find(step_id);
                            if (sit != def_.steps.end()) {
                                sit->second.status_message = std::string("progress ") + label;
                            }
                        }
                        notify_changed();
                    }

                    if (!run->running && pending.empty()) {
                        break;
                    }
                }
            });
        }

        std::ostringstream env_prefix;
        env_prefix << "cd " << shell_single_quote(project_dir)
                   << " && PATH=" << shell_single_quote(path_value)
                   << " SOP_PROJECT_DIR=" << shell_single_quote(project_dir)
                   << " SOP_DIR=" << shell_single_quote(sop_dir)
                   << " SOP_STEP_ID=" << shell_single_quote(step_id)
                   << " SOP_LOGLEVEL=" << loglevel << " LOGLEVEL=" << loglevel << " ";
        if (!fifo_path.empty()) {
            env_prefix << "SOP_PROGRESS_FIFO=" << shell_single_quote(fifo_path) << " ";
        }

        std::string cmd;
        if (run_file) {
            cmd = env_prefix.str() + "bash -- " + shell_single_quote(script_path);
            run->log += "$ cd " + project_dir + "\n";
            run->log += "$ # PATH includes " + extension_bash + "\n";
            if (!fifo_path.empty()) {
                run->log += "$ # SOP_PROGRESS_FIFO=" + fifo_path + "\n";
            }
            run->log += "$ bash -- " + script_path + "\n";
        } else {
            std::string wrapped = ". sop-script\n" + script;
            cmd = env_prefix.str() + "bash -lc " + shell_single_quote(wrapped);
            run->log += "$ cd " + project_dir + "\n";
            run->log += "$ # PATH includes " + extension_bash + "\n";
            run->log += script;
            if (!script.empty() && script.back() != '\n') {
                run->log += "\n";
            }
        }

        FILE *pipe = popen(cmd.c_str(), "r");
        if (!pipe) {
            run->log += "failed to start shell\n";
            run->exit_code = 127;
            run->running = false;
            if (progress_reader.joinable()) {
                if (fifo_hold_fd >= 0) {
                    close(fifo_hold_fd);
                    fifo_hold_fd = -1;
                }
                progress_reader.join();
            }
            if (!fifo_path.empty()) {
                unlink(fifo_path.c_str());
            }
            ensure_sop_config_dirs(project_dir, sop_dir);
            const std::string log_path = sop_step_log_path(project_dir, sop_dir, step_id);
            std::ofstream log_out(log_path, std::ios::trunc);
            if (log_out) {
                log_out << run->log;
            }
            notify_changed();
            return;
        }

        std::array<char, 4096> buf{};
        while (fgets(buf.data(), static_cast<int>(buf.size()), pipe) != nullptr) {
            run->log += buf.data();
        }
        run->exit_code = pclose(pipe);
        if (run->exit_code == -1) {
            run->exit_code = 127;
        } else if (WIFEXITED(run->exit_code)) {
            run->exit_code = WEXITSTATUS(run->exit_code);
        } else {
            run->exit_code = 1;
        }
        run->running = false;

        if (fifo_hold_fd >= 0) {
            close(fifo_hold_fd);
            fifo_hold_fd = -1;
        }
        if (progress_reader.joinable()) {
            progress_reader.join();
        }
        if (!fifo_path.empty()) {
            unlink(fifo_path.c_str());
        }

        ensure_sop_config_dirs(project_dir, sop_dir);
        const std::string log_path = sop_step_log_path(project_dir, sop_dir, step_id);
        std::ofstream log_out(log_path, std::ios::trunc);
        if (log_out) {
            log_out << run->log;
        }
        log(1, "shell finished for " + step_id + " exit=" + std::to_string(run->exit_code));
        notify_changed();
    }).detach();

    return true;
}

void SopEngine::stop_shell(const std::string &step_id) {
    (void)step_id;
}

