/*
 * Copyright (C) 2026 Lenik <soptools@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#define _POSIX_C_SOURCE 200809L

#include "config.h"
#include "ui/console.hpp"
#include "gui/gui.hpp"
#include "util/paths.hpp"
#include "engine/engine.hpp"

#include <cstdlib>
#include <cstring>
#include <getopt.h>
#include <stdio.h>
#include <string>
#include <unistd.h>
#include <bas/locale/i18n.h>
#include <bas/log/deflog.h>

extern "C" {
#include <bas/proc/env.h>
}

enum {
    OPT_VERSION = 256,
    OPT_SOP_DIR = 257,
    OPT_CONSOLE = 258,
    OPT_GUI = 259,
    OPT_CHDIR = 260,
    OPT_SUITE = 261,
    OPT_STAGE_DIR = 262,
};

enum class SopCmd {
    Open,
    Run,
    CopyDlg,
    Edit,
    Version,
};

define_logger();

static int verbose;

static void usage(FILE *out) {
    fputs("Usage: sopwin [OPTIONS] [CMD=open] ARGS...\n"
          "Guide project construction through SOP workflow steps.\n\n"
          "Commands:\n"
          "  open [PROJECTDIR]   Show main window and open the project (default)\n"
          "  run PROJECTDIR      Run SOP in console mode\n"
          "  copydlg             Clipboard & dialog helper (agent UI)\n"
          "  edit SOPDIR         Show SOP editor\n"
          "  version             Print version and exit\n\n"
          "Global options:\n"
          "  -s, --suite NAME     SOP suite under suite/ (default: worldman)\n"
          "  -S, --sop-dir DIR    SOP directory (overrides --suite / LANG branch)\n"
          "      --stage-dir DIR  watch DIR for incoming files (also SOP_STAGE_DIR)\n"
          "  -c, --console        alias for: run (deprecated)\n"
          "  -g, --gui            alias for: open (deprecated)\n"
          "  -C, --chdir DIR      project directory (default: cwd or nearest .git)\n"
          "  -v, --verbose        repeat for more verbose loggings\n"
          "  -q, --quiet          show less logging messages\n"
          "  -h, --help           display this help and exit\n"
          "      --version        output version information and exit\n\n"
          "Suite branch (default / zh_CN / ja) is chosen from LANG / LC_ALL /\n"
          "LC_MESSAGES unless --sop-dir is set.\n\n"
          "copydlg options (after 'copydlg'):\n"
          "  -t, --title TEXT     window title\n"
          "  -s, --status TEXT    status bar text\n"
          "      --paste          paste-response mode (chat)\n"
          "      --confirm        confirm-only mode (code)\n"
          "      --stage-dir DIR  stagedir for expected files\n"
          "      --expected FILE  expected project-relative path (repeatable)\n\n",
          out);
    fprintf(out, "Report bugs to: <%s>\n", PROJECT_EMAIL);
}

static void print_version() {
    printf("sopwin %s\n", PROJECT_VERSION);
    printf("Copyright (C) %d %s\n", PROJECT_YEAR, PROJECT_AUTHOR);
    fputs("License AGPL-3.0-or-later: <https://www.gnu.org/licenses/agpl-3.0.html>\n"
          "This is free software: you are free to change and redistribute it.\n"
          "This project opposes AI exploitation and AI hegemony.\n"
          "This project rejects mindless MIT-style licensing and politically naive "
          "BSD-style licensing.\n"
          "There is NO WARRANTY, to the extent permitted by law.\n",
          stdout);
}

static bool has_display() {
    const char *display = getenv("DISPLAY");
    return display && display[0] != '\0';
}

static bool is_cmd_token(const char *s) {
    return s && (!strcmp(s, "open") || !strcmp(s, "run") || !strcmp(s, "copydlg") ||
                 !strcmp(s, "edit") || !strcmp(s, "version"));
}

static SopCmd parse_cmd(const char *s) {
    if (!strcmp(s, "run")) {
        return SopCmd::Run;
    }
    if (!strcmp(s, "copydlg")) {
        return SopCmd::CopyDlg;
    }
    if (!strcmp(s, "edit")) {
        return SopCmd::Edit;
    }
    if (!strcmp(s, "version")) {
        return SopCmd::Version;
    }
    return SopCmd::Open;
}

int main(int argc, char **argv) {
    const char *exe = self_exe();
    (void)exe;
    init_i18n(LOCALEDIR);
    std::string sop_dir;
    std::string suite = "worldman";
    std::string chdir_dir;
    std::string project_arg;
    std::string stage_dir_cli;
    bool force_console = false;
    bool force_gui = false;
    SopCmd cmd = SopCmd::Open;

    static const struct option long_opts[] = {
        {"suite", required_argument, NULL, 's'},
        {"sop-dir", required_argument, NULL, 'S'},
        {"sop", required_argument, NULL, 'S'}, /* deprecated alias */
        {"stage-dir", required_argument, NULL, OPT_STAGE_DIR},
        {"console", no_argument, NULL, 'c'},
        {"gui", no_argument, NULL, 'g'},
        {"chdir", required_argument, NULL, 'C'},
        {"verbose", no_argument, NULL, 'v'},
        {"quiet", no_argument, NULL, 'q'},
        {"help", no_argument, NULL, 'h'},
        {"version", no_argument, NULL, OPT_VERSION},
        {NULL, 0, NULL, 0},
    };

    /* Parse only global options; stop before CMD / non-option args. */
    optind = 1;
    for (;;) {
        if (optind < argc && argv[optind] && argv[optind][0] != '-') {
            break;
        }
        int c = getopt_long(argc, argv, "+s:S:cgC:vqh", long_opts, NULL);
        if (c == -1) {
            break;
        }
        switch (c) {
        case 's':
            suite = optarg;
            break;
        case 'S':
            sop_dir = optarg;
            break;
        case 'c':
            force_console = true;
            cmd = SopCmd::Run;
            break;
        case 'g':
            force_gui = true;
            cmd = SopCmd::Open;
            break;
        case 'C':
            chdir_dir = optarg;
            break;
        case 'v':
            verbose++;
            break;
        case 'q':
            verbose = -1;
            break;
        case 'h':
            usage(stdout);
            return 0;
        case OPT_VERSION:
            print_version();
            return 0;
        case OPT_STAGE_DIR:
            stage_dir_cli = optarg;
            break;
        default:
            usage(stderr);
            return 1;
        }
    }

    argc -= optind;
    argv += optind;

    if (argc > 0 && is_cmd_token(argv[0])) {
        cmd = parse_cmd(argv[0]);
        argc--;
        argv++;
    }

    if (cmd == SopCmd::Version) {
        print_version();
        return 0;
    }

    if (cmd == SopCmd::CopyDlg) {
        return run_copydlg_mode(argc, argv);
    }

    if (cmd == SopCmd::Edit) {
        std::string edit_dir;
        if (argc > 0) {
            edit_dir = argv[0];
        } else if (!sop_dir.empty()) {
            edit_dir = sop_dir;
        } else {
            fputs("sopwin edit: SOPDIR required\n", stderr);
            return 1;
        }
        return run_edit_mode(edit_dir, argc, argv);
    }

