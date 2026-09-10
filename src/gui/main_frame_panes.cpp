/*
 * Copyright (C) 2026 Lenik <soptools@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "gui/main_frame.hpp"
#include "gui/theme.hpp"
#include "gui/help_dialogs.hpp"
#include "gui/paste_response_dialog.hpp"
#include "gui/render_response_dialog.hpp"
#include "engine/project.hpp"
#include "util/paths.hpp"

#include <wx/wx.h>
#include <wx/artprov.h>
#include <wx/filedlg.h>
#include <wx/textdlg.h>
#include <wx/scrolwin.h>
#include <wx/statline.h>
#include <wx/clipbrd.h>
#include <wx/dataobj.h>

#include "config.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <functional>
#include <string>

#include "gui/command_ids.hpp"

void MainFrame::SetGraphVisible(bool visible) {
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
        if (graph_) {
            graph_->Show();
        }
        if (!splitter_->IsSplit()) {
            splitter_->SplitHorizontally(graph_pane_, main_pane_, graph_split_sash_);
        }
    } else {
        if (splitter_->IsSplit()) {
            graph_split_sash_ = splitter_->GetSashPosition();
            splitter_->Unsplit(graph_pane_);
        }
        graph_pane_->Hide();
        if (graph_) {
            graph_->Hide();
        }
    }
    RelayoutMainPanes();
    if (toolbar_) {
        toolbar_->Raise();
        toolbar_->Refresh(true);
    }
    Refresh(true);
    Update();
}

void MainFrame::SetLoggingVisible(bool visible) {
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

void MainFrame::ToggleGraphDetach() {
    if (graph_detached_frame_) {
        AttachGraphPane();
        return;
    }
    DetachGraphPane();
}

void MainFrame::DetachGraphPane() {
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

void MainFrame::AttachGraphPane() {
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
}

void MainFrame::DetachLogPane() {
    SetLoggingVisible(true);
    if (log_view_->IsDetached()) {
        return;
    }
    log_view_->Detach();
    log_host_->Hide();
    UpdateDetachButton(log_detach_btn_, log_host_, true);
    RelayoutMainPanes();
}

void MainFrame::ReattachLogPane() {
    if (!log_view_->IsDetached()) {
        return;
    }
    log_view_->AttachTo(log_host_, log_host_sizer_);
    log_host_->Show(show_log_);
    UpdateDetachButton(log_detach_btn_, log_host_, false);
    RelayoutMainPanes();
}

void MainFrame::ToggleLogDetach() {
    if (log_view_->IsDetached()) {
        ReattachLogPane();
    } else {
        DetachLogPane();
    }
}

void MainFrame::RefreshAll() {
    RefreshProgressBar();
    RefreshStepView();
    RefreshNav();
    RefreshStatusBar();
}

void MainFrame::OnBack(wxCommandEvent &) {
    sticky_status_.clear();
    engine_->retreat();
    RefreshAll();
}

void MainFrame::OnNext(wxCommandEvent &) {
    sticky_status_.clear();

    const SopStep *step = engine_->step_at(engine_->current_index());
    if (step && step->is_gpt_get()) {
        const std::string sid = engine_->current_step_id();
        if (!engine_->is_complete(sid)) {
            if (!OpenGptPasteFlow(*step)) {
                RefreshAll();
                return;
            }
        }
        if (engine_->advance()) {
            RefreshAll();
            MaybeOpenGptPasteForCurrent();
        } else {
            RefreshAll();
        }
        return;
    }

    if (engine_->try_advance_with_run()) {
        RefreshAll();
        MaybeOpenGptPasteForCurrent();
    }
}

void MainFrame::OnExecute(wxCommandEvent &) {
    const SopStep *step = engine_->step_at(engine_->current_index());
    if (step && step->is_prompt_copy()) {
        DoCopy();
        return;
    }
    DoExecute(true);
}

void MainFrame::OnCopy(wxCommandEvent &) { DoCopy(); }

void MainFrame::OnAutoRun(wxCommandEvent &) {
    if (engine_->auto_run_state() == SopAutoRunState::Running) {
        engine_->pause_auto_run();
    } else {
        engine_->start_auto_run();
    }
    RefreshAll();
}

void MainFrame::OnStartResume(wxCommandEvent &) {
    if (engine_->auto_run_state() == SopAutoRunState::Running) {
        return;
    }
    engine_->start_auto_run();
    RefreshAll();
}

void MainFrame::OnPause(wxCommandEvent &) {
    engine_->pause_auto_run();
    RefreshAll();
}

void MainFrame::OnApply(wxCommandEvent &) {
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

void MainFrame::OnGraphDetachedClose(wxCloseEvent &evt) {
    evt.Veto();
    AttachGraphPane();
}

void MainFrame::OnToggleGraph(wxCommandEvent &) { SetGraphVisible(!show_graph_); }

void MainFrame::OnToggleLog(wxCommandEvent &) { SetLoggingVisible(!show_log_); }

void MainFrame::OnEngineUpdate() {
    engine_->poll_completion();
    if (engine_->auto_run_state() == SopAutoRunState::Running) {
        engine_->tick_auto_run();
    }
    RefreshAll();
}

void MainFrame::OnQuit(wxCommandEvent &) { Close(true); }

void MainFrame::OnOpenProject(wxCommandEvent &) {
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

void MainFrame::OnSaveProject(wxCommandEvent &) {
    std::string err;
    if (!save_project_config(*engine_, &err)) {
        wxMessageBox(wxString::FromUTF8(err.empty() ? "Save failed." : err), wxString::FromUTF8("Save failed"),
                     wxOK | wxICON_ERROR);
        return;
    }
    SetStatusText(wxString::FromUTF8("Project status saved to sop/status."));
}

void MainFrame::OnRevertProject(wxCommandEvent &) {
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

void MainFrame::OnLoadSop(wxCommandEvent &) {
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
    SyncLanguageMenu();
    SetStatusText(wxString::FromUTF8("SOP loaded."));
}

void MainFrame::OnHelpShortcuts(wxCommandEvent &) { show_shortcuts_dialog(this); }

void MainFrame::OnHelpLicense(wxCommandEvent &) { show_license_dialog(this); }

void MainFrame::OnHelpAbout(wxCommandEvent &) { show_about_dialog(this); }

void MainFrame::SyncLanguageMenu() {
    if (!lang_menu_) {
        return;
    }
    const std::string name = std::filesystem::path(engine_->options().sop_dir).filename().string();
    int id = ID_LANG_EN;
    if (name.find("zh_CN") != std::string::npos) {
        id = ID_LANG_ZH_CN;
    } else if (name.find("-ja") != std::string::npos || name == "worldman-ja.sop") {
        id = ID_LANG_JA;
    }
    lang_menu_->Check(id, true);
}

void MainFrame::SwitchLanguagePack(const std::string &lang) {
#ifdef SOURCE_ROOT
    const std::string source_root = SOURCE_ROOT;
#else
    const std::string source_root = ".";
#endif
    const std::string pack =
        resolve_worldman_sop_pack(lang, engine_->options().sop_dir, source_root);
    if (pack.empty()) {
        wxMessageBox(wxString::Format(wxString::FromUTF8("WorldMan SOP pack for language \"%s\" was not found."),
                                      wxString::FromUTF8(lang)),
                     wxString::FromUTF8("Language"), wxOK | wxICON_ERROR);
        SyncLanguageMenu();
        return;
    }
    {
        std::error_code ec;
        if (std::filesystem::equivalent(pack, engine_->options().sop_dir, ec) && !ec) {
            SyncLanguageMenu();
            return;
        }
    }
    if (!engine_->reload_sop(pack)) {
        wxMessageBox(wxString::FromUTF8("Failed to load SOP pack:\n") + wxString::FromUTF8(pack),
                     wxString::FromUTF8("Language"), wxOK | wxICON_ERROR);
        SyncLanguageMenu();
        return;
    }
    graph_layout_dirty_ = true;
    sticky_status_.clear();
    RefreshAll();
    SyncLanguageMenu();
    SetStatusBarMessage(wxString::Format(wxString::FromUTF8("Switched SOP language pack to %s."),
                                         wxString::FromUTF8(std::filesystem::path(pack).filename().string())),
                        true);
}

void MainFrame::OnLanguagePack(wxCommandEvent &evt) {
    std::string lang = "en";
    if (evt.GetId() == ID_LANG_ZH_CN) {
        lang = "zh_CN";
    } else if (evt.GetId() == ID_LANG_JA) {
        lang = "ja";
    }
    SwitchLanguagePack(lang);
}

