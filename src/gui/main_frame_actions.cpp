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

#include "gui/command_ids.hpp"

wxRect MainFrame::ContentAreaRect() const {
    int cw = 0;
    int ch = 0;
    GetClientSize(&cw, &ch);

    int top = 0;
    int bottom = ch;

    if (toolbar_ && toolbar_->IsShown()) {
        const wxRect tb = toolbar_->GetRect();
        top = std::max(0, tb.GetBottom() + 1);
    }

    if (wxStatusBar *sb = GetStatusBar()) {
        if (sb->IsShown()) {
            const int sb_h = std::max(sb->GetSize().GetHeight(), sb->GetBestSize().GetHeight());
            int cand = ch - sb_h;
            const int sb_y = sb->GetPosition().y;
            if (sb_y > top && sb_y <= ch) {
                cand = std::min(cand, sb_y);
            }
            bottom = std::min(bottom, cand);
        }
    }

    if (bottom < top) {
        bottom = top;
    }
    return wxRect(0, top, std::max(0, cw), bottom - top);
}

void MainFrame::RelayoutMainPanes() {
    if (!content_panel_) {
        return;
    }
    const wxRect area = ContentAreaRect();
    if (area.GetWidth() <= 0 || area.GetHeight() <= 0) {
        return;
    }
    if (content_panel_->GetRect() != area) {
        content_panel_->SetSize(area);
    }
    /* Keep chrome above content. */
    if (toolbar_) {
        toolbar_->Raise();
    }
    if (wxStatusBar *sb = GetStatusBar()) {
        sb->Raise();
    }
    content_panel_->Layout();
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
            int sash = splitter_->GetSashPosition();
            const int max_sash = std::max(min_pane, client_h - min_pane - 4);
            if (sash < min_pane) {
                sash = min_pane;
            }
            if (sash > max_sash) {
                sash = max_sash;
            }
            if (sash != splitter_->GetSashPosition()) {
                graph_split_sash_ = sash;
                splitter_->SetSashPosition(sash);
            }
        }
    }
    if (graph_ && graph_->IsShown()) {
        graph_->SendSizeEvent();
    }
}

void MainFrame::OnFrameSize(wxSizeEvent &evt) {
    evt.Skip();
    RelayoutMainPanes();
    if (toolbar_) {
        toolbar_->Refresh(true);
    }
    if (wxStatusBar *sb = GetStatusBar()) {
        sb->Refresh(true);
    }
}

void MainFrame::append_log(int level, const std::string &msg) {
    if (log_view_) {
        log_view_->Append(level, msg);
    }
}

void MainFrame::SetStatusBarMessage(const wxString &text, bool sticky) {
    SetStatusText(text);
    if (sticky) {
        sticky_status_ = text;
        sticky_status_index_ = engine_->current_index();
    } else {
        sticky_status_.clear();
        sticky_status_index_ = static_cast<size_t>(-1);
    }
}

bool MainFrame::OpenGptPasteFlow(const SopStep &step) {
    wxString err;
    if (!CopyStepPrompt(step, &err)) {
        SetStatusBarMessage(err, true);
        return false;
    }
    const std::string step_id = sop_step_id(step);
    SopPasteResponseDialog paste(this, engine_->options().project_dir, step);
    if (paste.ShowModal() != wxID_OK || !paste.saved()) {
        SetStatusBarMessage(wxString::FromUTF8("Paste Response cancelled."), true);
        return false;
    }
    auto applied = engine_->apply_gpt_save_result(step_id, paste.save_result());
    append_log(applied.ok ? 1 : 0, applied.message);
    if (!applied.ok) {
        wxMessageBox(wxString::FromUTF8(applied.message), wxString::FromUTF8("Save failed"),
                     wxOK | wxICON_ERROR, this);
        return false;
    }
    SetStatusBarMessage(wxString::FromUTF8(applied.message), true);
    if (step.interaction == SopInteraction::Select) {
        SopRenderResponseDialog render(this, paste.save_result());
        render.ShowModal();
    }
    return true;
}

