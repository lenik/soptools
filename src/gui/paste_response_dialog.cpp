/*
 * Copyright (C) 2026 Lenik <soptools@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "gui/paste_response_dialog.hpp"
#include "gui/theme.hpp"

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
#include <wx/statusbr.h>
#include <wx/textctrl.h>

#include <cctype>
#include <filesystem>
#include <thread>

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
                                               const SopStep &step)
    : wxDialog(parent, wxID_ANY, wxString::FromUTF8("Paste Response"), wxDefaultPosition,
               wxSize(920, 740), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      project_dir_(project_dir),
      step_(step) {
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
        wxString::FromUTF8("Paste the model reply below. Download links are detected automatically."));
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

    auto *links_title = new wxBoxSizer(wxHORIZONTAL);
    links_title->Add(new wxStaticBitmap(this, wxID_ANY,
                                        wxArtProvider::GetBitmap(wxART_FOLDER_OPEN, wxART_BUTTON, wxSize(16, 16))),
                     0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    auto *links_label = new wxStaticText(this, wxID_ANY, wxString::FromUTF8("Download links"));
    wxFont ll_font = links_label->GetFont();
    ll_font.MakeBold();
    links_label->SetFont(ll_font);
    links_title->Add(links_label, 0, wxALIGN_CENTER_VERTICAL);
    root->Add(links_title, 0, wxLEFT | wxRIGHT, 12);

    links_ = new wxListBox(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 140));
    root->Add(links_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);

    auto *dl_row = new wxBoxSizer(wxHORIZONTAL);
    download_btn_ = new wxButton(this, wxID_ANY, wxString::FromUTF8("Download"));
    download_btn_->SetBitmap(wxArtProvider::GetBitmap(wxART_GO_DOWN, wxART_BUTTON, wxSize(16, 16)));
    download_btn_->Bind(wxEVT_BUTTON, &SopPasteResponseDialog::OnDownload, this);
    dl_row->Add(download_btn_, 0, wxRIGHT, 8);
    root->Add(dl_row, 0, wxLEFT | wxRIGHT | wxTOP, 10);

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
                RebuildLinkList();
            }
        }
        wxTheClipboard->Close();
    }

    SetDialogStatus(wxString::FromUTF8("📋 Prompt has been copied to clipboard"));
}

void SopPasteResponseDialog::SetDialogStatus(const wxString &text) {
    if (status_label_) {
        status_label_->SetLabel(text);
        status_label_->GetParent()->Layout();
    }
}

void SopPasteResponseDialog::RebuildLinkList() {
    const std::string text = std::string(text_->GetValue().ToUTF8());
    const auto urls = extract_download_urls(text);

    std::vector<SopGptAttachment> previous = attachments_;
    attachments_.clear();
    links_->Clear();

    for (const auto &url : urls) {
        SopGptAttachment att;
        att.url = url;
        for (const auto &prev : previous) {
            if (prev.url == url && prev.downloaded) {
                att = prev;
                break;
            }
        }
        attachments_.push_back(att);
        wxString label = wxString::FromUTF8(url);
        if (att.downloaded) {
            label = wxString::FromUTF8("\xE2\x9C\x85 ") + label;
        }
        links_->Append(label);
    }

    if (urls.empty()) {
        SetDialogStatus(wxString::FromUTF8("No download links detected in the response."));
    } else {
        SetDialogStatus(wxString::Format(wxString::FromUTF8("Detected %zu download link(s)."), urls.size()));
    }
}

void SopPasteResponseDialog::OnTextChanged(wxCommandEvent &) {
    RebuildLinkList();
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
        RebuildLinkList();
        SetDialogStatus(wxString::FromUTF8("Pasted response from clipboard."));
    } else {
        SetDialogStatus(wxString::FromUTF8("Clipboard has no text."));
    }
    wxTheClipboard->Close();
}

void SopPasteResponseDialog::OnDownload(wxCommandEvent &) {
    StartDownloads(false);
}

void SopPasteResponseDialog::OnSave(wxCommandEvent &) {
    StartDownloads(true);
}

void SopPasteResponseDialog::OnCancel(wxCommandEvent &) {
    EndModal(wxID_CANCEL);
}

void SopPasteResponseDialog::StartDownloads(bool then_save) {
    if (downloading_) {
        return;
    }
    save_after_download_ = then_save;

    std::vector<size_t> pending;
    for (size_t i = 0; i < attachments_.size(); i++) {
        if (!attachments_[i].downloaded) {
            pending.push_back(i);
        }
    }

    if (pending.empty()) {
        if (then_save) {
            DoSave();
        } else {
            SetDialogStatus(wxString::FromUTF8("Nothing to download."));
        }
        return;
    }

    downloading_ = true;
    pending_downloads_ = static_cast<int>(pending.size());
    download_btn_->Enable(false);
    save_btn_->Enable(false);
    SetDialogStatus(wxString::Format(wxString::FromUTF8("Downloading %d file(s)…"), pending_downloads_));

    const fs::path tmp_dir = fs::temp_directory_path() / "sopwin-dl";
    std::error_code ec;
    fs::create_directories(tmp_dir, ec);

    for (size_t index : pending) {
        const std::string url = attachments_[index].url;
        std::string name = "att_" + std::to_string(index) + ".bin";
        const size_t slash = url.find_last_of('/');
        if (slash != std::string::npos) {
            std::string base = url.substr(slash + 1);
            const size_t q = base.find('?');
            if (q != std::string::npos) {
                base = base.substr(0, q);
            }
            if (!base.empty()) {
                name = base;
            }
        }
        const std::string dest = (tmp_dir / (std::to_string(index) + "_" + name)).string();

        std::thread([this, index, url, dest]() {
            std::string err;
            const bool ok = download_url_to_file(url, dest, &err);
            CallAfter([this, index, ok, dest, err]() {
                MarkLinkDone(index, ok, dest, err);
            });
        }).detach();
    }
}

void SopPasteResponseDialog::MarkLinkDone(size_t index, bool ok, const std::string &path,
                                          const std::string &err) {
    if (index < attachments_.size()) {
        attachments_[index].downloaded = ok;
        attachments_[index].absolute_path = ok ? path : "";
        attachments_[index].error = err;
        if (index < static_cast<size_t>(links_->GetCount())) {
            wxString label = wxString::FromUTF8(attachments_[index].url);
            if (ok) {
                label = wxString::FromUTF8("\xE2\x9C\x85 ") + label;
            } else {
                label = wxString::FromUTF8("\xE2\x9D\x8C ") + label;
            }
            links_->SetString(static_cast<unsigned int>(index), label);
        }
    }

    pending_downloads_--;
    if (pending_downloads_ <= 0) {
        FinishDownloadsAndMaybeSave();
    } else {
        SetDialogStatus(wxString::Format(wxString::FromUTF8("%d download(s) remaining…"), pending_downloads_));
    }
}

void SopPasteResponseDialog::FinishDownloadsAndMaybeSave() {
    downloading_ = false;
    download_btn_->Enable(true);
    save_btn_->Enable(true);
    SetDialogStatus(wxString::FromUTF8("Downloads finished."));
    if (save_after_download_) {
        DoSave();
    }
}

void SopPasteResponseDialog::DoSave() {
    const std::string text = std::string(text_->GetValue().ToUTF8());
    size_t downloaded = 0;
    for (const auto &att : attachments_) {
        if (att.downloaded) {
            downloaded++;
        }
    }
    if (is_blank(text) && downloaded == 0) {
        wxMessageBox(wxString::FromUTF8("Response is empty and there are no downloaded attachments."),
                     wxString::FromUTF8("Save"), wxOK | wxICON_WARNING, this);
        SetDialogStatus(wxString::FromUTF8("Save blocked: empty response and no attachments."));
        return;
    }
    save_result_ = save_gpt_response(project_dir_, step_, text, attachments_);
    if (!save_result_.ok) {
        wxMessageBox(wxString::FromUTF8(save_result_.message), wxString::FromUTF8("Save failed"),
                     wxOK | wxICON_ERROR, this);
        SetDialogStatus(wxString::FromUTF8("Save failed."));
        return;
    }
    saved_ = true;
    SetDialogStatus(wxString::FromUTF8(save_result_.message));
    EndModal(wxID_OK);
}
