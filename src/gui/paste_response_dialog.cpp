/*
 * Copyright (C) 2026 Lenik <soptools@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "gui/paste_response_dialog.hpp"
#include "gui/theme.hpp"
#include "engine/stage_files.hpp"

#include <wx/artprov.h>
#include <wx/button.h>
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/listbox.h>
#include <wx/panel.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/statline.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include <cctype>
#include <filesystem>
#include <set>

namespace fs = std::filesystem;

namespace {

bool is_blank(const std::string &s) {
    for (unsigned char c : s) {
        if (!std::isspace(c)) {
            return false;
        }
    }
    return true;
}

wxStaticText *MakeAccentLabel(wxWindow *parent, const wxString &text) {
    auto *label = new wxStaticText(parent, wxID_ANY, text);
    wxFont font = label->GetFont();
    font.MakeBold();
    label->SetFont(font);
    label->SetForegroundColour(ios_accent());
    return label;
}

} /* namespace */

SopPasteResponseDialog::SopPasteResponseDialog(wxWindow *parent,
                                               const std::string &project_dir,
                                               const SopStep &step,
                                               const std::string &stage_dir)
    : wxDialog(parent, wxID_ANY, wxString::FromUTF8("Paste Response"), wxDefaultPosition,
               wxSize(920, 740), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      project_dir_(project_dir),
      stage_dir_(stage_dir),
      step_(step),
      expected_files_(expected_files_for_step(step)) {
    SetBackgroundColour(ios_bg());

    auto *root = new wxBoxSizer(wxVERTICAL);

    auto *header = new wxBoxSizer(wxHORIZONTAL);
    header->Add(new wxStaticBitmap(this, wxID_ANY,
                                   wxArtProvider::GetBitmap(wxART_INFORMATION, wxART_OTHER, wxSize(32, 32))),
                0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    auto *header_text = new wxBoxSizer(wxVERTICAL);
    header_text->Add(MakeAccentLabel(this, wxString::FromUTF8("GPT response capture")), 0, wxBOTTOM, 2);
    hint_label_ = new wxStaticText(
        this, wxID_ANY,
        wxString::FromUTF8("Paste the model reply below. Expected files are listed; drop matches in stagedir."));
    hint_label_->SetForegroundColour(ios_muted());
    header_text->Add(hint_label_, 0);
    header->Add(header_text, 1, wxEXPAND);
    header->Add(new wxStaticBitmap(this, wxID_ANY,
                                   wxArtProvider::GetBitmap(wxART_COPY, wxART_OTHER, wxSize(24, 24))),
                0, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);
    root->Add(header, 0, wxEXPAND | wxALL, 12);
    root->Add(new wxStaticLine(this), 0, wxEXPAND | wxLEFT | wxRIGHT, 12);

    auto *body_title = new wxBoxSizer(wxHORIZONTAL);
    body_title->Add(new wxStaticBitmap(this, wxID_ANY,
                                       wxArtProvider::GetBitmap(wxART_NORMAL_FILE, wxART_BUTTON, wxSize(16, 16))),
                    0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    auto *response_label = new wxStaticText(this, wxID_ANY, wxString::FromUTF8("Response body"));
    wxFont rl_font = response_label->GetFont();
    rl_font.MakeBold();
    response_label->SetFont(rl_font);
    body_title->Add(response_label, 0, wxALIGN_CENTER_VERTICAL);
    root->Add(body_title, 0, wxLEFT | wxRIGHT | wxTOP, 12);

    text_ = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                           wxTE_MULTILINE | wxTE_RICH2 | wxTE_PROCESS_TAB);
    root->Add(text_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);
    text_->Bind(wxEVT_TEXT, &SopPasteResponseDialog::OnTextChanged, this);

    auto *paste_row = new wxBoxSizer(wxHORIZONTAL);
    auto *paste_btn = new wxButton(this, wxID_ANY, wxString::FromUTF8("Paste from clipboard"));
    paste_btn->SetBitmap(wxArtProvider::GetBitmap(wxART_PASTE, wxART_BUTTON, wxSize(16, 16)));
    paste_btn->Bind(wxEVT_BUTTON, &SopPasteResponseDialog::OnPasteClipboard, this);
    paste_row->Add(paste_btn, 0, wxRIGHT, 8);
    root->Add(paste_row, 0, wxALL, 10);

    auto *files_title = new wxBoxSizer(wxHORIZONTAL);
    files_title->Add(new wxStaticBitmap(this, wxID_ANY,
                                        wxArtProvider::GetBitmap(wxART_FOLDER_OPEN, wxART_BUTTON, wxSize(16, 16))),
                     0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    auto *files_label = new wxStaticText(this, wxID_ANY, wxString::FromUTF8("Files:"));
    wxFont ll_font = files_label->GetFont();
    ll_font.MakeBold();
    files_label->SetFont(ll_font);
    files_title->Add(files_label, 0, wxALIGN_CENTER_VERTICAL);
    root->Add(files_title, 0, wxLEFT | wxRIGHT, 12);

    files_ = new wxListBox(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 140));
    root->Add(files_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);

    root->Add(new wxStaticLine(this), 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 12);
    auto *btn_row = new wxBoxSizer(wxHORIZONTAL);
    btn_row->AddStretchSpacer(1);
    save_btn_ = new wxButton(this, wxID_ANY, wxString::FromUTF8("Save"));
    save_btn_->SetBitmap(wxArtProvider::GetBitmap(wxART_FILE_SAVE, wxART_BUTTON, wxSize(16, 16)));
    auto *cancel_btn = new wxButton(this, wxID_ANY, wxString::FromUTF8("Cancel"));
    cancel_btn->SetBitmap(wxArtProvider::GetBitmap(wxART_CLOSE, wxART_BUTTON, wxSize(16, 16)));
    save_btn_->Bind(wxEVT_BUTTON, &SopPasteResponseDialog::OnSave, this);
    cancel_btn->Bind(wxEVT_BUTTON, &SopPasteResponseDialog::OnCancel, this);
    btn_row->Add(save_btn_, 0, wxRIGHT, 8);
    btn_row->Add(cancel_btn, 0);
    root->Add(btn_row, 0, wxEXPAND | wxALL, 12);

    auto *status_panel = new wxPanel(this);
    status_panel->SetBackgroundColour(ios_card());
    auto *status_row = new wxBoxSizer(wxHORIZONTAL);
    status_row->Add(new wxStaticBitmap(status_panel, wxID_ANY,
                                       wxArtProvider::GetBitmap(wxART_INFORMATION, wxART_BUTTON, wxSize(14, 14))),
                    0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 8);
    status_label_ = new wxStaticText(status_panel, wxID_ANY, wxEmptyString);
    status_label_->SetForegroundColour(ios_muted());
    status_row->Add(status_label_, 1, wxALIGN_CENTER_VERTICAL | wxTOP | wxBOTTOM | wxRIGHT, 6);
    status_panel->SetSizer(status_row);
    root->Add(status_panel, 0, wxEXPAND);

    SetSizer(root);

    if (wxTheClipboard->Open()) {
        if (wxTheClipboard->IsSupported(wxDF_TEXT)) {
            wxTextDataObject data;
            wxTheClipboard->GetData(data);
            const wxString clip = data.GetText();
            const wxString prompt = wxString::FromUTF8(step_.body);
            if (!clip.empty() && clip != prompt) {
                text_->SetValue(clip);
            }
        }
        wxTheClipboard->Close();
    }

    RebuildFileList();
    SetDialogStatus(wxString::FromUTF8("📋 Prompt has been copied to clipboard"));
}

void SopPasteResponseDialog::SetDialogStatus(const wxString &text) {
    if (status_label_) {
        status_label_->SetLabel(text);
        status_label_->GetParent()->Layout();
    }
}

void SopPasteResponseDialog::RebuildFileList() {
    files_->Clear();
    std::set<std::string> present_bases;
    if (!stage_dir_.empty()) {
        for (const auto &name : list_stage_basenames(stage_dir_)) {
            present_bases.insert(normalize_stage_basename(name));
        }
    }
    for (const auto &rel : expected_files_) {
        const std::string base = fs::path(rel).filename().string();
        const bool in_stage = present_bases.count(base) > 0;
        const bool in_project = fs::exists(fs::path(project_dir_) / rel);
        wxString label = wxString::FromUTF8(rel);
        if (in_stage) {
            label = wxString::FromUTF8("📥 ") + label;
        } else if (in_project) {
            label = wxString::FromUTF8("✓ ") + label;
        }
        files_->Append(label);
    }
    if (expected_files_.empty()) {
        SetDialogStatus(wxString::FromUTF8("No expected files listed for this step."));
    } else {
        SetDialogStatus(wxString::Format(wxString::FromUTF8("%zu expected file(s)."),
                                         expected_files_.size()));
    }
}

void SopPasteResponseDialog::OnTextChanged(wxCommandEvent &) {
    /* Body edits do not change the expected file list. */
}

void SopPasteResponseDialog::OnPasteClipboard(wxCommandEvent &) {
    if (!wxTheClipboard->Open()) {
        SetDialogStatus(wxString::FromUTF8("Could not open clipboard."));
        return;
    }
    if (wxTheClipboard->IsSupported(wxDF_TEXT)) {
        wxTextDataObject data;
        wxTheClipboard->GetData(data);
        text_->SetValue(data.GetText());
        SetDialogStatus(wxString::FromUTF8("Pasted response from clipboard."));
    } else {
        SetDialogStatus(wxString::FromUTF8("Clipboard has no text."));
    }
    wxTheClipboard->Close();
}

void SopPasteResponseDialog::OnSave(wxCommandEvent &) { DoSave(); }

void SopPasteResponseDialog::OnCancel(wxCommandEvent &) { EndModal(wxID_CANCEL); }

void SopPasteResponseDialog::DoSave() {
    /* Claim matching stage files first. */
    if (!stage_dir_.empty() && !expected_files_.empty()) {
        auto claimed = claim_stage_files(project_dir_, stage_dir_, expected_files_);
        if (!claimed.moved.empty()) {
            SetDialogStatus(wxString::FromUTF8(claimed.message));
        }
        RebuildFileList();
    }

    const std::string text = std::string(text_->GetValue().ToUTF8());
    bool any_expected_on_disk = false;
    for (const auto &rel : expected_files_) {
        if (fs::exists(fs::path(project_dir_) / rel)) {
            any_expected_on_disk = true;
            break;
        }
    }
    if (is_blank(text) && !any_expected_on_disk && step_.save_as.empty()) {
        wxMessageBox(wxString::FromUTF8("Response is empty and no expected files are present."),
                     wxString::FromUTF8("Save"), wxOK | wxICON_WARNING, this);
        SetDialogStatus(wxString::FromUTF8("Save blocked: empty response and no files."));
        return;
    }
    if (is_blank(text) && step_.save_as.empty() && any_expected_on_disk) {
        /* Stage-only capture — synthesize an OK save result. */
        save_result_.ok = true;
        save_result_.message = "stage files claimed; no response body";
        saved_ = true;
        EndModal(wxID_OK);
        return;
    }

    std::vector<SopGptAttachment> empty_atts;
    save_result_ = save_gpt_response(project_dir_, step_, text, empty_atts);
    if (!save_result_.ok) {
        /* Allow success when Save-As already satisfied by stage claim and body empty. */
        if (is_blank(text) && !step_.save_as.empty() &&
            fs::exists(fs::path(project_dir_) / step_.save_as)) {
            save_result_.ok = true;
            save_result_.message = "Save-As satisfied from stagedir";
            saved_ = true;
            EndModal(wxID_OK);
            return;
        }
        wxMessageBox(wxString::FromUTF8(save_result_.message), wxString::FromUTF8("Save failed"),
                     wxOK | wxICON_ERROR, this);
        SetDialogStatus(wxString::FromUTF8("Save failed."));
        return;
    }
    saved_ = true;
    SetDialogStatus(wxString::FromUTF8(save_result_.message));
    EndModal(wxID_OK);
}
