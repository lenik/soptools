/*
 * Copyright (C) 2026 Lenik <soptools@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#define _POSIX_C_SOURCE 200809L

#include "sop_console.hpp"

#include <iostream>
#include <limits>
#include <string>

static const char *status_name(SopStepStatus s) {
    switch (s) {
    case SopStepStatus::Pending:
        return "pending";
    case SopStepStatus::Running:
        return "running";
    case SopStepStatus::Waiting:
        return "waiting";
    case SopStepStatus::Complete:
        return "complete";
    case SopStepStatus::Error:
        return "error";
    case SopStepStatus::User:
        return "user";
    }
    return "unknown";
}

static void print_step(const SopStep &step, size_t index, size_t current) {
    const char *st = step.is_user() ? "user" : status_name(step.status);
    std::cout << (index == current ? "=> " : "   ") << step.seq << " [" << step.role_slug << "] "
              << step.display_title() << " (" << st << ")\n";
}

int run_console_mode(SopEngine &engine) {
    engine.set_log_fn([&](int level, const std::string &msg) {
        if (engine.options().verbose >= level) {
            std::cerr << "[log] " << msg << '\n';
        }
    });

    if (!engine.load_sop()) {
        return 1;
    }

    std::cout << "SOP console mode\n";
    std::cout << "Project: " << engine.options().project_dir << '\n';
    std::cout << "SOP dir: " << engine.options().sop_dir << '\n';
    std::cout << "Commands: n/next, p/prev, s/status, r/run, a/apply, b/branch, start, pause, q/quit\n";

    std::string ai_buffer;
    for (;;) {
        engine.poll_completion();
        engine.tick_auto_run();
        const SopStep *step = engine.step_at(engine.current_index());
        if (!step) {
            std::cout << "No active step.\n";
            return 1;
        }

        std::cout << "\n--- Step " << (engine.current_index() + 1) << "/"
                  << engine.active_steps().size() << " ---\n";
        std::cout << step->role_label() << ": " << step->display_title() << '\n';
        if (!step->description.empty()) {
            std::cout << step->description << '\n';
        }
        std::cout << "Status: " << (step->is_user() ? "user" : status_name(step->status))
                  << " (" << step->status_message << ")\n";
        std::cout << step->body << '\n';

        std::cout << "> ";
        std::string cmd;
        if (!std::getline(std::cin, cmd)) {
            break;
        }
        if (cmd.empty()) {
            cmd = "n";
        }
        if (cmd == "q" || cmd == "quit") {
            break;
        }
        if (cmd == "n" || cmd == "next") {
            if (!engine.try_advance_with_run()) {
                std::cout << "Cannot advance.\n";
            }
            continue;
        }
        if (cmd == "p" || cmd == "prev") {
            engine.retreat();
            continue;
        }
        if (cmd == "s" || cmd == "status") {
            for (size_t i = 0; i < engine.active_steps().size(); i++) {
                const SopStep *st = engine.step_at(i);
                if (st) {
                    print_step(*st, i, engine.current_index());
                }
            }
            continue;
        }
        if ((cmd == "r" || cmd == "run") && step->kind == SopStepKind::Shell) {
            engine.run_shell(engine.active_steps()[engine.current_index()]);
            continue;
        }
        if (cmd == "a" || cmd == "apply") {
            std::cout << "Paste AI output, end with a line containing only EOF\n";
            ai_buffer.clear();
            std::string line;
            while (std::getline(std::cin, line)) {
                if (line == "EOF") {
                    break;
                }
                ai_buffer += line;
                ai_buffer += '\n';
            }
            auto result = engine.apply_ai_output(engine.active_steps()[engine.current_index()], ai_buffer);
            std::cout << (result.ok ? "OK: " : "ERR: ") << result.message << '\n';
            continue;
        }
        if (cmd == "start" || cmd == "resume") {
            engine.start_auto_run();
            continue;
        }
        if (cmd == "pause") {
            engine.pause_auto_run();
            continue;
        }
        if (cmd == "b" || cmd == "branch") {
            const auto &def = engine.definition();
            const auto git = def.branch_groups.find(step->seq);
            if (git == def.branch_groups.end() || git->second.step_ids.size() <= 1) {
                std::cout << "No branches at this step.\n";
                continue;
            }
            for (size_t i = 0; i < git->second.step_ids.size(); i++) {
                const std::string &id = git->second.step_ids[i];
                std::cout << i + 1 << ") " << id << " " << def.steps.at(id).display_title() << '\n';
            }
            std::cout << "Choose branch number: ";
            size_t choice = 0;
            if (!(std::cin >> choice)) {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                continue;
            }
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            if (choice == 0 || choice > git->second.step_ids.size()) {
                continue;
            }
            engine.activate_branch(step->seq, git->second.step_ids[choice - 1]);
            continue;
        }
        std::cout << "Unknown command.\n";
    }
    return 0;
}
