/*
 * Copyright (C) 2026 Lenik <soptools@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "gui/suite_status_bar.hpp"

#include <wx/image.h>

namespace {

constexpr int kFieldMsg = 0;
constexpr int kFieldSuite = 1;
constexpr int kIconPx = 16;
constexpr int kPad = 6;

wxBitmap load_suite_bitmap(const std::string &png_path) {
    if (png_path.empty()) {
        return {};
    }
    wxImage img(wxString::FromUTF8(png_path), wxBITMAP_TYPE_PNG);
    if (!img.IsOk()) {
        return {};
    }
    if (img.GetWidth() != kIconPx || img.GetHeight() != kIconPx) {
        img = img.Scale(kIconPx, kIconPx, wxIMAGE_QUALITY_HIGH);
    }
    return wxBitmap(img);
}

} /* namespace */

SuiteStatusBar::SuiteStatusBar(wxWindow *parent)
    : wxStatusBar(parent, wxID_ANY, wxSTB_DEFAULT_STYLE, wxString::FromUTF8("suite-status")) {
    static const int widths[2] = {-1, 168};
    SetFieldsCount(2, widths);
    SetMinHeight(kIconPx + 8);

    icon_ = new wxStaticBitmap(this, wxID_ANY, wxNullBitmap);
    label_ = new wxStaticText(this, wxID_ANY, wxString::FromUTF8("SOP"));
    label_->SetForegroundColour(GetForegroundColour());

    Bind(wxEVT_SIZE, &SuiteStatusBar::OnSize, this);
    RelayoutBadge();
}

void SuiteStatusBar::SetMainMessage(const wxString &text) {
    SetStatusText(text, kFieldMsg);
}

void SuiteStatusBar::SetSuiteMeta(const SuiteMeta &meta) {
    const wxString text = wxString::FromUTF8(suite_badge_label(meta));
    label_->SetLabel(text);
    SetStatusText(wxEmptyString, kFieldSuite); /* children draw the badge */

    icon_bmp_ = load_suite_bitmap(meta.icon_png);
    if (icon_bmp_.IsOk()) {
        icon_->SetBitmap(icon_bmp_);
        icon_->Show(true);
    } else {
        icon_->SetBitmap(wxNullBitmap);
        icon_->Show(false);
    }

    /* Widen right field for long names. */
    const int text_w = label_->GetBestSize().GetWidth();
    const int need = kPad + (icon_->IsShown() ? kIconPx + 4 : 0) + text_w + kPad + 8;
    int widths[2] = {-1, need < 120 ? 120 : need};
    SetStatusWidths(2, widths);
    RelayoutBadge();
}

void SuiteStatusBar::RelayoutBadge() {
    wxRect r;
    if (!GetFieldRect(kFieldSuite, r)) {
        return;
    }
    int x = r.x + kPad;
    const int y_icon = r.y + (r.height - kIconPx) / 2;
    if (icon_->IsShown()) {
        icon_->SetSize(x, y_icon, kIconPx, kIconPx);
        x += kIconPx + 4;
    }
    const wxSize ls = label_->GetBestSize();
    const int y_text = r.y + (r.height - ls.GetHeight()) / 2;
    label_->SetSize(x, y_text, ls.GetWidth(), ls.GetHeight());
}

void SuiteStatusBar::OnSize(wxSizeEvent &evt) {
    RelayoutBadge();
    evt.Skip();
}
