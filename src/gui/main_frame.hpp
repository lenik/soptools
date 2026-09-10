#ifndef GUI_MAIN_FRAME_HPP
#define GUI_MAIN_FRAME_HPP

#include "engine/engine.hpp"
#include "gui/graph_canvas.hpp"
#include "gui/log_view.hpp"

#include <wx/bmpbuttn.h>
#include <wx/frame.h>
#include <wx/gauge.h>
#include <wx/menu.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/statbmp.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/toolbar.h>

#include <string>

class MainFrame : public wxFrame {
public:
    explicit MainFrame(SopEngine *engine);
    void UpdateWindowTitle();

private:
    SopEngine *engine_;
    SopGraphCanvas *graph_ = nullptr;
    wxPanel *graph_pane_ = nullptr;
    wxPanel *main_pane_ = nullptr;
    wxFrame *graph_detached_frame_ = nullptr;
    wxBitmapButton *graph_detach_btn_ = nullptr;
    wxBitmapButton *log_detach_btn_ = nullptr;
    int graph_split_sash_ = 220;
    wxGauge *progress_gauge_ = nullptr;
    wxStaticText *progress_label_ = nullptr;
    wxStaticBitmap *role_icon_ = nullptr;
    wxStaticText *role_label_ = nullptr;
    wxStaticText *title_label_ = nullptr;
    wxStaticText *desc_label_ = nullptr;
    wxStaticText *status_label_ = nullptr;
    wxTextCtrl *body_view_ = nullptr;
    wxTextCtrl *ai_input_ = nullptr;
    wxPanel *ai_panel_ = nullptr;
    SopLogView *log_view_ = nullptr;
    wxPanel *log_host_ = nullptr;
    wxBoxSizer *log_host_sizer_ = nullptr;
    wxSplitterWindow *splitter_ = nullptr;
    wxPanel *content_panel_ = nullptr;
    wxToolBar *toolbar_ = nullptr;
    wxMenu *view_menu_ = nullptr;
    wxMenu *lang_menu_ = nullptr;
    bool show_log_ = false;
    bool show_graph_ = true;
    bool graph_layout_dirty_ = true;
    bool preview_mode_ = false;
    std::string preview_step_id_;
    std::string last_current_step_id_;
    wxString sticky_status_;
    size_t sticky_status_index_ = static_cast<size_t>(-1);

    void CreateMenu();
    void CreateAppToolBar();
    void SetupAccelerators();
    wxPanel *MakeCard(wxWindow *parent);
    void CreateUi();
    wxRect ContentAreaRect() const;
    void RelayoutMainPanes();
    void OnFrameSize(wxSizeEvent &evt);
    void append_log(int level, const std::string &msg);
    void SetStatusBarMessage(const wxString &text, bool sticky = false);
    bool OpenGptPasteFlow(const SopStep &step);
    bool CopyStepPrompt(const SopStep &step, wxString *err = nullptr);
    /* If current step is an incomplete GPT .get, copy prompt + paste dialog. */
    bool MaybeOpenGptPasteForCurrent();
    void RunStepAction(const std::string &step_id, bool force);
    void DoExecute(bool force);
    void DoCopy();
    void RefreshProgressBar();
    void PreviewStep(const std::string &step_id);
    void RefreshStepView();
    void RefreshStatusBar();
    void RefreshActionTool();
    void RefreshAutoRunTool();
    void RefreshNav();
    void SyncLanguageMenu();
    void SwitchLanguagePack(const std::string &lang);
    void SetGraphVisible(bool visible);
    void SetLoggingVisible(bool visible);
    void ToggleGraphDetach();
    void DetachGraphPane();
    void AttachGraphPane();
    void DetachLogPane();
    void ReattachLogPane();
    void ToggleLogDetach();
    void RefreshAll();
    void OnBack(wxCommandEvent &);
    void OnNext(wxCommandEvent &);
    void OnExecute(wxCommandEvent &);
    void OnCopy(wxCommandEvent &);
    void OnAutoRun(wxCommandEvent &);
    void OnStartResume(wxCommandEvent &);
    void OnPause(wxCommandEvent &);
    void OnApply(wxCommandEvent &);
    void OnGraphDetachedClose(wxCloseEvent &evt);
    void OnToggleGraph(wxCommandEvent &);
    void OnToggleLog(wxCommandEvent &);
    void OnEngineUpdate();
    void OnQuit(wxCommandEvent &);
    void OnOpenProject(wxCommandEvent &);
    void OnSaveProject(wxCommandEvent &);
    void OnRevertProject(wxCommandEvent &);
    void OnLoadSop(wxCommandEvent &);
    void OnHelpShortcuts(wxCommandEvent &);
    void OnHelpLicense(wxCommandEvent &);
    void OnHelpAbout(wxCommandEvent &);
    void OnLanguagePack(wxCommandEvent &);
};

wxFrame *create_main_frame(SopEngine *engine);

#endif /* GUI_MAIN_FRAME_HPP */
