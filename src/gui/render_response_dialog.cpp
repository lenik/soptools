/*
 * Copyright (C) 2026 Lenik <soptools@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "gui/render_response_dialog.hpp"
#include "gui/theme.hpp"

#include <wx/artprov.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/html/htmlwin.h>
#include <wx/listbox.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/statline.h>
#include <wx/stattext.h>
#include <wx/statusbr.h>

#include <fstream>
#include <sstream>

namespace {

std::string html_escape(const std::string &s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
        case '&':
            out += "&amp;";
            break;
        case '<':
            out += "&lt;";
            break;
        case '>':
            out += "&gt;";
            break;
        case '"':
            out += "&quot;";
            break;
        default:
            out.push_back(c);
            break;
        }
    }
    return out;
}

} /* namespace */

SopRenderResponseDialog::SopRenderResponseDialog(wxWindow *parent, const SopGptSaveResult &result)
    : wxDialog(parent, wxID_ANY, wxString::FromUTF8("(Re-)Rendered response"), wxDefaultPosition,
               wxSize(1000, 740), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      result_(result) {
    SetBackgroundColour(ios_bg());

    auto *root = new wxBoxSizer(wxVERTICAL);

    auto *header = new wxBoxSizer(wxHORIZONTAL);
    header->Add(new wxStaticBitmap(this, wxID_ANY,
                                   wxArtProvider::GetBitmap(wxART_INFORMATION, wxART_OTHER, wxSize(32, 32))),
                0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    auto *header_col = new wxBoxSizer(wxVERTICAL);
    auto *title = new wxStaticText(this, wxID_ANY, wxString::FromUTF8("Review saved GPT output"));
    wxFont title_font = title->GetFont();
    title_font.MakeBold();
    title->SetFont(title_font);
    title->SetForegroundColour(ios_accent());
    header_col->Add(title, 0, wxBOTTOM, 2);
    auto *hint = new wxStaticText(
        this, wxID_ANY,
        wxString::FromUTF8("Select a part or attachment. Use Copy to put the selection on the clipboard."));
    hint->SetForegroundColour(ios_muted());
    header_col->Add(hint, 0);
    header->Add(header_col, 1, wxEXPAND);
    root->Add(header, 0, wxEXPAND | wxALL, 12);
    root->Add(new wxStaticLine(this), 0, wxEXPAND | wxLEFT | wxRIGHT, 12);

    auto *split = new wxBoxSizer(wxHORIZONTAL);

    auto *left = new wxBoxSizer(wxVERTICAL);
    auto *contents_hdr = new wxBoxSizer(wxHORIZONTAL);
    contents_hdr->Add(new wxStaticBitmap(this, wxID_ANY,
                                         wxArtProvider::GetBitmap(wxART_NORMAL_FILE, wxART_BUTTON, wxSize(16, 16))),
                      0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    auto *contents_label = new wxStaticText(this, wxID_ANY, wxString::FromUTF8("Contents"));
    wxFont cl_font = contents_label->GetFont();
    cl_font.MakeBold();
    contents_label->SetFont(cl_font);
    contents_hdr->Add(contents_label, 0, wxALIGN_CENTER_VERTICAL);
    left->Add(contents_hdr, 0, wxBOTTOM, 4);
    contents_ = new wxListBox(this, wxID_ANY, wxDefaultPosition, wxSize(180, -1));
    for (size_t i = 0; i < result_.parts.size(); i++) {
        contents_->Append(wxString::FromUTF8(result_.parts[i].label));
    }
    if (!result_.parts.empty()) {
        contents_->SetSelection(0);
    }
    contents_->Bind(wxEVT_LISTBOX, &SopRenderResponseDialog::OnContentsSelect, this);
    left->Add(contents_, 1, wxEXPAND);
    split->Add(left, 0, wxEXPAND | wxALL, 8);

    auto *right = new wxBoxSizer(wxVERTICAL);
    auto *view_hdr = new wxBoxSizer(wxHORIZONTAL);
    view_hdr->Add(new wxStaticBitmap(this, wxID_ANY,
                                     wxArtProvider::GetBitmap(wxART_REPORT_VIEW, wxART_BUTTON, wxSize(16, 16))),
                  0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    auto *view_label = new wxStaticText(this, wxID_ANY, wxString::FromUTF8("View"));
    wxFont vl_font = view_label->GetFont();
    vl_font.MakeBold();
    view_label->SetFont(vl_font);
    view_hdr->Add(view_label, 0, wxALIGN_CENTER_VERTICAL);
    right->Add(view_hdr, 0, wxBOTTOM, 4);
    html_ = new wxHtmlWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                             wxHW_SCROLLBAR_AUTO);
    right->Add(html_, 1, wxEXPAND);
    split->Add(right, 1, wxEXPAND | wxALL, 8);

    root->Add(split, 1, wxEXPAND);

    root->Add(new wxStaticLine(this), 0, wxEXPAND | wxLEFT | wxRIGHT, 8);
    auto *att_hdr = new wxBoxSizer(wxHORIZONTAL);
    att_hdr->Add(new wxStaticBitmap(this, wxID_ANY,
                                    wxArtProvider::GetBitmap(wxART_FOLDER, wxART_BUTTON, wxSize(16, 16))),
                 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    auto *att_label = new wxStaticText(this, wxID_ANY, wxString::FromUTF8("Attachments"));
    wxFont al_font = att_label->GetFont();
    al_font.MakeBold();
    att_label->SetFont(al_font);
    att_hdr->Add(att_label, 0, wxALIGN_CENTER_VERTICAL);
    root->Add(att_hdr, 0, wxLEFT | wxTOP, 8);
    attachments_ = new wxListBox(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 90), {},
                                 wxLB_SINGLE | wxLB_NEEDED_SB);
    for (const auto &att : result_.attachments) {
        if (!att.downloaded) {
            continue;
        }
        const std::string label =
            att.relative_path.empty() ? att.absolute_path : att.relative_path;
        attachments_->Append(wxString::FromUTF8(label));
    }
    attachments_->Bind(wxEVT_LISTBOX, &SopRenderResponseDialog::OnAttachSelect, this);
    root->Add(attachments_, 0, wxEXPAND | wxALL, 8);

    auto *btn_row = new wxBoxSizer(wxHORIZONTAL);
    auto_copy_box_ = new wxCheckBox(this, wxID_ANY, wxString::FromUTF8("Auto copy"));
    auto_copy_box_->Bind(wxEVT_CHECKBOX, &SopRenderResponseDialog::OnAutoCopy, this);
    btn_row->Add(auto_copy_box_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);
    btn_row->AddStretchSpacer(1);
    auto *copy_btn = new wxButton(this, wxID_ANY, wxString::FromUTF8("Copy"));
    copy_btn->SetBitmap(wxArtProvider::GetBitmap(wxART_COPY, wxART_BUTTON, wxSize(16, 16)));
    auto *close_btn = new wxButton(this, wxID_ANY, wxString::FromUTF8("Close"));
    close_btn->SetBitmap(wxArtProvider::GetBitmap(wxART_CLOSE, wxART_BUTTON, wxSize(16, 16)));
    copy_btn->Bind(wxEVT_BUTTON, &SopRenderResponseDialog::OnCopy, this);
    close_btn->Bind(wxEVT_BUTTON, &SopRenderResponseDialog::OnClose, this);
    btn_row->Add(copy_btn, 0, wxRIGHT, 8);
    btn_row->Add(close_btn, 0);
    root->Add(btn_row, 0, wxEXPAND | wxALL, 10);

    auto *status_panel = new wxPanel(this);
    status_panel->SetBackgroundColour(ios_card());
    auto *status_row = new wxBoxSizer(wxHORIZONTAL);
    status_row->Add(new wxStaticBitmap(status_panel, wxID_ANY,
                                       wxArtProvider::GetBitmap(wxART_INFORMATION, wxART_BUTTON, wxSize(14, 14))),
                    0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 8);
    status_label_ = new wxStaticText(status_panel, wxID_ANY,
                                     wxString::FromUTF8("Select a content part or attachment to review."));
    status_label_->SetForegroundColour(ios_muted());
    status_row->Add(status_label_, 1, wxALIGN_CENTER_VERTICAL | wxTOP | wxBOTTOM | wxRIGHT, 6);
    status_panel->SetSizer(status_row);
    root->Add(status_panel, 0, wxEXPAND);

    SetSizer(root);
    RefreshView();
}

std::string SopRenderResponseDialog::LoadFileText(const std::string &path) const {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return {};
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

wxString SopRenderResponseDialog::MarkdownToHtml(const std::string &md) const {
    std::ostringstream html;
    html << "<html><body style='font-family: sans-serif; font-size: 14px;'>";
    std::istringstream in(md);
    std::string line;
    bool in_code = false;
    bool in_ul = false;
    auto close_ul = [&]() {
        if (in_ul) {
            html << "</ul>";
            in_ul = false;
        }
    };
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.rfind("```", 0) == 0) {
            close_ul();
            if (!in_code) {
                html << "<pre style='background:#f4f4f4;padding:8px;'>";
                in_code = true;
            } else {
                html << "</pre>";
                in_code = false;
            }
            continue;
        }
        if (in_code) {
            html << html_escape(line) << "\n";
            continue;
        }
        if (line.rfind("### ", 0) == 0) {
            close_ul();
            html << "<h3>" << html_escape(line.substr(4)) << "</h3>";
        } else if (line.rfind("## ", 0) == 0) {
            close_ul();
            html << "<h2>" << html_escape(line.substr(3)) << "</h2>";
        } else if (line.rfind("# ", 0) == 0) {
            close_ul();
            html << "<h1>" << html_escape(line.substr(2)) << "</h1>";
        } else if (line.rfind("- ", 0) == 0 || line.rfind("* ", 0) == 0) {
            if (!in_ul) {
                html << "<ul>";
                in_ul = true;
            }
            html << "<li>" << html_escape(line.substr(2)) << "</li>";
        } else if (line.empty()) {
            close_ul();
            html << "<br/>";
        } else {
            close_ul();
            html << "<p>" << html_escape(line) << "</p>";
        }
    }
    close_ul();
    if (in_code) {
        html << "</pre>";
    }
    html << "</body></html>";
    return wxString::FromUTF8(html.str());
}

void SopRenderResponseDialog::RefreshLabels() {
    for (size_t i = 0; i < result_.parts.size(); i++) {
        wxString label = wxString::FromUTF8(result_.parts[i].label);
        if (static_cast<int>(i) == clipboard_part_) {
            label += wxString::FromUTF8(" \xF0\x9F\x93\x8B");
        }
        if (static_cast<int>(i) < contents_->GetCount()) {
            contents_->SetString(static_cast<unsigned>(i), label);
        }
    }
    int shown = 0;
    for (size_t i = 0; i < result_.attachments.size(); i++) {
        if (!result_.attachments[i].downloaded) {
            continue;
        }
        const std::string base = result_.attachments[i].relative_path.empty()
                                     ? result_.attachments[i].absolute_path
                                     : result_.attachments[i].relative_path;
        wxString label = wxString::FromUTF8(base);
        if (static_cast<int>(i) == clipboard_attach_) {
            label += wxString::FromUTF8(" \xF0\x9F\x93\x8B");
        }
        if (shown < attachments_->GetCount()) {
            attachments_->SetString(static_cast<unsigned>(shown), label);
        }
        shown++;
    }
}

void SopRenderResponseDialog::RefreshView() {
    std::string content;
    if (selected_part_ >= 0 && selected_part_ < static_cast<int>(result_.parts.size())) {
        content = result_.parts[static_cast<size_t>(selected_part_)].content;
        if (content.empty()) {
            content = LoadFileText(result_.parts[static_cast<size_t>(selected_part_)].absolute_path);
        }
    }
    html_->SetPage(MarkdownToHtml(content));
    RefreshLabels();
}

bool SopRenderResponseDialog::CopyText(const wxString &text) {
    if (!wxTheClipboard->Open()) {
        return false;
    }
    wxTheClipboard->SetData(new wxTextDataObject(text));
    wxTheClipboard->Close();
    return true;
}

void SopRenderResponseDialog::CopySelection() {
    if (selected_attach_ >= 0 && selected_attach_ < static_cast<int>(result_.attachments.size())) {
        const auto &att = result_.attachments[static_cast<size_t>(selected_attach_)];
        std::string text = LoadFileText(att.absolute_path);
        if (text.empty()) {
            text = att.absolute_path;
        }
        if (CopyText(wxString::FromUTF8(text))) {
            clipboard_attach_ = selected_attach_;
            clipboard_part_ = -1;
            RefreshLabels();
        }
        return;
    }
    if (selected_part_ >= 0 && selected_part_ < static_cast<int>(result_.parts.size())) {
        std::string content = result_.parts[static_cast<size_t>(selected_part_)].content;
        if (content.empty()) {
            content = LoadFileText(result_.parts[static_cast<size_t>(selected_part_)].absolute_path);
        }
        if (CopyText(wxString::FromUTF8(content))) {
            clipboard_part_ = selected_part_;
            clipboard_attach_ = -1;
            RefreshLabels();
        }
    }
}

void SopRenderResponseDialog::OnContentsSelect(wxCommandEvent &) {
    selected_part_ = contents_->GetSelection();
    selected_attach_ = -1;
    RefreshView();
    if (auto_copy_) {
        CopySelection();
    }
}

void SopRenderResponseDialog::OnAttachSelect(wxCommandEvent &) {
    /* Map visible attachment index to attachments_ vector index. */
    const int visible = attachments_->GetSelection();
    int shown = 0;
    selected_attach_ = -1;
    for (size_t i = 0; i < result_.attachments.size(); i++) {
        if (!result_.attachments[i].downloaded) {
            continue;
        }
        if (shown == visible) {
            selected_attach_ = static_cast<int>(i);
            break;
        }
        shown++;
    }
    if (auto_copy_) {
        CopySelection();
    } else {
        RefreshLabels();
    }
}

void SopRenderResponseDialog::OnCopy(wxCommandEvent &) {
    CopySelection();
}

void SopRenderResponseDialog::OnClose(wxCommandEvent &) {
    EndModal(wxID_OK);
}

void SopRenderResponseDialog::OnAutoCopy(wxCommandEvent &) {
    auto_copy_ = auto_copy_box_->GetValue();
    if (auto_copy_) {
        CopySelection();
    }
}
