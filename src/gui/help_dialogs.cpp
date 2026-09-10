/*
 * Copyright (C) 2026 Lenik <soptools@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "gui/help_dialogs.hpp"
#include "gui/theme.hpp"

#include "config.h"

#include <wx/artprov.h>
#include <wx/button.h>
#include <wx/dialog.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/statline.h>
#include <wx/stattext.h>

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {

void TrimAsciiWs(std::string &s) {
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t')) {
        s.pop_back();
    }
    size_t i = 0;
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) {
        ++i;
    }
    if (i > 0) {
        s.erase(0, i);
    }
}

/* Merge hard-wrapped license lines into soft-wrapping paragraphs (blank line
 * keeps a paragraph break). */
std::string SoftWrapLicenseText(const std::string &raw) {
    std::istringstream in(raw);
    std::string line;
    std::string out;
    std::string para;
    auto flush = [&]() {
        if (para.empty()) {
            return;
        }
        if (!out.empty()) {
            out += "\n\n";
        }
        out += para;
        para.clear();
    };
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        TrimAsciiWs(line);
        if (line.empty()) {
            flush();
            continue;
        }
        if (!para.empty()) {
            para += ' ';
        }
        para += line;
    }
    flush();
    return out;
}

} /* namespace */

void show_shortcuts_dialog(wxWindow *parent) {
    wxDialog dlg(parent, wxID_ANY, wxString::FromUTF8("Keyboard Shortcuts"), wxDefaultPosition, wxSize(560, 480));
    auto *root = new wxBoxSizer(wxVERTICAL);

    auto *scroll = new wxScrolledWindow(&dlg, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL | wxBORDER_NONE);
    scroll->SetScrollRate(0, 12);
    scroll->ShowScrollbars(wxSHOW_SB_NEVER, wxSHOW_SB_DEFAULT);
    auto *inner = new wxBoxSizer(wxVERTICAL);
    inner->Add(MakeShortcutSection(
                   scroll, wxString::FromUTF8("File"),
                   {{"Ctrl+O", "Open Project"},
                    {"Ctrl+S", "Save project status (sop/status)"},
                    {"Ctrl+Q", "Quit"}}),
               0, wxEXPAND | wxALL, 12);
    inner->Add(new wxStaticLine(scroll), 0, wxEXPAND | wxLEFT | wxRIGHT, 12);
    inner->Add(MakeShortcutSection(
                   scroll, wxString::FromUTF8("Procedure"),
                   {{"Ctrl+U", "Load SOP"},
                    {"PgUp", "Back"},
                    {"PgDn", "Next"},
                    {"Enter", "Set location to selected graph node"},
                    {"F5", "Run / Resume auto-run"},
                    {"F8", "Pause auto-run"},
                    {"Ctrl+Enter", "Execute / Get current step"}}),
               0, wxEXPAND | wxALL, 12);
    inner->Add(new wxStaticLine(scroll), 0, wxEXPAND | wxLEFT | wxRIGHT, 12);
    inner->Add(MakeShortcutSection(
                   scroll, wxString::FromUTF8("View"),
                   {{"F2", "Toggle Graph"},
                    {"Ctrl+L", "Toggle Loggings"},
                    {"Alt+letter", "Open menu (File, Procedure, View, Help)"},
                    {"Wheel", "Pan graph up / down"},
                    {"Ctrl+Wheel", "Zoom graph in / out"}}),
               0, wxEXPAND | wxALL, 12);
    inner->Add(new wxStaticLine(scroll), 0, wxEXPAND | wxLEFT | wxRIGHT, 12);
    inner->Add(MakeShortcutSection(scroll, wxString::FromUTF8("Help"), {{"F1", "Keyboard Shortcuts"}}), 0,
               wxEXPAND | wxALL, 12);
    scroll->SetSizer(inner);
    root->Add(scroll, 1, wxEXPAND);

    auto *close_btn = new wxButton(&dlg, wxID_OK, wxString::FromUTF8("Close"));
    root->Add(close_btn, 0, wxALIGN_CENTER | wxALL, 12);
    dlg.SetSizer(root);
    dlg.Layout();
    scroll->FitInside();
    scroll->Scroll(0, 0);
    dlg.ShowModal();
}

