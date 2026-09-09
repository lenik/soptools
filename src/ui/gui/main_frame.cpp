/*
 * Copyright (C) 2026 Lenik <soptools@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "ui/gui/main_frame.hpp"
#include "ui/gui/theme.hpp"
#include "ui/gui/help_dialogs.hpp"
#include "ui/gui/paste_response_dialog.hpp"
#include "ui/gui/render_response_dialog.hpp"
#include "engine/project.hpp"

#include <wx/wx.h>
#include <wx/artprov.h>
#include <wx/filedlg.h>
#include <wx/textdlg.h>
#include <wx/scrolwin.h>
#include <wx/statline.h>
#include <wx/clipbrd.h>
#include <wx/dataobj.h>

#include "config.h"

#include <fstream>
#include <sstream>
#include <functional>

#include "ui/gui/command_ids.hpp"


MainFrame::MainFrame(SopEngine *engine)
    : wxFrame(nullptr, wxID_ANY, wxString::FromUTF8("sopwin"), wxDefaultPosition, wxSize(1020, 820)),
      engine_(engine) {
    SetBackgroundColour(ios_bg());
    CreateMenu();
    CreateAppToolBar();
    CreateStatusBar(1);
    SetStatusText(wxString::FromUTF8("Ready"));
    CreateUi();
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
    RelayoutMainPanes();
    Bind(wxEVT_SIZE, &MainFrame::OnFrameSize, this);
}

void MainFrame::UpdateWindowTitle() {
    SetTitle(wxString::Format(wxString::FromUTF8("sopwin — %s"),
                              wxString::FromUTF8(engine_->options().project_dir)));
}

void MainFrame::CreateMenu() {
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

void MainFrame::CreateAppToolBar() {
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

void MainFrame::SetupAccelerators() {
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

wxPanel *MainFrame::MakeCard(wxWindow *parent) {
    auto *card = new wxPanel(parent);
    card->SetBackgroundColour(ios_card());
    return card;
}

void MainFrame::CreateUi() {
    content_panel_ = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                 wxTAB_TRAVERSAL | wxCLIP_CHILDREN);
    content_panel_->SetBackgroundColour(ios_bg());

    auto *root = new wxBoxSizer(wxVERTICAL);

    splitter_ = new wxSplitterWindow(content_panel_, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                     wxSP_LIVE_UPDATE | wxSP_3D);
    splitter_->SetMinimumPaneSize(80);
    splitter_->SetSashGravity(0.0);

    graph_pane_ = new wxPanel(splitter_, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxCLIP_CHILDREN);
    graph_pane_->SetBackgroundColour(ios_bg());
    auto *graph_sizer = new wxBoxSizer(wxVERTICAL);
    graph_sizer->Add(MakePaneHeader(graph_pane_, &graph_detach_btn_,
                                    [this]() { ToggleGraphDetach(); }),
                     0, wxEXPAND);
    graph_ = new SopGraphCanvas(graph_pane_, engine_);
    graph_sizer->Add(graph_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
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
    root->Add(splitter_, 1, wxEXPAND | wxALL, 8);
    content_panel_->SetSizer(root);
}


wxFrame *create_main_frame(SopEngine *engine) {
    return new MainFrame(engine);
}
