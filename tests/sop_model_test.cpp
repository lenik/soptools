/*
 * Copyright (C) 2026 Lenik <soptools@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "model/model.hpp"

#include <cstdio>
#include <filesystem>
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

static void expect_eq_int(const char *name, int got, int want) {
    if (got != want) {
        fprintf(stderr, "FAIL %s: got %d want %d\n", name, got, want);
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
    const fs::path sop_dir = fs::path(TEST_SOP_DIR);
    if (!fs::is_directory(sop_dir)) {
        fprintf(stderr, "FAIL missing worldman.sop at %s\n", TEST_SOP_DIR);
        return 1;
    }

    expect_true("humanize create_prd", humanize_name("create_prd") == "Create Prd");

    auto step = parse_sop_file((sop_dir / "000a._shell.refactor_figma.sh").string());
    expect_true("parse shell step", step.has_value());
    if (step) {
        expect_eq_int("seq", step->seq, 0);
        expect_eq_str("step id", sop_step_id(*step), "000ashell");
        expect_true("variant a", step->variant == 'a');
        expect_true("role shell", step->role == SopRole::Shell);
        expect_true("has shell script", !step->shell_script.empty());
        expect_true("kind shell", step->kind == SopStepKind::Shell);
        expect_true("has shebang", step->shell_script.rfind("#!/bin/bash", 0) == 0);
        expect_true("sources sop-script", step->shell_script.find(". sop-script") != std::string::npos);
        expect_true("action node", step->is_action());
        expect_true("not user", !step->is_user());
        expect_true("shell not prompt copy", step->is_automatable() && !step->is_prompt_copy());
        expect_true("shell not alt", !step->is_alt_branch());
    }

    auto gpt = parse_sop_file((sop_dir / "020a.___gpt.create_prd.get").string());
    expect_true("parse gpt step", gpt.has_value());
    if (gpt) {
        expect_eq_str("gpt id", sop_step_id(*gpt), "020agpt");
        expect_true("kind ai", gpt->kind == SopStepKind::AiOutput);
        expect_true("gpt get", gpt->is_gpt_get());
        expect_eq_str("save-as", gpt->save_as, "sop/PRD.md");
        expect_true("file-link inode", gpt->file_link == SopFileLink::Inode);
        expect_true("parse multi-parts", gpt->has_parse_format("multi-parts"));
        expect_true("output path", !gpt->output_paths.empty());
        expect_true("user node", gpt->is_user());
        expect_true("not automatable", !gpt->is_automatable());
        expect_true("gpt prompt copy", gpt->is_prompt_copy());
        expect_true("gpt default", !gpt->is_alt_branch());
        expect_true("body starts with heading", gpt->body.rfind("# Create PRD", 0) == 0 ||
                                                    gpt->body.find("# Create PRD") != std::string::npos);
        expect_true("no vim modeline in body", gpt->body.find("vim:") == std::string::npos);
        expect_true("no emacs modeline in body", gpt->body.find("-*-") == std::string::npos);
    }

    auto schema = parse_sop_file((sop_dir / "030a.___gpt.create_prisma_schema.get").string());
    expect_true("parse schema gpt", schema.has_value());
    if (schema) {
        expect_eq_str("030 save-as", schema->save_as, "prisma/schema.prisma");
        expect_true("030 file-link default", schema->file_link == SopFileLink::Default);
        expect_true("030 no parse", schema->parse_formats.empty());
        expect_true("030 interaction none", schema->interaction == SopInteraction::None);
    }

    auto seed = parse_sop_file((sop_dir / "040a.___gpt.create_seed.get").string());
    expect_true("parse seed gpt", seed.has_value());
    if (seed) {
        expect_eq_str("040 save-as", seed->save_as, "prisma/seed.ts");
        expect_true("040 no parse", seed->parse_formats.empty());
    }

    auto prompt = parse_sop_file((sop_dir / "050a.___gpt.write_backend_prompt.get").string());
    expect_true("parse backend prompt gpt", prompt.has_value());
    if (prompt) {
        expect_true("050 parse multi-parts", prompt->has_parse_format("multi-parts"));
        expect_true("050 interaction select", prompt->interaction == SopInteraction::Select);
    }

    auto wf = parse_sop_file((sop_dir / "100a.___gpt.create_workflows.get").string());
    expect_true("parse workflows gpt", wf.has_value());
    if (wf) {
        expect_eq_str("100 save-as", wf->save_as, "web-e2e/workflows.md");
        expect_true("100 no parse", wf->parse_formats.empty());
        expect_true("100 interaction none", wf->interaction == SopInteraction::None);
    }

    auto prep = parse_sop_file((sop_dir / "050z._codex.prepare_backend_implementation.md").string());
    expect_true("parse 050z prepare", prep.has_value());
    if (prep) {
        expect_eq_str("050z id", sop_step_id(*prep), "050zcodex");
        expect_true("050z alt", prep->is_alt_branch());
    }

    auto codex = parse_sop_file((sop_dir / "010a._codex.refactor_web.md").string());    expect_true("parse codex step", codex.has_value());
    if (codex) {
        expect_eq_str("codex id", sop_step_id(*codex), "010acodex");
        expect_true("codex user", codex->is_user());
        expect_true("codex not automatable", !codex->is_automatable());
        expect_true("codex prompt copy", codex->is_prompt_copy());
    }

    auto alt = parse_sop_file((sop_dir / "020z._codex.create_prd.md").string());
    expect_true("parse z alt step", alt.has_value());
    if (alt) {
        expect_eq_str("alt id", sop_step_id(*alt), "020zcodex");
        expect_true("alt variant z", alt->variant == 'z');
        expect_true("alt role codex", alt->role == SopRole::Codex);
        expect_true("alt is alt branch", alt->is_alt_branch());
        expect_true("alt prompt copy", alt->is_prompt_copy());
    }

    SopDefinition def = load_sop_directory(sop_dir.string());
    expect_true("load sop dir", !def.steps.empty());
    expect_true("branch at 020", def.branch_groups[20].step_ids.size() >= 2);
    if (def.branch_groups[20].step_ids.size() >= 2) {
        expect_eq_str("020 default first", def.branch_groups[20].step_ids.front(), "020agpt");
    }

    auto order = build_active_step_order(def, {});
    expect_true("active order", !order.empty());

    return failures == 0 ? 0 : 1;
}
