/*
 * Copyright (C) 2026 Lenik <soptools@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "sop_model.hpp"

#include <cstdio>
#include <filesystem>
#include <string>

#ifndef TEST_SOP_DIR
#define TEST_SOP_DIR "../figma.sop"
#endif

namespace fs = std::filesystem;

static int failures;

static void expect_true(const char *name, bool ok) {
    if (!ok) {
        fprintf(stderr, "FAIL %s\n", name);
        failures++;
    }
}

static void expect_eq_int(const char *name, int got, int want) {
    if (got != want) {
        fprintf(stderr, "FAIL %s: got %d want %d\n", name, got, want);
        failures++;
    }
}

int main(void) {
    const fs::path sop_dir = fs::path(TEST_SOP_DIR);
    if (!fs::is_directory(sop_dir)) {
        fprintf(stderr, "FAIL missing figma.sop at %s\n", TEST_SOP_DIR);
        return 1;
    }

    expect_true("humanize create_prd", humanize_name("create_prd") == "Create Prd");

    auto step = parse_sop_file((sop_dir / "000sh.refactor_figma.md").string());
    expect_true("parse shell step", step.has_value());
    if (step) {
        expect_eq_int("seq", step->seq, 0);
        expect_true("step id", sop_step_id(*step) == "000sh");
        expect_true("role sh", step->role == SopRole::Shell);
        expect_true("has shell script", !step->shell_script.empty());
        expect_true("kind shell", step->kind == SopStepKind::Shell);
        expect_true("action node", step->is_action());
        expect_true("not user", !step->is_user());
        expect_true("shell not prompt copy", step->is_automatable() && !step->is_prompt_copy());
    }

    auto gpt = parse_sop_file((sop_dir / "020gpt.create_prd.md").string());
    expect_true("parse gpt step", gpt.has_value());
    if (gpt) {
        expect_true("kind ai", gpt->kind == SopStepKind::AiOutput);
        expect_true("output path", !gpt->output_paths.empty());
        expect_true("user node", gpt->is_user());
        expect_true("not automatable", !gpt->is_automatable());
        expect_true("gpt prompt copy", gpt->is_prompt_copy());
    }

    auto codex = parse_sop_file((sop_dir / "010codex.refactor_web.md").string());
    expect_true("parse codex step", codex.has_value());
    if (codex) {
        expect_true("codex user", codex->is_user());
        expect_true("codex not automatable", !codex->is_automatable());
        expect_true("codex prompt copy", codex->is_prompt_copy());
    }

    SopDefinition def = load_sop_directory(sop_dir.string());
    expect_true("load sop dir", !def.steps.empty());
    expect_true("branch at 020", def.branch_groups[20].step_ids.size() >= 2);

    auto order = build_active_step_order(def, {});
    expect_true("active order", !order.empty());

    return failures == 0 ? 0 : 1;
}
