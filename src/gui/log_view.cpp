/*
 * Copyright (C) 2026 Lenik <soptools@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "gui/log_view.hpp"

#include <wx/sizer.h>
#include <wx/listctrl.h>
#include <wx/stattext.h>
#include <wx/frame.h>
#include <wx/bmpbuttn.h>
#include <wx/artprov.h>
#include <iomanip>
#include <sstream>

namespace {

wxColour ios_card() { return wxColour(255, 255, 255); }
wxColour ios_muted() { return wxColour(142, 142, 147); }

} /* namespace */

SopLogView::SopLogView(wxWindow *parent) : wxPanel(parent) {
    SetBackgroundColour(ios_card());
    auto *root = new wxBoxSizer(wxVERTICAL);

    auto *filter_row = new wxBoxSizer(wxHORIZONTAL);
    filter_row->Add(new wxStaticText(this, wxID_ANY, wxString::FromUTF8("Level:")), 0,
                    wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    level_filter_ = new wxChoice(this, wxID_ANY);
    level_filter_->Append(wxString::FromUTF8("All"));
    level_filter_->Append(wxString::FromUTF8("0+ Error"));
    level_filter_->Append(wxString::FromUTF8("1+ Info"));
    level_filter_->Append(wxString::FromUTF8("2+ Debug"));
    level_filter_->SetSelection(0);
    filter_row->Add(level_filter_, 0, wxRIGHT, 12);
    filter_row->Add(new wxStaticText(this, wxID_ANY, wxString::FromUTF8("Keyword:")), 0,
                    wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    keyword_filter_ = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(220, -1));
    filter_row->Add(keyword_filter_, 1, wxEXPAND);
    root->Add(filter_row, 0, wxEXPAND | wxALL, 8);

    list_ = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                           wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_NONE);
    list_->SetBackgroundColour(ios_card());
    list_->InsertColumn(0, wxString::FromUTF8("Time"), wxLIST_FORMAT_LEFT, 110);
    list_->InsertColumn(1, wxString::FromUTF8("Level"), wxLIST_FORMAT_LEFT, 60);
    list_->InsertColumn(2, wxString::FromUTF8("Message"), wxLIST_FORMAT_LEFT, 520);
    root->Add(list_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    SetSizer(root);

    level_filter_->Bind(wxEVT_CHOICE, &SopLogView::OnLevelFilter, this);
    keyword_filter_->Bind(wxEVT_TEXT, &SopLogView::OnKeywordFilter, this);
}

wxString SopLogView::FormatTime(const SopLogEntry &e) const {
    const std::time_t t = std::chrono::system_clock::to_time_t(e.time);
    std::tm tm_buf{};
#if defined(_WIN32)
    localtime_s(&tm_buf, &t);
#else
    localtime_r(&t, &tm_buf);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%H:%M:%S");
    return wxString::FromUTF8(oss.str());
}

wxString SopLogView::LevelLabel(int level) const {
    if (level <= 0) {
        return wxString::FromUTF8("ERR");
    }
    if (level == 1) {
        return wxString::FromUTF8("INF");
    }
    return wxString::Format("DBG%d", level);
}

bool SopLogView::PassesFilter(const SopLogEntry &e) const {
    const int min_level = level_filter_ ? level_filter_->GetSelection() - 1 : -1;
    if (min_level >= 0 && e.level < min_level) {
        return false;
    }
    const wxString kw = keyword_filter_ ? keyword_filter_->GetValue().Lower() : wxString();
    if (kw.empty()) {
        return true;
    }
    const wxString msg = wxString::FromUTF8(e.message).Lower();
    return msg.Contains(kw);
}

void SopLogView::RefreshList() {
    if (!list_) {
        return;
    }
    list_->DeleteAllItems();
    long row = 0;
    for (const SopLogEntry &e : entries_) {
        if (!PassesFilter(e)) {
            continue;
        }
        list_->InsertItem(row, FormatTime(e));
        list_->SetItem(row, 1, LevelLabel(e.level));
        list_->SetItem(row, 2, wxString::FromUTF8(e.message));
        row++;
    }
}

void SopLogView::Append(int level, const std::string &message) {
    SopLogEntry e;
    e.time = std::chrono::system_clock::now();
    e.level = level;
    e.message = message;
    entries_.push_back(e);
    if (PassesFilter(e) && list_) {
        const long row = list_->GetItemCount();
        list_->InsertItem(row, FormatTime(e));
        list_->SetItem(row, 1, LevelLabel(e.level));
        list_->SetItem(row, 2, wxString::FromUTF8(e.message));
        list_->EnsureVisible(row);
    }
}

void SopLogView::Clear() {
    entries_.clear();
    if (list_) {
        list_->DeleteAllItems();
    }
}

void SopLogView::OnLevelFilter(wxCommandEvent &) { RefreshList(); }
void SopLogView::OnKeywordFilter(wxCommandEvent &) { RefreshList(); }

void SopLogView::Detach() {
    if (detached_frame_) {
        detached_frame_->Raise();
        return;
    }
    host_parent_ = GetParent();
    host_sizer_ = nullptr;
    if (host_parent_) {
        wxSizer *sizer = host_parent_->GetSizer();
        if (sizer) {
            host_sizer_ = static_cast<wxBoxSizer *>(sizer);
            host_sizer_->Detach(this);
            host_parent_->Layout();
        }
    }
    Hide();

    detached_frame_ = new wxFrame(nullptr, wxID_ANY, wxString::FromUTF8("sopwin — Loggings"),
                                  wxDefaultPosition, wxSize(760, 420));
    detached_frame_->SetBackgroundColour(ios_card());
    auto *frame_sizer = new wxBoxSizer(wxVERTICAL);
    auto *header = new wxPanel(detached_frame_);
    header->SetBackgroundColour(ios_card());
    auto *header_row = new wxBoxSizer(wxHORIZONTAL);
    header_row->AddStretchSpacer(1);
    auto *attach_btn =
        new wxBitmapButton(header, wxID_ANY,
                           wxArtProvider::GetBitmap(wxART_UNDO, wxART_BUTTON, wxSize(16, 16)),
                           wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
    attach_btn->SetBackgroundColour(ios_card());
    attach_btn->SetToolTip(wxString::FromUTF8("Attach to main window"));
    attach_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent &) {
        if (attach_fn_) {
            attach_fn_();
        }
    });
    header_row->Add(attach_btn, 0, wxALIGN_CENTER_VERTICAL | wxTOP | wxRIGHT, 4);
    header->SetSizer(header_row);
    frame_sizer->Add(header, 0, wxEXPAND);
    Reparent(detached_frame_);
    frame_sizer->Add(this, 1, wxEXPAND);
    detached_frame_->SetSizer(frame_sizer);
    Show();
    detached_frame_->Bind(wxEVT_CLOSE_WINDOW, &SopLogView::OnDetachClose, this);
    detached_frame_->Show(true);
}