void show_license_dialog(wxWindow *parent) {
    std::ifstream in("LICENSE");
    if (!in) {
#ifdef SOURCE_ROOT
        in.open(std::string(SOURCE_ROOT) + "/LICENSE");
#endif
    }
    std::ostringstream body;
    body << in.rdbuf();
    const std::string soft = SoftWrapLicenseText(body.str());

    wxDialog dlg(parent, wxID_ANY, wxString::FromUTF8("License"), wxDefaultPosition, wxSize(680, 460));
    auto *root = new wxBoxSizer(wxVERTICAL);

    auto *scroll =
        new wxScrolledWindow(&dlg, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL | wxBORDER_SUNKEN);
    scroll->SetScrollRate(0, 16);
    scroll->ShowScrollbars(wxSHOW_SB_NEVER, wxSHOW_SB_DEFAULT);

    auto *inner = new wxBoxSizer(wxVERTICAL);
    constexpr int kPad = 12;
    struct Para {
        wxString text;
        wxStaticText *label = nullptr;
    };
    std::vector<Para> paras;
    {
        std::istringstream split(soft);
        std::string chunk;
        std::string line;
        while (std::getline(split, line)) {
            if (line.empty()) {
                if (!chunk.empty()) {
                    paras.push_back({wxString::FromUTF8(chunk), nullptr});
                    chunk.clear();
                }
                continue;
            }
            if (!chunk.empty()) {
                chunk += ' ';
            }
            chunk += line;
        }
        if (!chunk.empty()) {
            paras.push_back({wxString::FromUTF8(chunk), nullptr});
        }
    }
    for (auto &p : paras) {
        p.label = new wxStaticText(scroll, wxID_ANY, p.text);
        inner->Add(p.label, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, kPad);
    }
    inner->AddSpacer(kPad);
    scroll->SetSizer(inner);
    root->Add(scroll, 1, wxEXPAND | wxALL, 12);
    root->Add(new wxButton(&dlg, wxID_OK, wxString::FromUTF8("Close")), 0, wxALIGN_CENTER | wxBOTTOM, 12);
    dlg.SetSizer(root);

    auto reflow = [scroll, &paras]() {
        const int wrap_w = std::max(120, scroll->GetClientSize().GetWidth() - kPad * 2);
        for (auto &p : paras) {
            if (!p.label) {
                continue;
            }
            p.label->SetLabel(p.text);
            p.label->Wrap(wrap_w);
        }
        scroll->FitInside();
    };
    scroll->Bind(wxEVT_SIZE, [reflow](wxSizeEvent &evt) {
        evt.Skip();
        reflow();
    });
    dlg.Layout();
    reflow();
    scroll->Scroll(0, 0);
    dlg.ShowModal();
}

void show_about_dialog(wxWindow *parent) {
    wxDialog dlg(parent, wxID_ANY, wxString::FromUTF8("About sopwin"), wxDefaultPosition, wxSize(460, 220));
    auto *root = new wxBoxSizer(wxVERTICAL);
    auto *header = new wxBoxSizer(wxHORIZONTAL);
    header->Add(new wxStaticBitmap(&dlg, wxID_ANY,
                                   wxArtProvider::GetBitmap(wxART_EXECUTABLE_FILE, wxART_OTHER, wxSize(64, 64))),
                0, wxALL, 12);
    auto *text_col = new wxBoxSizer(wxVERTICAL);
    auto *app_name = new wxStaticText(&dlg, wxID_ANY, wxString::FromUTF8("sopwin"));
    wxFont title_font = app_name->GetFont();
    title_font.SetPointSize(title_font.GetPointSize() + 6);
    title_font.MakeBold();
    app_name->SetFont(title_font);
    text_col->Add(app_name, 0, wxBOTTOM, 4);
    text_col->Add(new wxStaticText(&dlg, wxID_ANY,
                                   wxString::Format(wxString::FromUTF8("Version %s"),
                                                    wxString::FromUTF8(PROJECT_VERSION))),
                  0, wxBOTTOM, 4);
    text_col->Add(new wxStaticText(&dlg, wxID_ANY,
                                   wxString::Format(wxString::FromUTF8("Copyright (C) %d %s"), PROJECT_YEAR,
                                                    wxString::FromUTF8(PROJECT_AUTHOR))),
                  0, wxBOTTOM, 8);
    text_col->Add(new wxStaticText(
                      &dlg, wxID_ANY,
                      wxString::FromUTF8("Guide project construction through SOP workflow steps.")),
                  0);
    /* wxEXPAND + vertical align is illegal in a horizontal box sizer. */
    header->Add(text_col, 1, wxEXPAND | wxRIGHT, 12);
    root->Add(header, 1, wxEXPAND);
    root->Add(new wxButton(&dlg, wxID_OK, wxString::FromUTF8("Close")), 0, wxALIGN_CENTER | wxBOTTOM, 12);
    dlg.SetSizer(root);
    dlg.ShowModal();
}