#ifdef SOURCE_ROOT
    const std::string source_root = SOURCE_ROOT;
#else
    const std::string source_root = ".";
#endif

    SopRuntimeOptions opts;
    opts.verbose = verbose;
    const std::string branch = suite_branch_from_env();
    opts.sop_dir = resolve_sop_dir(sop_dir, suite, branch, source_root);

    if (!stage_dir_cli.empty()) {
        opts.stage_dir = stage_dir_cli;
    } else if (const char *env_stage = getenv("SOP_STAGE_DIR")) {
        if (env_stage[0]) {
            opts.stage_dir = env_stage;
        }
    }

    if (argc > 0) {
        project_arg = argv[0];
    }

    if (!chdir_dir.empty()) {
        opts.project_dir = find_project_dir(chdir_dir);
    } else if (!project_arg.empty()) {
        opts.project_dir = find_project_dir(project_arg);
    } else {
        opts.project_dir = find_project_dir(".");
    }

    if (cmd == SopCmd::Run || force_console) {
        opts.console = true;
        opts.gui = false;
    } else if (force_gui || cmd == SopCmd::Open) {
        opts.console = false;
        opts.gui = has_display();
        if (!opts.gui) {
            opts.console = true;
        }
    } else {
        opts.gui = has_display();
        opts.console = !opts.gui;
    }

    if (cmd == SopCmd::Run && project_arg.empty() && chdir_dir.empty()) {
        fputs("sopwin run: PROJECTDIR required\n", stderr);
        return 1;
    }

    SopEngine engine(opts);

    if (opts.gui && cmd == SopCmd::Open) {
        return run_gui_mode(engine, argc, argv);
    }
    return run_console_mode(engine);
}
