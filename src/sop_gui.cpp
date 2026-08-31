/*
 * Copyright (C) 2026 Lenik <soptools@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "sop_gui.hpp"
#include "gui/sop_graph_canvas.hpp"
#include "gui/sop_log_view.hpp"
#include "sop_project.hpp"

#include <wx/wx.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/panel.h>
#include <wx/menu.h>
#include <wx/artprov.h>
#include <wx/toolbar.h>
#include <wx/gauge.h>
#include <wx/splitter.h>
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/filedlg.h>
#include <wx/textdlg.h>
#include <wx/bmpbuttn.h>
#include <wx/scrolwin.h>
#include <wx/statline.h>

#include "config.h"

#include <fstream>
#include <sstream>
#include <functional>

namespace {

constexpr int ID_BACK = wxID_HIGHEST + 1;
constexpr int ID_NEXT = wxID_HIGHEST + 2;
constexpr int ID_EXECUTE = wxID_HIGHEST + 3;
constexpr int ID_APPLY = wxID_HIGHEST + 4;
constexpr int ID_SHOW_LOG = wxID_HIGHEST + 7;
constexpr int ID_SHOW_GRAPH = wxID_HIGHEST + 8;
constexpr int ID_AUTO_RUN = wxID_HIGHEST + 10;
constexpr int ID_PAUSE = wxID_HIGHEST + 11;
constexpr int ID_START_RESUME = wxID_HIGHEST + 12;
constexpr int ID_COPY = wxID_HIGHEST + 13;
constexpr int ID_OPEN_PROJECT = wxID_HIGHEST + 14;
constexpr int ID_SAVE_PROJECT = wxID_HIGHEST + 15;
constexpr int ID_REVERT_PROJECT = wxID_HIGHEST + 16;
constexpr int ID_LOAD_SOP = wxID_HIGHEST + 17;
constexpr int ID_HELP_SHORTCUTS = wxID_HIGHEST + 18;
constexpr int ID_HELP_LICENSE = wxID_HIGHEST + 19;
constexpr int ID_HELP_ABOUT = wxID_HIGHEST + 20;

wxColour ios_bg() { return wxColour(242, 242, 247); }
wxColour ios_card() { return wxColour(255, 255, 255); }
wxColour ios_accent() { return wxColour(0, 122, 255); }
wxColour ios_muted() { return wxColour(142, 142, 147); }
wxColour ios_success() { return wxColour(52, 199, 89); }
wxColour ios_error() { return wxColour(255, 59, 48); }
wxColour ios_orange() { return wxColour(255, 149, 0); }
wxColour ios_sky() { return wxColour(56, 170, 230); }

wxBitmap role_icon(const SopStep &step, const wxSize &size) {
    wxArtID art = wxART_INFORMATION;
    switch (step.role) {
    case SopRole::Shell:
        art = wxART_EXECUTABLE_FILE;
        break;
    case SopRole::Gpt:
        art = wxART_TIP;
        break;
    case SopRole::Codex:
    case SopRole::AltCodex:
        art = wxART_CDROM;
        break;
    default:
        break;
    }
    return wxArtProvider::GetBitmap(art, wxART_BUTTON, size);
}

wxString status_text(const SopStep &step) {
    if (step.is_user()) {
        return wxString::FromUTF8("User");
    }
    switch (step.status) {
    case SopStepStatus::Pending:
        return wxString::FromUTF8("Pending");
    case SopStepStatus::Running:
        return wxString::FromUTF8("Running");
    case SopStepStatus::Waiting:
        return wxString::FromUTF8("Waiting");
    case SopStepStatus::Complete:
        return wxString::FromUTF8("Complete");
    case SopStepStatus::Error:
        return wxString::FromUTF8("Error");
    case SopStepStatus::User:
        return wxString::FromUTF8("User");
    }
    return wxString::FromUTF8("Unknown");
}

wxColour status_colour(const SopStep &step) {
    if (step.is_user()) {
        return ios_muted();
    }
    switch (step.status) {
    case SopStepStatus::Complete:
        return ios_success();
    case SopStepStatus::Error:
        return ios_error();
    case SopStepStatus::Running:
        return ios_orange();
    case SopStepStatus::Pending:
    case SopStepStatus::Waiting:
        return ios_sky();
    default:
        return ios_muted();
    }
}

void count_text_stats(const wxString &text, size_t &lines, size_t &chars) {
    chars = static_cast<size_t>(text.length());
    if (text.empty()) {
        lines = 0;
        return;
    }
    lines = 1;
    for (wxString::const_iterator it = text.begin(); it != text.end(); ++it) {
        if (*it == '\n') {
            lines++;
        }
    }
}

bool copy_text_to_clipboard(const wxString &text) {
    if (!wxTheClipboard->Open()) {
        return false;
    }
    wxTheClipboard->SetData(new wxTextDataObject(text));
    wxTheClipboard->Close();
    return true;
}

wxString status_bar_text(const SopStep *step, SopEngine *engine) {
    if (!step || !engine) {
        return wxString::FromUTF8("Ready");
    }
    const size_t idx = engine->current_index();
    if (idx >= engine->active_steps().size()) {
        return wxString::FromUTF8("Ready");
    }
    const std::string &sid = engine->active_steps()[idx];
    if (engine->is_shell_running(sid)) {
        return wxString::FromUTF8("Shell script is running…");
    }
    if (step->is_prompt_copy()) {
        switch (step->status) {
        case SopStepStatus::Complete:
            return wxString::FromUTF8("Step completed.");
        case SopStepStatus::Waiting:
            return wxString::FromUTF8("Paste AI output below and click Apply.");
        default:
            return wxString::FromUTF8("Click Copy to copy the prompt to the clipboard.");
        }
    }
    if (step->is_automatable()) {
        switch (step->status) {
        case SopStepStatus::Running:
            return wxString::FromUTF8("Shell script is running…");
        case SopStepStatus::Complete:
            return wxString::FromUTF8("Shell script has been successfully executed.");
        case SopStepStatus::Error:
            if (!step->status_message.empty()) {
                return wxString::FromUTF8("Shell script failed: ") +
                       wxString::FromUTF8(step->status_message);
            }
            return wxString::FromUTF8("Shell script failed.");
        case SopStepStatus::Pending:
            return wxString::FromUTF8("Ready to execute shell script.");
        default:
            break;
        }
    }
    if (step->is_user()) {
        switch (step->status) {
        case SopStepStatus::Complete:
            return wxString::FromUTF8("Step completed.");
        case SopStepStatus::Waiting:
            return wxString::FromUTF8("Paste AI output below and click Apply.");
        default:
            return wxString::FromUTF8("Complete this user step manually.");
        }
    }
    if (!step->status_message.empty()) {
        return wxString::FromUTF8(step->status_message);
    }
    return wxString::FromUTF8("Ready");
}

wxBitmapButton *MakeDetachButton(wxWindow *parent, bool detached) {
    const wxArtID art = detached ? wxART_UNDO : wxART_NEW;
    const wxString tip = detached ? wxString::FromUTF8("Attach to main window")
                                  : wxString::FromUTF8("Detach to separate window");
    auto *btn = new wxBitmapButton(parent, wxID_ANY,
                                   wxArtProvider::GetBitmap(art, wxART_BUTTON, wxSize(16, 16)),
                                   wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
    btn->SetBackgroundColour(parent->GetBackgroundColour());
    btn->SetToolTip(tip);
    return btn;
}

void UpdateDetachButton(wxBitmapButton *btn, wxWindow *parent, bool detached) {
    if (!btn) {
        return;
    }
    const wxArtID art = detached ? wxART_UNDO : wxART_NEW;
    btn->SetBitmap(wxArtProvider::GetBitmap(art, wxART_BUTTON, wxSize(16, 16)));
    btn->SetToolTip(detached ? wxString::FromUTF8("Attach to main window")
                             : wxString::FromUTF8("Detach to separate window"));
    btn->SetBackgroundColour(parent->GetBackgroundColour());
}

wxPanel *MakePaneHeader(wxWindow *parent, wxBitmapButton **button_out,
                        const std::function<void()> &on_click) {
    auto *header = new wxPanel(parent);
    header->SetBackgroundColour(parent->GetBackgroundColour());
    auto *row = new wxBoxSizer(wxHORIZONTAL);
    row->AddStretchSpacer(1);
    wxBitmapButton *btn = MakeDetachButton(header, false);
    btn->Bind(wxEVT_BUTTON, [on_click](wxCommandEvent &) { on_click(); });
    row->Add(btn, 0, wxALIGN_CENTER_VERTICAL | wxTOP | wxRIGHT, 4);
    header->SetSizer(row);
    header->SetMinSize(wxSize(-1, 28));
    if (button_out) {
        *button_out = btn;
    }
    return header;
}

wxStaticText *MakeSectionTitle(wxWindow *parent, const wxString &title) {
    auto *label = new wxStaticText(parent, wxID_ANY, title);
    wxFont font = label->GetFont();
    font.MakeBold();
    label->SetFont(font);
    return label;
}

void AddShortcutRows(wxFlexGridSizer *grid, std::initializer_list<std::pair<const char *, const char *>> rows) {
    for (const auto &row : rows) {
        auto *key = new wxStaticText(grid->GetContainingWindow(), wxID_ANY, wxString::FromUTF8(row.first));
        wxFont key_font = key->GetFont();
        key_font.MakeBold();
        key->SetFont(key_font);
        grid->Add(key, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
        grid->Add(new wxStaticText(grid->GetContainingWindow(), wxID_ANY, wxString::FromUTF8(row.second)), 0,
                wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
    }
}

wxPanel *MakeShortcutSection(wxWindow *parent, const wxString &title,
                             std::initializer_list<std::pair<const char *, const char *>> rows) {
    auto *panel = new wxPanel(parent);
    panel->SetBackgroundColour(parent->GetBackgroundColour());
    auto *section = new wxBoxSizer(wxVERTICAL);
    section->Add(MakeSectionTitle(panel, title), 0, wxBOTTOM, 6);
    auto *grid = new wxFlexGridSizer(2, 12, 4);
    grid->AddGrowableCol(1);
    AddShortcutRows(grid, rows);
    section->Add(grid, 0, wxEXPAND);
    panel->SetSizer(section);
    return panel;
}

class MainFrame : public wxFrame {
public:
    explicit MainFrame(SopEngine *engine)
        : wxFrame(nullptr, wxID_ANY, wxString::FromUTF8("sopwin"), wxDefaultPosition, wxSize(1020, 820)),
          engine_(engine) {
        SetBackgroundColour(ios_bg());
        CreateMenu();
        CreateAppToolBar();
        CreateUi();
        CreateStatusBar(1);
        SetStatusText(wxString::FromUTF8("Ready"));
        SetupAccelerators();
        engine_->set_log_fn([this](int level, const std::string &msg) { append_log(level, msg); });
        engine_->set_notify_fn([this]() {
            if (auto *app = wxTheApp) {
                app->CallAfter([this]() { OnEngineUpdate(); });
            }
        });
        if (!engine_->load_sop()) {
            wxMessageBox(wxString::FromUTF8("Failed to load SOP from ") +
                             wxString::FromUTF8(engine_->options().sop_dir),
                         wxString::FromUTF8("sopwin"), wxOK | wxICON_ERROR);
        }
        graph_layout_dirty_ = true;
        graph_->SetStepSelectHandler([this](const std::string &step_id) {
            graph_->SetSelectedStep(step_id);
            PreviewStep(step_id);
        });
        graph_->SetMoveToHandler([this](const std::string &, size_t path_index) {
            engine_->set_current_index(path_index);
            RefreshAll();
        });
        graph_->SetExcludeHandler([this](const std::string &step_id, bool excluded) {
            engine_->set_step_excluded(step_id, excluded);
            graph_layout_dirty_ = true;
            RefreshAll();
        });
        graph_->SetBranchActivateHandler([this](int seq, const std::string &step_id) {
            if (engine_->activate_branch(seq, step_id)) {
                graph_layout_dirty_ = true;
                RefreshAll();
            }
        });
        graph_->SetExecuteHandler([this](const std::string &step_id) {
            RunStepAction(step_id, true);
        });
        RefreshAll();
        UpdateWindowTitle();
        Bind(wxEVT_SIZE, &MainFrame::OnFrameSize, this);
    }

    void UpdateWindowTitle() {
        SetTitle(wxString::Format(wxString::FromUTF8("sopwin — %s"),
                                  wxString::FromUTF8(engine_->options().project_dir)));
    }

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
    bool show_log_ = false;
    bool show_graph_ = true;
    bool graph_layout_dirty_ = true;
    bool preview_mode_ = false;
    std::string preview_step_id_;
    wxString sticky_status_;
    size_t sticky_status_index_ = static_cast<size_t>(-1);

    void CreateMenu() {
        auto *menu_bar = new wxMenuBar();

        auto *file_menu = new wxMenu();
        file_menu->Append(ID_OPEN_PROJECT, wxString::FromUTF8("Open Project\tCtrl-O"));
        file_menu->Append(ID_SAVE_PROJECT, wxString::FromUTF8("Save\tCtrl-S"));
        file_menu->Append(ID_REVERT_PROJECT, wxString::FromUTF8("Revert"));
        file_menu->AppendSeparator();
        file_menu->Append(wxID_EXIT, wxString::FromUTF8("Quit\tCtrl-Q"));

        auto *procedure_menu = new wxMenu();
        procedure_menu->Append(ID_LOAD_SOP, wxString::FromUTF8("Load SOP...\tCtrl-U"));
        procedure_menu->AppendSeparator();
        procedure_menu->Append(ID_BACK, wxString::FromUTF8("Back\tPgUp"));
        procedure_menu->Append(ID_NEXT, wxString::FromUTF8("Next\tPgDn"));
        procedure_menu->AppendSeparator();
        procedure_menu->Append(ID_START_RESUME, wxString::FromUTF8("Start / Resume\tF5"));
        procedure_menu->Append(ID_PAUSE, wxString::FromUTF8("Pause\tF8"));
        procedure_menu->Append(ID_EXECUTE, wxString::FromUTF8("Execute / Copy\tCtrl-Enter"));

        view_menu_ = new wxMenu();
        view_menu_->AppendCheckItem(ID_SHOW_GRAPH, wxString::FromUTF8("Toggle Graph\tF2"));
        view_menu_->AppendCheckItem(ID_SHOW_LOG, wxString::FromUTF8("Toggle Loggings\tCtrl-L"));
        view_menu_->Check(ID_SHOW_GRAPH, true);

        auto *help_menu = new wxMenu();
        help_menu->Append(ID_HELP_SHORTCUTS, wxString::FromUTF8("Keyboard Shortcuts\tF1"));
        help_menu->AppendSeparator();
        help_menu->Append(ID_HELP_LICENSE, wxString::FromUTF8("License"));
        help_menu->Append(ID_HELP_ABOUT, wxString::FromUTF8("About"));

        menu_bar->Append(file_menu, wxString::FromUTF8("File"));
        menu_bar->Append(procedure_menu, wxString::FromUTF8("Procedure"));
        menu_bar->Append(view_menu_, wxString::FromUTF8("View"));
        menu_bar->Append(help_menu, wxString::FromUTF8("Help"));
        SetMenuBar(menu_bar);

        Bind(wxEVT_MENU, &MainFrame::OnOpenProject, this, ID_OPEN_PROJECT);
        Bind(wxEVT_MENU, &MainFrame::OnSaveProject, this, ID_SAVE_PROJECT);
        Bind(wxEVT_MENU, &MainFrame::OnRevertProject, this, ID_REVERT_PROJECT);
        Bind(wxEVT_MENU, &MainFrame::OnLoadSop, this, ID_LOAD_SOP);
        Bind(wxEVT_MENU, &MainFrame::OnBack, this, ID_BACK);
        Bind(wxEVT_MENU, &MainFrame::OnNext, this, ID_NEXT);
        Bind(wxEVT_MENU, &MainFrame::OnQuit, this, wxID_EXIT);
        Bind(wxEVT_MENU, &MainFrame::OnToggleGraph, this, ID_SHOW_GRAPH);
        Bind(wxEVT_MENU, &MainFrame::OnToggleLog, this, ID_SHOW_LOG);
        Bind(wxEVT_MENU, &MainFrame::OnExecute, this, ID_EXECUTE);
        Bind(wxEVT_MENU, &MainFrame::OnCopy, this, ID_COPY);
        Bind(wxEVT_MENU, &MainFrame::OnStartResume, this, ID_START_RESUME);
        Bind(wxEVT_MENU, &MainFrame::OnPause, this, ID_PAUSE);
        Bind(wxEVT_MENU, &MainFrame::OnHelpShortcuts, this, ID_HELP_SHORTCUTS);
        Bind(wxEVT_MENU, &MainFrame::OnHelpLicense, this, ID_HELP_LICENSE);
        Bind(wxEVT_MENU, &MainFrame::OnHelpAbout, this, ID_HELP_ABOUT);
    }

    void CreateAppToolBar() {
        toolbar_ = wxFrame::CreateToolBar(wxTB_TEXT | wxTB_HORIZONTAL | wxTB_FLAT);
        toolbar_->SetBackgroundColour(ios_card());
        toolbar_->AddTool(ID_BACK, wxString::FromUTF8("Back"), wxArtProvider::GetBitmap(wxART_GO_BACK, wxART_TOOLBAR),
                          wxString::FromUTF8("Previous step"));
        toolbar_->AddTool(ID_NEXT, wxString::FromUTF8("Next"), wxArtProvider::GetBitmap(wxART_GO_FORWARD, wxART_TOOLBAR),
                          wxString::FromUTF8("Run and go to next step"));
        toolbar_->AddSeparator();
        toolbar_->AddTool(ID_AUTO_RUN, wxString::FromUTF8("Start"), wxArtProvider::GetBitmap(wxART_GO_FORWARD, wxART_TOOLBAR),
                          wxString::FromUTF8("Start/Resume automated run (F5)"));
        toolbar_->AddTool(ID_EXECUTE, wxString::FromUTF8("Execute"),
                          wxArtProvider::GetBitmap(wxART_EXECUTABLE_FILE, wxART_TOOLBAR),
                          wxString::FromUTF8("Execute current step (Ctrl+Enter)"));
        toolbar_->Realize();
        Bind(wxEVT_TOOL, &MainFrame::OnBack, this, ID_BACK);
        Bind(wxEVT_TOOL, &MainFrame::OnNext, this, ID_NEXT);
        Bind(wxEVT_TOOL, &MainFrame::OnAutoRun, this, ID_AUTO_RUN);
        Bind(wxEVT_TOOL, &MainFrame::OnExecute, this, ID_EXECUTE);
        Bind(wxEVT_TOOL, &MainFrame::OnCopy, this, ID_COPY);
    }

    void SetupAccelerators() {
        wxAcceleratorEntry entries[11];
        entries[0].Set(wxACCEL_CTRL, static_cast<int>('O'), ID_OPEN_PROJECT);
        entries[1].Set(wxACCEL_CTRL, static_cast<int>('S'), ID_SAVE_PROJECT);
        entries[2].Set(wxACCEL_CTRL, static_cast<int>('U'), ID_LOAD_SOP);
        entries[3].Set(wxACCEL_NORMAL, WXK_PAGEUP, ID_BACK);
        entries[4].Set(wxACCEL_NORMAL, WXK_PAGEDOWN, ID_NEXT);
        entries[5].Set(wxACCEL_CTRL, WXK_RETURN, ID_EXECUTE);
        entries[6].Set(wxACCEL_NORMAL, WXK_F5, ID_START_RESUME);
        entries[7].Set(wxACCEL_NORMAL, WXK_F8, ID_PAUSE);
        entries[8].Set(wxACCEL_NORMAL, WXK_F1, ID_HELP_SHORTCUTS);
        entries[9].Set(wxACCEL_NORMAL, WXK_F2, ID_SHOW_GRAPH);
        entries[10].Set(wxACCEL_CTRL, static_cast<int>('L'), ID_SHOW_LOG);
        wxAcceleratorTable accel(11, entries);
        SetAcceleratorTable(accel);
    }

    wxPanel *MakeCard(wxWindow *parent) {
        auto *card = new wxPanel(parent);
        card->SetBackgroundColour(ios_card());
        return card;
    }

    void CreateUi() {
        // wxGTK: frame sizer must not host content directly alongside native toolbar.
        content_panel_ = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxCLIP_CHILDREN);
        content_panel_->SetBackgroundColour(ios_bg());

        auto *root = new wxBoxSizer(wxVERTICAL);

        splitter_ = new wxSplitterWindow(content_panel_, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                         wxSP_LIVE_UPDATE | wxSP_3D);
        splitter_->SetMinimumPaneSize(120);
        splitter_->SetSashGravity(0.0);

        graph_pane_ = new wxPanel(splitter_, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxCLIP_CHILDREN);
        graph_pane_->SetBackgroundColour(ios_bg());
        auto *graph_sizer = new wxBoxSizer(wxVERTICAL);
        graph_sizer->Add(MakePaneHeader(graph_pane_, &graph_detach_btn_,
                                        [this]() { ToggleGraphDetach(); }),
                         0, wxEXPAND);
        graph_ = new SopGraphCanvas(graph_pane_, engine_);
        graph_sizer->Add(graph_, 1, wxEXPAND);
        graph_pane_->SetSizer(graph_sizer);

        main_pane_ = new wxPanel(splitter_);
        main_pane_->SetBackgroundColour(ios_bg());
        auto *main_sizer = new wxBoxSizer(wxVERTICAL);

        auto *progress_row = new wxBoxSizer(wxHORIZONTAL);
        progress_gauge_ = new wxGauge(main_pane_, wxID_ANY, 100, wxDefaultPosition, wxSize(-1, 10));
        progress_gauge_->SetValue(0);
        progress_gauge_->SetBackgroundColour(ios_bg());
        progress_label_ = new wxStaticText(main_pane_, wxID_ANY, wxString::FromUTF8("0 / 0"));
        progress_label_->SetForegroundColour(ios_muted());
        progress_row->Add(progress_gauge_, 1, wxEXPAND | wxRIGHT, 10);
        progress_row->Add(progress_label_, 0, wxALIGN_CENTER_VERTICAL);
        main_sizer->Add(progress_row, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 12);

        auto *header = MakeCard(main_pane_);
        auto *header_sizer = new wxBoxSizer(wxHORIZONTAL);
        role_icon_ = new wxStaticBitmap(header, wxID_ANY, role_icon(SopStep{}, wxSize(28, 28)));
        role_label_ = new wxStaticText(header, wxID_ANY, wxString::FromUTF8("Role"));
        title_label_ = new wxStaticText(header, wxID_ANY, wxString::FromUTF8("Title"));
        desc_label_ = new wxStaticText(header, wxID_ANY, wxString::FromUTF8(""));
        status_label_ = new wxStaticText(header, wxID_ANY, wxString::FromUTF8("Pending"));
        wxFont title_font = title_label_->GetFont();
        title_font.SetPointSize(title_font.GetPointSize() + 4);
        title_font.MakeBold();
        title_label_->SetFont(title_font);
        wxFont role_font = role_label_->GetFont();
        role_font.MakeBold();
        role_label_->SetFont(role_font);
        desc_label_->SetForegroundColour(ios_muted());
        header_sizer->Add(role_icon_, 0, wxALIGN_CENTER_VERTICAL | wxALL, 14);
        auto *text_col = new wxBoxSizer(wxVERTICAL);
        text_col->Add(role_label_, 0, wxTOP | wxRIGHT, 8);
        text_col->Add(title_label_, 0, wxRIGHT, 8);
        text_col->Add(desc_label_, 0, wxRIGHT | wxBOTTOM, 8);
        header_sizer->Add(text_col, 1, wxEXPAND);
        header_sizer->Add(status_label_, 0, wxALIGN_CENTER_VERTICAL | wxALL, 14);
        header->SetSizer(header_sizer);
        main_sizer->Add(header, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 12);

        auto *content = MakeCard(main_pane_);
        auto *content_sizer = new wxBoxSizer(wxVERTICAL);
        body_view_ = new wxTextCtrl(content, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                                    wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2 | wxBORDER_NONE);
        body_view_->SetBackgroundColour(ios_card());
        wxFont mono = body_view_->GetFont();
        mono.SetFamily(wxFONTFAMILY_TELETYPE);
        body_view_->SetFont(mono);
        content_sizer->Add(body_view_, 1, wxEXPAND | wxALL, 12);

        ai_panel_ = new wxPanel(content);
        ai_panel_->SetBackgroundColour(ios_card());
        ai_input_ = new wxTextCtrl(ai_panel_, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(-1, 160),
                                   wxTE_MULTILINE | wxBORDER_NONE);
        ai_input_->SetHint(wxString::FromUTF8("Paste AI output here..."));
        auto *apply_btn = new wxButton(ai_panel_, ID_APPLY, wxString::FromUTF8("Apply"));
        apply_btn->Bind(wxEVT_BUTTON, &MainFrame::OnApply, this);
        auto *ai_sizer = new wxBoxSizer(wxVERTICAL);
        ai_sizer->Add(ai_input_, 1, wxEXPAND | wxALL, 12);
        ai_sizer->Add(apply_btn, 0, wxALL, 12);
        ai_panel_->SetSizer(ai_sizer);
        content_sizer->Add(ai_panel_, 0, wxEXPAND);

        content->SetSizer(content_sizer);
        main_sizer->Add(content, 1, wxEXPAND | wxALL, 12);

        log_host_ = new wxPanel(main_pane_);
        log_host_->SetBackgroundColour(ios_bg());
        log_host_sizer_ = new wxBoxSizer(wxVERTICAL);
        log_host_sizer_->Add(MakePaneHeader(log_host_, &log_detach_btn_, [this]() { ToggleLogDetach(); }), 0,
                             wxEXPAND);
        log_view_ = new SopLogView(log_host_);
        log_view_->SetAttachHandler([this]() { ReattachLogPane(); });
        log_host_sizer_->Add(log_view_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
        log_host_->SetSizer(log_host_sizer_);
        log_host_->Hide();
        main_sizer->Add(log_host_, 0, wxEXPAND);

        main_pane_->SetSizer(main_sizer);
        splitter_->SplitHorizontally(graph_pane_, main_pane_, graph_split_sash_);
        root->Add(splitter_, 1, wxEXPAND | wxALL, 12);
        content_panel_->SetSizer(root);

        auto *frame_root = new wxBoxSizer(wxVERTICAL);
        frame_root->Add(content_panel_, 1, wxEXPAND);
        SetSizer(frame_root);
    }

    void RelayoutMainPanes() {
        if (content_panel_) {
            content_panel_->Layout();
        }
        if (graph_pane_) {
            graph_pane_->Layout();
        }
        if (main_pane_) {
            main_pane_->Layout();
        }
        if (splitter_) {
            splitter_->Layout();
            splitter_->UpdateSize();
            if (splitter_->IsSplit()) {
                const int client_h = splitter_->GetClientSize().y;
                const int min_pane = splitter_->GetMinimumPaneSize();
                const int max_sash = std::max(min_pane, client_h - min_pane - 4);
                if (splitter_->GetSashPosition() > max_sash) {
                    graph_split_sash_ = max_sash;
                    splitter_->SetSashPosition(max_sash);
                }
            }
        }
        if (GetSizer()) {
            GetSizer()->Layout();
        }
        Layout();
        if (graph_) {
            graph_->SendSizeEvent();
        }
    }

    void OnFrameSize(wxSizeEvent &evt) {
        evt.Skip();
        RelayoutMainPanes();
    }

    void append_log(int level, const std::string &msg) {
        if (log_view_) {
            log_view_->Append(level, msg);
        }
    }

    void SetStatusBarMessage(const wxString &text, bool sticky = false) {
        SetStatusText(text);
        if (sticky) {
            sticky_status_ = text;
            sticky_status_index_ = engine_->current_index();
        } else {
            sticky_status_.clear();
            sticky_status_index_ = static_cast<size_t>(-1);
        }
    }

    bool CopyStepPrompt(const SopStep &step, wxString *err = nullptr) {
        if (!step.is_prompt_copy()) {
            if (err) {
                *err = wxString::FromUTF8("This step has no prompt to copy.");
            }
            return false;
        }
        const wxString text = wxString::FromUTF8(step.body);
        if (!copy_text_to_clipboard(text)) {
            if (err) {
                *err = wxString::FromUTF8("Failed to copy prompt to clipboard.");
            }
            return false;
        }
        size_t lines = 0;
        size_t chars = 0;
        count_text_stats(text, lines, chars);
        SetStatusBarMessage(wxString::Format(
                                wxString::FromUTF8("Prompt has been copied to the clipboard: %zu lines, %zu chars."),
                                lines, chars),
                            true);
        append_log(1, "prompt copied to clipboard");
        return true;
    }

    void RunStepAction(const std::string &step_id, bool force) {
        if (!engine_->definition().steps.count(step_id)) {
            return;
        }
        const SopStep &step = engine_->definition().steps.at(step_id);
        if (step.is_prompt_copy()) {
            wxString err;
            if (!CopyStepPrompt(step, &err)) {
                SetStatusBarMessage(err, true);
            }
            RefreshProgressBar();
            RefreshStepView();
            RefreshNav();
            return;
        }
        if (step.is_user()) {
            SetStatusBarMessage(wxString::FromUTF8("Paste AI output below and click Apply."));
            append_log(1, "user step: paste AI output and click Apply");
            RefreshAll();
            return;
        }
        if (engine_->execute_step(step_id, force)) {
            append_log(1, force ? "re-executed step" : "executed step");
            if (engine_->is_shell_running(step_id)) {
                SetStatusBarMessage(wxString::FromUTF8("Shell script is running…"));
            }
        }
        RefreshAll();
    }

    void DoExecute(bool force) {
        if (engine_->current_index() >= engine_->active_steps().size()) {
            return;
        }
        RunStepAction(engine_->active_steps()[engine_->current_index()], force);
    }

    void DoCopy() {
        const SopStep *step = engine_->step_at(engine_->current_index());
        if (!step) {
            return;
        }
        wxString err;
        if (!CopyStepPrompt(*step, &err)) {
            SetStatusBarMessage(err, true);
        }
        RefreshProgressBar();
        RefreshStepView();
        RefreshNav();
        RefreshStatusBar();
    }

    void RefreshProgressBar() {
        const size_t total = engine_->active_steps().size();
        const size_t current = engine_->current_index();
        int pct = 0;
        if (total > 0) {
            pct = static_cast<int>(current * 100 / total);
        }
        progress_gauge_->SetValue(pct);
        progress_label_->SetLabel(wxString::Format("%zu / %zu", current + 1, total));
        if (graph_layout_dirty_) {
            graph_->Rebuild();
            graph_layout_dirty_ = false;
        } else {
            graph_->SyncNodeStates();
        }
        graph_->SetCurrentIndex(current);
        if (current < engine_->active_steps().size()) {
            const std::string step_id = engine_->active_steps()[current];
            CallAfter([this, step_id]() {
                if (!graph_) {
                    return;
                }
                RelayoutMainPanes();
                graph_->ScrollToStepId(step_id, false);
            });
        }
    }

    void PreviewStep(const std::string &step_id) {
        const auto &def = engine_->definition();
        const auto it = def.steps.find(step_id);
        if (it == def.steps.end()) {
            return;
        }
        const SopStep &step = it->second;
        preview_mode_ = true;
        preview_step_id_ = step_id;

        role_icon_->SetBitmap(role_icon(step, wxSize(28, 28)));
        role_label_->SetLabel(wxString::FromUTF8(step.role_label()));
        title_label_->SetLabel(wxString::FromUTF8(step.display_title()));
        desc_label_->SetLabel(wxString::FromUTF8(step.description));
        status_label_->SetLabel(status_text(step));
        status_label_->SetForegroundColour(status_colour(step));
        body_view_->SetValue(wxString::FromUTF8(step.body));
        ai_panel_->Show(step.is_user());
        Layout();
    }

    void RefreshStepView() {
        preview_mode_ = false;
        preview_step_id_.clear();
        const SopStep *step = engine_->step_at(engine_->current_index());
        if (!step) {
            return;
        }

        role_icon_->SetBitmap(role_icon(*step, wxSize(28, 28)));
        role_label_->SetLabel(wxString::FromUTF8(step->role_label()));
        title_label_->SetLabel(wxString::FromUTF8(step->display_title()));
        desc_label_->SetLabel(wxString::FromUTF8(step->description));
        status_label_->SetLabel(status_text(*step));
        status_label_->SetForegroundColour(status_colour(*step));
        body_view_->SetValue(wxString::FromUTF8(step->body));
        ai_panel_->Show(step->is_user());
        Layout();
    }

    void RefreshStatusBar() {
        if (!sticky_status_.empty() && sticky_status_index_ == engine_->current_index()) {
            SetStatusText(sticky_status_);
            return;
        }
        sticky_status_.clear();
        sticky_status_index_ = static_cast<size_t>(-1);
        const SopStep *step = engine_->step_at(engine_->current_index());
        SetStatusText(status_bar_text(step, engine_));
    }

    void RefreshActionTool() {
        if (!toolbar_) {
            return;
        }
        const SopStep *step = engine_->step_at(engine_->current_index());
        const bool copy_mode = step && step->is_prompt_copy();
        const bool exec_mode = step && step->is_automatable();

        int pos = toolbar_->GetToolPos(ID_EXECUTE);
        if (pos == wxNOT_FOUND) {
            pos = toolbar_->GetToolPos(ID_COPY);
        }
        if (pos == wxNOT_FOUND) {
            pos = static_cast<int>(toolbar_->GetToolsCount());
        }

        if (toolbar_->GetToolPos(ID_EXECUTE) != wxNOT_FOUND) {
            toolbar_->DeleteTool(ID_EXECUTE);
        }
        if (toolbar_->GetToolPos(ID_COPY) != wxNOT_FOUND) {
            toolbar_->DeleteTool(ID_COPY);
        }

        if (copy_mode) {
            toolbar_->InsertTool(pos, ID_COPY, wxString::FromUTF8("Copy"),
                                 wxArtProvider::GetBitmap(wxART_COPY, wxART_TOOLBAR), wxNullBitmap,
                                 wxITEM_NORMAL, wxString::FromUTF8("Copy prompt to clipboard (Ctrl+Enter)"));
        } else if (exec_mode) {
            toolbar_->InsertTool(pos, ID_EXECUTE, wxString::FromUTF8("Execute"),
                                 wxArtProvider::GetBitmap(wxART_EXECUTABLE_FILE, wxART_TOOLBAR), wxNullBitmap,
                                 wxITEM_NORMAL, wxString::FromUTF8("Execute current step (Ctrl+Enter)"));
        }
        toolbar_->Realize();
    }

    void RefreshAutoRunTool() {
        if (!toolbar_) {
            return;
        }
        wxString label = wxString::FromUTF8("Start");
        switch (engine_->auto_run_state()) {
        case SopAutoRunState::Running:
            label = wxString::FromUTF8("Pause");
            break;
        case SopAutoRunState::Paused:
            label = wxString::FromUTF8("Resume");
            break;
        default:
            break;
        }
        const wxString help = label + wxString::FromUTF8(" (F5)");
        const int pos = toolbar_->GetToolPos(ID_AUTO_RUN);
        if (pos != wxNOT_FOUND) {
            toolbar_->DeleteTool(ID_AUTO_RUN);
            toolbar_->InsertTool(pos, ID_AUTO_RUN, label,
                                 wxArtProvider::GetBitmap(wxART_GO_FORWARD, wxART_TOOLBAR), wxNullBitmap,
                                 wxITEM_NORMAL, help);
            toolbar_->Realize();
        }
    }

    void RefreshNav() {
        if (toolbar_) {
            toolbar_->EnableTool(ID_BACK, engine_->current_index() > 0);
            toolbar_->EnableTool(ID_NEXT, engine_->can_advance());
        }
        RefreshAutoRunTool();
        RefreshActionTool();
    }

    void SetGraphVisible(bool visible) {
        show_graph_ = visible;
        if (view_menu_) {
            view_menu_->Check(ID_SHOW_GRAPH, visible);
        }
        if (graph_detached_frame_) {
            graph_detached_frame_->Show(visible);
            if (visible) {
                graph_detached_frame_->Raise();
            }
            return;
        }
        if (!splitter_ || !graph_pane_ || !main_pane_) {
            return;
        }
        if (visible) {
            graph_pane_->Show();
            if (!splitter_->IsSplit()) {
                splitter_->SplitHorizontally(graph_pane_, main_pane_, graph_split_sash_);
            }
        } else {
            if (splitter_->IsSplit()) {
                graph_split_sash_ = splitter_->GetSashPosition();
                splitter_->Unsplit(graph_pane_);
            }
            graph_pane_->Hide();
        }
        RelayoutMainPanes();
        if (graph_ && engine_->current_index() < engine_->active_steps().size()) {
            graph_->ScrollToStepId(engine_->active_steps()[engine_->current_index()], false);
        }
    }

    void SetLoggingVisible(bool visible) {
        show_log_ = visible;
        if (view_menu_) {
            view_menu_->Check(ID_SHOW_LOG, visible);
        }
        if (log_view_->IsDetached()) {
            log_view_->SetLoggingVisible(visible);
        } else {
            log_host_->Show(visible);
            log_view_->SetLoggingVisible(visible);
        }
        RelayoutMainPanes();
    }

    void ToggleGraphDetach() {
        if (graph_detached_frame_) {
            AttachGraphPane();
            return;
        }
        DetachGraphPane();
    }

    void DetachGraphPane() {
        if (graph_detached_frame_ || !splitter_ || !splitter_->IsSplit()) {
            return;
        }
        graph_split_sash_ = splitter_->GetSashPosition();
        splitter_->Unsplit(graph_pane_);

        graph_detached_frame_ =
            new wxFrame(nullptr, wxID_ANY, wxString::FromUTF8("sopwin — Graph"), wxDefaultPosition, wxSize(900, 520));
        graph_detached_frame_->SetBackgroundColour(ios_bg());
        auto *frame_sizer = new wxBoxSizer(wxVERTICAL);
        graph_pane_->Reparent(graph_detached_frame_);
        frame_sizer->Add(graph_pane_, 1, wxEXPAND);
        graph_detached_frame_->SetSizer(frame_sizer);
        graph_detached_frame_->Bind(wxEVT_CLOSE_WINDOW, &MainFrame::OnGraphDetachedClose, this);
        graph_detached_frame_->Show(true);
        UpdateDetachButton(graph_detach_btn_, graph_pane_, true);
        RelayoutMainPanes();
    }

    void AttachGraphPane() {
        if (!graph_detached_frame_) {
            return;
        }
        graph_detached_frame_->Unbind(wxEVT_CLOSE_WINDOW, &MainFrame::OnGraphDetachedClose, this);
        graph_pane_->Reparent(splitter_);
        graph_detached_frame_->Hide();
        graph_detached_frame_->Destroy();
        graph_detached_frame_ = nullptr;
        UpdateDetachButton(graph_detach_btn_, graph_pane_, false);
        if (show_graph_) {
            graph_pane_->Show();
            splitter_->SplitHorizontally(graph_pane_, main_pane_, graph_split_sash_);
        } else {
            graph_pane_->Hide();
        }
        RelayoutMainPanes();
        if (graph_ && engine_->current_index() < engine_->active_steps().size()) {
            graph_->ScrollToStepId(engine_->active_steps()[engine_->current_index()], false);
        }
    }

    void DetachLogPane() {
        SetLoggingVisible(true);
        if (log_view_->IsDetached()) {
            return;
        }
        log_view_->Detach();
        log_host_->Hide();
        UpdateDetachButton(log_detach_btn_, log_host_, true);
        RelayoutMainPanes();
    }

    void ReattachLogPane() {
        if (!log_view_->IsDetached()) {
            return;
        }
        log_view_->AttachTo(log_host_, log_host_sizer_);
        log_host_->Show(show_log_);
        UpdateDetachButton(log_detach_btn_, log_host_, false);
        RelayoutMainPanes();
    }

    void ToggleLogDetach() {
        if (log_view_->IsDetached()) {
            ReattachLogPane();
        } else {
            DetachLogPane();
        }
    }

    void RefreshAll() {
        RefreshProgressBar();
        RefreshStepView();
        RefreshNav();
        RefreshStatusBar();
    }

    void OnBack(wxCommandEvent &) {
        sticky_status_.clear();
        engine_->retreat();
        RefreshAll();
    }

    void OnNext(wxCommandEvent &) {
        if (engine_->try_advance_with_run()) {
            sticky_status_.clear();
            RefreshAll();
        }
    }

    void OnExecute(wxCommandEvent &) {
        const SopStep *step = engine_->step_at(engine_->current_index());
        if (step && step->is_prompt_copy()) {
            DoCopy();
            return;
        }
        DoExecute(true);
    }

    void OnCopy(wxCommandEvent &) { DoCopy(); }

    void OnAutoRun(wxCommandEvent &) {
        if (engine_->auto_run_state() == SopAutoRunState::Running) {
            engine_->pause_auto_run();
        } else {
            engine_->start_auto_run();
        }
        RefreshAll();
    }

    void OnStartResume(wxCommandEvent &) {
        if (engine_->auto_run_state() == SopAutoRunState::Running) {
            return;
        }
        engine_->start_auto_run();
        RefreshAll();
    }

    void OnPause(wxCommandEvent &) {
        engine_->pause_auto_run();
        RefreshAll();
    }

    void OnApply(wxCommandEvent &) {
        const std::string sid = engine_->active_steps()[engine_->current_index()];
        const wxString text = ai_input_->GetValue();
        auto result = engine_->apply_ai_output(sid, text.ToUTF8().data());
        append_log(result.ok ? 1 : 0, result.message);
        if (result.ok) {
            ai_input_->Clear();
            SetStatusBarMessage(wxString::FromUTF8(result.message), true);
        } else {
            SetStatusBarMessage(wxString::FromUTF8("Apply failed: ") + wxString::FromUTF8(result.message));
            wxMessageBox(wxString::FromUTF8(result.message), wxString::FromUTF8("Apply failed"), wxOK | wxICON_ERROR);
        }
        RefreshAll();
    }

    void OnGraphDetachedClose(wxCloseEvent &evt) {
        evt.Veto();
        AttachGraphPane();
    }

    void OnToggleGraph(wxCommandEvent &) { SetGraphVisible(!show_graph_); }

    void OnToggleLog(wxCommandEvent &) { SetLoggingVisible(!show_log_); }

    void OnEngineUpdate() {
        engine_->poll_completion();
        if (engine_->auto_run_state() == SopAutoRunState::Running) {
            engine_->tick_auto_run();
        }
        RefreshAll();
    }

    void OnQuit(wxCommandEvent &) { Close(true); }

    void OnOpenProject(wxCommandEvent &) {
        wxDirDialog dlg(this, wxString::FromUTF8("Open Project"), wxString::FromUTF8(engine_->options().project_dir),
                        wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
        if (dlg.ShowModal() != wxID_OK) {
            return;
        }
        engine_->set_project_dir(dlg.GetPath().ToUTF8().data());
        if (!engine_->load_sop()) {
            wxMessageBox(wxString::FromUTF8("Failed to load SOP from ") +
                             wxString::FromUTF8(engine_->options().sop_dir),
                         wxString::FromUTF8("sopwin"), wxOK | wxICON_ERROR);
            return;
        }
        graph_layout_dirty_ = true;
        sticky_status_.clear();
        UpdateWindowTitle();
        RefreshAll();
        SetStatusText(wxString::FromUTF8("Project opened."));
    }

    void OnSaveProject(wxCommandEvent &) {
        std::string err;
        if (!save_project_config(*engine_, &err)) {
            wxMessageBox(wxString::FromUTF8(err.empty() ? "Save failed." : err), wxString::FromUTF8("Save failed"),
                         wxOK | wxICON_ERROR);
            return;
        }
        SetStatusText(wxString::FromUTF8("Project saved."));
    }

    void OnRevertProject(wxCommandEvent &) {
        if (wxMessageBox(wxString::FromUTF8("Reload project state from the saved configuration file?"),
                         wxString::FromUTF8("Revert project"), wxYES_NO | wxICON_QUESTION) != wxYES) {
            return;
        }
        std::string err;
        if (!revert_project_config(*engine_, &err)) {
            wxMessageBox(wxString::FromUTF8(err.empty() ? "Revert failed." : err),
                         wxString::FromUTF8("Revert failed"), wxOK | wxICON_ERROR);
            return;
        }
        graph_layout_dirty_ = true;
        sticky_status_.clear();
        RefreshAll();
        SetStatusText(wxString::FromUTF8("Project reverted."));
    }

    void OnLoadSop(wxCommandEvent &) {
        wxDirDialog dlg(this, wxString::FromUTF8("Load SOP"), wxString::FromUTF8(engine_->options().sop_dir),
                        wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
        if (dlg.ShowModal() != wxID_OK) {
            return;
        }
        if (!engine_->reload_sop(dlg.GetPath().ToUTF8().data())) {
            wxMessageBox(wxString::FromUTF8("Failed to load SOP."), wxString::FromUTF8("Load SOP"), wxOK | wxICON_ERROR);
            return;
        }
        graph_layout_dirty_ = true;
        sticky_status_.clear();
        RefreshAll();
        SetStatusText(wxString::FromUTF8("SOP loaded."));
    }

    void OnHelpShortcuts(wxCommandEvent &) {
        wxDialog dlg(this, wxID_ANY, wxString::FromUTF8("Keyboard Shortcuts"), wxDefaultPosition, wxSize(520, 420));
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

    void OnHelpLicense(wxCommandEvent &) {
        std::ifstream in("LICENSE");
        if (!in) {
#ifdef SOURCE_ROOT
            in.open(std::string(SOURCE_ROOT) + "/LICENSE");
#endif
        }
        std::ostringstream body;
        body << in.rdbuf();

        wxDialog dlg(this, wxID_ANY, wxString::FromUTF8("License"), wxDefaultPosition, wxSize(680, 460));
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

    void OnHelpAbout(wxCommandEvent &) {
        wxDialog dlg(this, wxID_ANY, wxString::FromUTF8("About sopwin"), wxDefaultPosition, wxSize(460, 220));
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
};

SopEngine *g_sop_engine = nullptr;

class SoptoolsApp : public wxApp {
public:
    bool OnInit() override {
        if (!wxApp::OnInit() || !g_sop_engine) {
            return false;
        }
        auto *frame = new MainFrame(g_sop_engine);
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP_NO_MAIN(SoptoolsApp);

} /* namespace */

int run_gui_mode(SopEngine &engine, int argc, char **argv) {
    g_sop_engine = &engine;
    return wxEntry(argc, argv);
}
