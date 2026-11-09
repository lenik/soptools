#ifndef GUI_SUITE_STATUS_BAR_HPP
#define GUI_SUITE_STATUS_BAR_HPP

#include "util/suite_meta.hpp"

#include <wx/statusbr.h>
#include <wx/statbmp.h>
#include <wx/stattext.h>

/* Two-field status bar: messages on the left, suite badge (icon + name ver) on the right. */
class SuiteStatusBar : public wxStatusBar {
public:
    explicit SuiteStatusBar(wxWindow *parent);

    void SetMainMessage(const wxString &text);
    void SetSuiteMeta(const SuiteMeta &meta);

private:
    wxStaticBitmap *icon_ = nullptr;
    wxStaticText *label_ = nullptr;
    wxBitmap icon_bmp_;

    void RelayoutBadge();
    void OnSize(wxSizeEvent &evt);
};

#endif /* GUI_SUITE_STATUS_BAR_HPP */
