/*
 * Copyright (C) 2026 Lenik <soptools@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "engine/project.hpp"
#include "engine/engine.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

#ifndef TEST_SOP_DIR
#define TEST_SOP_DIR "../worldman.sop"
#endif

namespace fs = std::filesystem;

static int failures;

static void expect_true(const char *name, bool ok) {
    if (!ok) {
        fprintf(stderr, "FAIL %s\n", name);
        failures++;
    }
}

static void expect_eq_str(const char *name, const std::string &got, const std::string &want) {
    if (got != want) {
        fprintf(stderr, "FAIL %s: got '%s' want '%s'\n", name, got.c_str(), want.c_str());
        failures++;
    }
}

int main(void) {
    const fs::path tmp = fs::temp_directory_path() / "sopwin_project_test";
    std::error_code ec;
    fs::remove_all(tmp, ec);
    fs::create_directories(tmp, ec);

    const fs::path sop_dir = fs::path(TEST_SOP_DIR);
    if (!fs::is_directory(sop_dir)) {
        fprintf(stderr, "FAIL missing worldman.sop at %s\n", TEST_SOP_DIR);
        return 1;
    }

    SopRuntimeOptions opts;
    opts.project_dir = tmp.string();
    opts.sop_dir = fs::absolute(sop_dir).string();
    opts.gui = false;

    SopEngine engine(opts);
    expect_true("load sop", engine.load_sop());
    if (failures > 0) {
        fs::remove_all(tmp, ec);
        return 1;
    }

    expect_eq_str("config name", sop_config_name(opts.sop_dir), "worldman.sop");
    expect_eq_str("legacy config path suffix",
                  fs::path(sop_config_path(opts.project_dir, opts.sop_dir)).filename().string(),
                  "worldman.sop.conf");
    expect_eq_str("status path", sop_status_path(opts.project_dir),
                  (tmp / "sop" / "status").string());

    if (engine.active_steps().size() > 1) {
        const std::string saved = engine.active_steps()[1];
        engine.set_current_step_id(saved);
        expect_true("status auto-saved", fs::exists(sop_status_path(opts.project_dir)));
        {
            std::ifstream in(sop_status_path(opts.project_dir));
            std::string line;
            bool found_location = false;
            while (std::getline(in, line)) {
                if (line.rfind("location=", 0) == 0) {
                    expect_eq_str("status location line", line, "location=" + saved);
                    found_location = true;
                }
            }
            expect_true("status has location", found_location);
        }
        /* Move away without writing so Revert can restore from sop/status. */
        engine.begin_status_load();
        engine.set_current_step_id(engine.active_steps().front());
        expect_true("revert config", revert_project_config(engine));
        expect_eq_str("location restored", engine.current_step_id(), saved);
    } else {
        expect_true("save config", save_project_config(engine));
        expect_true("revert config", revert_project_config(engine));
    }

    expect_true("status exists", fs::exists(sop_status_path(opts.project_dir)));
    {
        /* Re-open project: load_sop resumes location from sop/status. */
        const std::string want = engine.current_step_id();
        SopEngine engine2(opts);
        expect_true("reload sop", engine2.load_sop());
        expect_eq_str("resumed location", engine2.current_step_id(), want);
    }

    fs::remove_all(tmp, ec);
    return failures == 0 ? 0 : 1;
}
