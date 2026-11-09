/*
 * Copyright (C) 2026 Lenik <soptools@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "gui/codex_copy_dialog.hpp"
#include "gui/theme.hpp"
#include "gui/display_util.hpp"

#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/stattext.h>

#include <cstdio>

bool show_codex_copy_dialog(wxWindow *parent, const SopStep &step) {
    char seq_buf[16];
    std::snprintf(seq_buf, sizeof(seq_buf), "%03d", step.seq);

    const wxString title = wxString::Format(wxString::FromUTF8("👤 Codex [%s]: 已复制到剪贴板"),
                                           wxString::FromUTF8(seq_buf));

    wxDialog dlg(parent, wxID_ANY, title, wxDefaultPosition, wxSize(480, 260),
                 wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
    dlg.SetBackgroundColour(ios_bg());

    auto *root = new wxBoxSizer(wxVERTICAL);
    root->AddSpacer(24);

    auto *emoji = new wxStaticText(&dlg, wxID_ANY, wxString::FromUTF8("📋"));
    wxFont emoji_font = emoji->GetFont();
    emoji_font.SetPointSize(emoji_font.GetPointSize() + 18);
    emoji->SetFont(emoji_font);
    root->Add(emoji, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, 12);

    const wxString body = wxString::Format(
        wxString::FromUTF8("已复制 %s 到剪贴板，请转发给Codex..."),
        wxString::FromUTF8(step.display_title()));
    auto *msg = new wxStaticText(&dlg, wxID_ANY, body, wxDefaultPosition, wxDefaultSize,
                                 wxALIGN_CENTRE_HORIZONTAL);
    msg->Wrap(400);
    root->Add(msg, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT, 24);

    root->AddSpacer(20);
    root->Add(new wxStaticLine(&dlg), 0, wxEXPAND | wxLEFT | wxRIGHT, 16);

    auto *btns = new wxBoxSizer(wxHORIZONTAL);
    btns->AddStretchSpacer(1);
    auto *ok = new wxButton(&dlg, wxID_OK, wxString::FromUTF8("确定"));
    auto *cancel = new wxButton(&dlg, wxID_CANCEL, wxString::FromUTF8("取消"));
    ok->SetDefault();
    btns->Add(ok, 0, wxRIGHT, 8);
    btns->Add(cancel, 0);
    root->Add(btns, 0, wxEXPAND | wxALL, 16);

    dlg.SetSizer(root);
    centre_on_same_display(&dlg, parent);
    return dlg.ShowModal() == wxID_OK;
}
