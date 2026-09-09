/*
 * Copyright (C) 2026 Lenik <soptools@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "ui/gui/help_dialogs.hpp"
#include "ui/gui/theme.hpp"

#include "config.h"

#include <wx/artprov.h>
#include <wx/button.h>
#include <wx/dialog.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/statline.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include <fstream>
#include <sstream>

void show_shortcuts_dialog(wxWindow *parent) {
    wxDialog dlg(parent, wxID_ANY, wxString::FromUTF8("Keyboard Shortcuts"), wxDefaultPosition, wxSize(520, 420));
    auto *root = new wxBoxSizer(wxVERTICAL);
    root->Add(MakeShortcutSection(
                  &dlg, wxString::FromUTF8("File"),
                  {{"Ctrl+O", "Open Project"},
                   {"Ctrl+S", "Save project configuration"},
                   {"Ctrl+Q", "Quit"}}),
              0, wxEXPAND | wxALL, 12);
    root->Add(new wxStaticLine(&dlg), 0, wxEXPAND | wxLEFT | wxRIGHT, 12);
    root->Add(MakeShortcutSection(
                  &dlg, wxString::FromUTF8("Procedure"),
                  {{"Ctrl+U", "Load SOP"},
                   {"PgUp", "Back"},
                   {"PgDn", "Next"},
                   {"F5", "Start / Resume auto-run"},
                   {"F8", "Pause auto-run"},
                   {"Ctrl+Enter", "Execute / Copy current step"}}),
              0, wxEXPAND | wxALL, 12);
    root->Add(new wxStaticLine(&dlg), 0, wxEXPAND | wxLEFT | wxRIGHT, 12);
    root->Add(MakeShortcutSection(
                  &dlg, wxString::FromUTF8("View"),
                  {{"F2", "Toggle Graph"}, {"Ctrl+L", "Toggle Loggings"}}),
              0, wxEXPAND | wxALL, 12);
    root->Add(new wxStaticLine(&dlg), 0, wxEXPAND | wxLEFT | wxRIGHT, 12);
    root->Add(MakeShortcutSection(&dlg, wxString::FromUTF8("Help"), {{"F1", "Keyboard Shortcuts"}}), 0,
              wxEXPAND | wxALL, 12);
    auto *close_btn = new wxButton(&dlg, wxID_OK, wxString::FromUTF8("Close"));
    root->Add(close_btn, 0, wxALIGN_CENTER | wxBOTTOM, 12);
    dlg.SetSizer(root);
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

    wxDialog dlg(parent, wxID_ANY, wxString::FromUTF8("License"), wxDefaultPosition, wxSize(680, 460));
    auto *root = new wxBoxSizer(wxVERTICAL);
    auto *txt = new wxTextCtrl(&dlg, wxID_ANY, wxString::FromUTF8(body.str()), wxDefaultPosition, wxSize(-1, 360),
                                wxTE_MULTILINE | wxTE_READONLY | wxTE_WORDWRAP | wxBORDER_SUNKEN);
    wxFont mono = txt->GetFont();
    mono.SetFamily(wxFONTFAMILY_TELETYPE);
    txt->SetFont(mono);
    root->Add(txt, 1, wxEXPAND | wxALL, 12);
    root->Add(new wxButton(&dlg, wxID_OK, wxString::FromUTF8("Close")), 0, wxALIGN_CENTER | wxBOTTOM, 12);
    dlg.SetSizer(root);
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
    header->Add(text_col, 1, wxEXPAND | wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);
    root->Add(header, 1, wxEXPAND);
    root->Add(new wxButton(&dlg, wxID_OK, wxString::FromUTF8("Close")), 0, wxALIGN_CENTER | wxBOTTOM, 12);
    dlg.SetSizer(root);
    dlg.ShowModal();
}
