#ifndef UI_GUI_LOG_VIEW_HPP
#define UI_GUI_LOG_VIEW_HPP

#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/listctrl.h>
#include <wx/choice.h>
#include <wx/textctrl.h>
#include <wx/frame.h>
#include <chrono>
#include <functional>
#include <string>
#include <vector>

struct SopLogEntry {
    std::chrono::system_clock::time_point time;
    int level = 0;
    std::string message;
};

class SopLogView : public wxPanel {
public:
    SopLogView(wxWindow *parent);

    void Append(int level, const std::string &message);
    void Clear();
    void Detach();
    void AttachTo(wxWindow *parent, wxSizer *sizer);
    bool IsDetached() const { return detached_frame_ != nullptr; }
    void SetAttachHandler(std::function<void()> fn) { attach_fn_ = std::move(fn); }
    void SetLoggingVisible(bool visible);
    bool IsLoggingVisible() const;

private:
    wxListCtrl *list_ = nullptr;
    wxChoice *level_filter_ = nullptr;
    wxTextCtrl *keyword_filter_ = nullptr;
    wxFrame *detached_frame_ = nullptr;
    wxSizer *host_sizer_ = nullptr;
    wxWindow *host_parent_ = nullptr;
    std::function<void()> attach_fn_;
    std::vector<SopLogEntry> entries_;

    void RefreshList();
    bool PassesFilter(const SopLogEntry &e) const;
    wxString FormatTime(const SopLogEntry &e) const;
    wxString LevelLabel(int level) const;

    void OnLevelFilter(wxCommandEvent &);
    void OnKeywordFilter(wxCommandEvent &);
    void OnDetachClose(wxCloseEvent &);
};

#endif /* UI_GUI_LOG_VIEW_HPP */