void SopLogView::AttachTo(wxWindow *parent, wxSizer *sizer) {
    if (detached_frame_) {
        detached_frame_->Unbind(wxEVT_CLOSE_WINDOW, &SopLogView::OnDetachClose, this);
        detached_frame_->Hide();
        detached_frame_->Destroy();
        detached_frame_ = nullptr;
    }
    Reparent(parent);
    if (sizer) {
        sizer->Add(this, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    }
    Show();
    parent->Show();
    parent->Layout();
    if (wxWindow *grand = parent->GetParent()) {
        grand->Layout();
    }
}

void SopLogView::OnDetachClose(wxCloseEvent &evt) {
    evt.Veto();
    if (attach_fn_) {
        attach_fn_();
        return;
    }
    if (host_parent_ && host_sizer_) {
        AttachTo(host_parent_, host_sizer_);
    } else {
        Hide();
    }
}

void SopLogView::SetLoggingVisible(bool visible) {
    if (detached_frame_) {
        if (visible) {
            detached_frame_->Show(true);
            detached_frame_->Raise();
        } else {
            detached_frame_->Show(false);
        }
        return;
    }
    if (visible) {
        Show();
        if (wxWindow *parent = GetParent()) {
            parent->Show(true);
            parent->Layout();
        }
    } else {
        Hide();
        if (wxWindow *parent = GetParent()) {
            parent->Layout();
        }
    }
}

bool SopLogView::IsLoggingVisible() const {
    if (detached_frame_) {
        return detached_frame_->IsShown();
    }
    return IsShown() && GetParent() && GetParent()->IsShown();
}
