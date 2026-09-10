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
#include <getopt.h>
#include <stdio.h>
#include <string.h>
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
};


define_logger();

static int verbose;

static void usage(FILE *out) {
    fputs("Usage: sopwin [OPTIONS] [PROJECTDIR]\n"
          "Guide project construction through SOP workflow steps.\n\n"
          "  -s, --suite NAME     SOP suite under suite/ (default: worldman)\n"
          "  -S, --sop-dir DIR    SOP directory (overrides --suite / LANG branch)\n"
          "  -c, --console        console-only mode (no GUI)\n"
          "  -g, --gui            force GUI mode\n"
          "  -C, --chdir DIR      project directory (default: cwd or nearest .git)\n"
          "  -v, --verbose        repeat for more verbose loggings\n"
          "  -q, --quiet          show less logging messages\n"
          "  -h, --help           display this help and exit\n"
          "      --version        output version information and exit\n\n"
          "Suite branch (default / zh_CN / ja) is chosen from LANG / LC_ALL /\n"
          "LC_MESSAGES unless --sop-dir is set.\n\n",
          out);
    fprintf(out, "Report bugs to: <%s>\n", PROJECT_EMAIL);
}

static bool has_display() {
    const char *display = getenv("DISPLAY");
    return display && display[0] != '\0';
}

int main(int argc, char **argv) {
    const char *exe = self_exe();
    (void)exe;
    init_i18n(LOCALEDIR);
    std::string sop_dir;
    std::string suite = "worldman";
    std::string chdir_dir;
    std::string project_arg;
    bool force_console = false;
    bool force_gui = false;

    static const struct option long_opts[] = {
        {"suite", required_argument, NULL, 's'},
        {"sop-dir", required_argument, NULL, 'S'},
        {"sop", required_argument, NULL, 'S'}, /* deprecated alias */
        {"console", no_argument, NULL, 'c'},
        {"gui", no_argument, NULL, 'g'},
        {"chdir", required_argument, NULL, 'C'},
        {"verbose", no_argument, NULL, 'v'},
        {"quiet", no_argument, NULL, 'q'},
        {"help", no_argument, NULL, 'h'},
        {"version", no_argument, NULL, OPT_VERSION},
        {NULL, 0, NULL, 0},
    };

    for (;;) {
        int c = getopt_long(argc, argv, "s:S:cgC:vqh", long_opts, NULL);
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
            break;
        case 'g':
            force_gui = true;
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
            printf("sopwin %s\n", PROJECT_VERSION);
            printf("Copyright (C) %d %s\n", PROJECT_YEAR, PROJECT_AUTHOR);
            fputs("License AGPL-3.0-or-later: <https://www.gnu.org/licenses/agpl-3.0.html>\n"
                  "This is free software: you are free to change and redistribute it.\n"
                  "This project opposes AI exploitation and AI hegemony.\n"
                  "This project rejects mindless MIT-style licensing and politically naive "
                  "BSD-style licensing.\n"
                  "There is NO WARRANTY, to the extent permitted by law.\n",
                  stdout);
            return 0;
        default:
            usage(stderr);
            return 1;
        }
    }

    argc -= optind;
    argv += optind;
    if (argc > 0) {
        project_arg = argv[0];
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

    if (!chdir_dir.empty()) {
        opts.project_dir = find_project_dir(chdir_dir);
    } else if (!project_arg.empty()) {
        opts.project_dir = find_project_dir(project_arg);
    } else {
        opts.project_dir = find_project_dir(".");
    }

    if (force_console) {
        opts.console = true;
        opts.gui = false;
    } else if (force_gui) {
        opts.console = false;
        opts.gui = true;
    } else {
        opts.gui = has_display();
        opts.console = !opts.gui;
    }

    SopEngine engine(opts);

    if (opts.gui) {
        return run_gui_mode(engine, argc, argv);
    }
    return run_console_mode(engine);
}