bool MainFrame::MaybeOpenGptPasteForCurrent() {
    const SopStep *step = engine_->step_at(engine_->current_index());
    if (!step || !step->is_gpt_get()) {
        return false;
    }
    const std::string sid = engine_->current_step_id();
    if (engine_->is_complete(sid)) {
        return false;
    }
    const bool ok = OpenGptPasteFlow(*step);
    RefreshAll();
    return ok;
}

bool MainFrame::CopyStepPrompt(const SopStep &step, wxString *err) {
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

void MainFrame::RunStepAction(const std::string &step_id, bool force) {
    if (!engine_->definition().steps.count(step_id)) {
        return;
    }
    const SopStep &step = engine_->definition().steps.at(step_id);
    if (step.is_gpt_get()) {
        OpenGptPasteFlow(step);
        RefreshAll();
        return;
    }
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

void MainFrame::DoExecute(bool force) {
    if (engine_->current_index() >= engine_->active_steps().size()) {
        return;
    }
    RunStepAction(engine_->active_steps()[engine_->current_index()], force);
}

void MainFrame::DoCopy() {
    const SopStep *step = engine_->step_at(engine_->current_index());
    if (!step) {
        return;
    }
    if (step->is_gpt_get()) {
        OpenGptPasteFlow(*step);
        RefreshAll();
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

void MainFrame::RefreshProgressBar() {
    const size_t total = engine_->active_steps().size();
    const size_t current = engine_->current_index();
    int pct = 0;
    wxString label;
    std::string step_id;
    if (current < engine_->active_steps().size()) {
        step_id = engine_->active_steps()[current];
    }
    const double shell_pct =
        step_id.empty() ? -1.0 : engine_->shell_progress_pct(step_id);
    if (shell_pct >= 0.0 && engine_->is_shell_running(step_id)) {
        pct = static_cast<int>(shell_pct + 0.5);
        if (pct > 100) {
            pct = 100;
        }
        const std::string shell_label = engine_->shell_progress_label(step_id);
        if (!shell_label.empty()) {
            label = wxString::FromUTF8(shell_label);
        } else {
            label = wxString::Format("%.0f%%", shell_pct);
        }
    } else if (total > 0) {
        pct = static_cast<int>(current * 100 / total);
        label = wxString::Format("%zu / %zu", current + 1, total);
    } else {
        label = wxString::FromUTF8("0 / 0");
    }
    progress_gauge_->SetValue(pct);
    progress_label_->SetLabel(label);
    if (graph_layout_dirty_) {
        graph_->Rebuild();
        graph_layout_dirty_ = false;
    } else {
        graph_->SyncNodeStates();
    }
    graph_->SetCurrentIndex(current);
    const bool current_changed = step_id != last_current_step_id_;
    last_current_step_id_ = step_id;
    /* Auto-scroll only when the running step changes (Back/Next/Start/…),
     * and ScrollToStepId itself no-ops if the node is already in view. */
    if (current_changed && !step_id.empty()) {
        CallAfter([this, step_id]() {
            if (!graph_) {
                return;
            }
            graph_->ScrollToStepId(step_id, false);
        });
    }
}

void MainFrame::PreviewStep(const std::string &step_id) {
    const auto &def = engine_->definition();
    const auto it = def.steps.find(step_id);
    if (it == def.steps.end()) {
        return;
    }
    const SopStep &step = it->second;
    preview_mode_ = true;
    preview_step_id_ = step_id;

    role_icon_->SetBitmap(role_icon(step, wxSize(28, 28)));
    role_label_->SetLabel(wxString::Format(wxString::FromUTF8("%s  ·  %s"),
                                           wxString::FromUTF8(step_id),
                                           wxString::FromUTF8(step.role_label())));
    title_label_->SetLabel(wxString::FromUTF8(step.display_title()));
    desc_label_->SetLabel(wxString::FromUTF8(step.description));
    status_label_->SetLabel(status_text(step));
    status_label_->SetForegroundColour(status_colour(step));
    body_view_->SetValue(wxString::FromUTF8(step.body));
    ai_panel_->Show(step.is_user() && !step.is_gpt_get());
    Layout();
}

void MainFrame::RefreshStepView() {
    preview_mode_ = false;
    preview_step_id_.clear();
    const SopStep *step = engine_->step_at(engine_->current_index());
    if (!step) {
        return;
    }

    const std::string step_id = engine_->current_step_id();
    role_icon_->SetBitmap(role_icon(*step, wxSize(28, 28)));
    role_label_->SetLabel(wxString::Format(wxString::FromUTF8("%s  ·  %s"),
                                           wxString::FromUTF8(step_id),
                                           wxString::FromUTF8(step->role_label())));
    title_label_->SetLabel(wxString::FromUTF8(step->display_title()));
    desc_label_->SetLabel(wxString::FromUTF8(step->description));
    status_label_->SetLabel(status_text(*step));
    status_label_->SetForegroundColour(status_colour(*step));
    body_view_->SetValue(wxString::FromUTF8(step->body));
    ai_panel_->Show(step->is_user() && !step->is_gpt_get());
    Layout();
}

void MainFrame::RefreshStatusBar() {
    if (!sticky_status_.empty() && sticky_status_index_ == engine_->current_index()) {
        SetStatusText(sticky_status_);
        return;
    }
    sticky_status_.clear();
    sticky_status_index_ = static_cast<size_t>(-1);
    const SopStep *step = engine_->step_at(engine_->current_index());
    SetStatusText(status_bar_text(step, engine_));
}

void MainFrame::RefreshActionTool() {
    if (!toolbar_) {
        return;
    }
    const SopStep *step = engine_->step_at(engine_->current_index());
    const bool gpt_get = step && step->is_gpt_get();
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

    if (gpt_get) {
        toolbar_->InsertTool(pos, ID_COPY, wxString::FromUTF8("Get"),
                             wxArtProvider::GetBitmap(wxART_GO_DOWN, wxART_TOOLBAR), wxNullBitmap,
                             wxITEM_NORMAL,
                             wxString::FromUTF8("Get prompt + open Paste Response (Ctrl+Enter)"));
    } else if (copy_mode) {
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

void MainFrame::RefreshAutoRunTool() {
    if (!toolbar_) {
        return;
    }
    wxString label = wxString::FromUTF8("Run");
    wxArtID art = wxART_EXECUTABLE_FILE;
    switch (engine_->auto_run_state()) {
    case SopAutoRunState::Running:
        label = wxString::FromUTF8("Pause");
        art = wxART_STOP;
        break;
    case SopAutoRunState::Paused:
        label = wxString::FromUTF8("Resume");
        art = wxART_EXECUTABLE_FILE;
        break;
    default:
        break;
    }
    const wxString help = label + wxString::FromUTF8(" automated workflow (F5)");
    const int pos = toolbar_->GetToolPos(ID_AUTO_RUN);
    if (pos != wxNOT_FOUND) {
        toolbar_->DeleteTool(ID_AUTO_RUN);
        toolbar_->InsertTool(pos, ID_AUTO_RUN, label, wxArtProvider::GetBitmap(art, wxART_TOOLBAR), wxNullBitmap,
                             wxITEM_NORMAL, help);
        toolbar_->Realize();
    }
}

void MainFrame::RefreshNav() {
    if (toolbar_) {
        toolbar_->EnableTool(ID_BACK, engine_->current_index() > 0);
        toolbar_->EnableTool(ID_NEXT, engine_->can_advance());
    }
    RefreshAutoRunTool();
    RefreshActionTool();
}

