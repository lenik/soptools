/*
 * Copyright (C) 2026 Lenik <soptools@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "gui/display_util.hpp"

#include <cstdio>
#include <wx/display.h>
#include <wx/utils.h>

void centre_on_same_display(wxWindow *win, wxWindow *ref) {
    if (!win) {
        return;
    }
    int idx = wxNOT_FOUND;
    if (ref) {
        idx = wxDisplay::GetFromWindow(ref);
    }
    if (idx == wxNOT_FOUND) {
        idx = wxDisplay::GetFromPoint(wxGetMousePosition());
    }
    if (idx == wxNOT_FOUND) {
        idx = 0;
    }
    wxDisplay display(static_cast<unsigned>(idx));
    const wxRect area = display.GetClientArea();
    const wxSize sz = win->GetSize();
    int x = area.x + (area.width - sz.x) / 2;
    int y = area.y + (area.height - sz.y) / 2;
    if (x < area.x) {
        x = area.x;
    }
    if (y < area.y) {
        y = area.y;
    }
    win->SetPosition(wxPoint(x, y));
}

std::string encode_display_geometry(wxWindow *ref) {
    int idx = wxNOT_FOUND;
    if (ref) {
        idx = wxDisplay::GetFromWindow(ref);
    }
    if (idx == wxNOT_FOUND) {
        idx = wxDisplay::GetFromPoint(wxGetMousePosition());
    }
    if (idx == wxNOT_FOUND) {
        idx = 0;
    }
    wxDisplay display(static_cast<unsigned>(idx));
    const wxRect area = display.GetClientArea();
    char buf[128];
    std::snprintf(buf, sizeof(buf), "%d,%d,%d,%d", area.x, area.y, area.width, area.height);
    return buf;
}

void centre_on_display_geometry(wxWindow *win, const std::string &geometry) {
    if (!win || geometry.empty()) {
        centre_on_same_display(win, nullptr);
        return;
    }
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    if (std::sscanf(geometry.c_str(), "%d,%d,%d,%d", &x, &y, &w, &h) != 4 || w <= 0 || h <= 0) {
        centre_on_same_display(win, nullptr);
        return;
    }
    const wxSize sz = win->GetSize();
    int px = x + (w - sz.x) / 2;
    int py = y + (h - sz.y) / 2;
    if (px < x) {
        px = x;
    }
    if (py < y) {
        py = y;
    }
    win->SetPosition(wxPoint(px, py));
}
