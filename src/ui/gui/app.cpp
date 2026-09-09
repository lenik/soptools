/*
 * Copyright (C) 2026 Lenik <soptools@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "ui/gui.hpp"
#include "ui/gui/main_frame.hpp"

#include <wx/wx.h>

namespace {

SopEngine *g_sop_engine = nullptr;

class SoptoolsApp : public wxApp {
public:
    bool OnInit() override {
        if (!wxApp::OnInit() || !g_sop_engine) {
            return false;
        }
        wxFrame *frame = create_main_frame(g_sop_engine);
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP_NO_MAIN(SoptoolsApp);

int run_gui_mode_inner(SopEngine &engine, int argc, char **argv) {
    g_sop_engine = &engine;
    return wxEntry(argc, argv);
}

} /* namespace */

int run_gui_mode(SopEngine &engine, int argc, char **argv) {
    return run_gui_mode_inner(engine, argc, argv);
}
