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
#include "gui/codex_copy_dialog.hpp"
#include "engine/project.hpp"
#include "engine/stage_files.hpp"
#include "engine/agents.hpp"
#include "engine/gpt_response.hpp"
#include "model/step_interact.hpp"

#include <wx/wx.h>
#include <wx/artprov.h>
#include <wx/filedlg.h>
#include <wx/textdlg.h>
#include <wx/scrolwin.h>
#include <wx/statline.h>
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/radiobox.h>
#include <wx/choicdlg.h>

#include "config.h"

#include <fstream>
#include <sstream>
#include <functional>
#include <thread>
#include <algorithm>

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

void MainFrame::SetAgentBusy(bool busy) {
    agent_busy_ = busy;
    if (toolbar_) {
        toolbar_->EnableTool(ID_MAKE, !busy);
        toolbar_->EnableTool(ID_EXECUTE, !busy);
        toolbar_->EnableTool(ID_START_RESUME, !busy);
        toolbar_->EnableTool(ID_BACK, !busy);
    }
    RefreshRunTools();
    RefreshNav();
}

bool MainFrame::ResolveStepInteractions(const SopStep &step, std::string *resolved_body) {
    const std::string prior = find_nearest_chat_response(
        engine_->definition(), engine_->active_steps(), engine_->current_index(),
        engine_->options().project_dir);
    auto selects = parse_selects(step.body);
    auto optionals = parse_optionals(step.body);
    if (selects.empty() && optionals.empty()) {
        *resolved_body = expand_response_token(step.body, prior);
        return true;
    }

    /* Document order: ask in start-offset order. */
    struct Ask {
        bool is_select = false;
        size_t index = 0;
        size_t start = 0;
    };
    std::vector<Ask> asks;
    for (size_t i = 0; i < selects.size(); i++) {
        asks.push_back(Ask{true, i, selects[i].start});
    }
    for (size_t i = 0; i < optionals.size(); i++) {
        asks.push_back(Ask{false, i, optionals[i].start});
    }
    std::sort(asks.begin(), asks.end(),
              [](const Ask &a, const Ask &b) { return a.start < b.start; });

    std::vector<int> select_answers(selects.size(), 0);
    std::vector<bool> optional_include(optionals.size(), false);

    for (const Ask &ask : asks) {
        if (ask.is_select) {
            const auto &sel = selects[ask.index];
            wxArrayString choices;
            for (const auto &opt : sel.options) {
                const std::string shown = expand_response_token(opt.text, prior);
                const wxString label = wxString::FromUTF8(
                    shown.size() > 120 ? shown.substr(0, 117) + "..." : shown);
                choices.Add(label);
            }
            if (choices.empty()) {
                select_answers[ask.index] = 0;
                continue;
            }
            wxSingleChoiceDialog dlg(this, wxString::FromUTF8(sel.title),
                                     wxString::FromUTF8("Step interaction"), choices);
            dlg.SetSelection(0);
            if (dlg.ShowModal() != wxID_OK) {
                return false;
            }
            select_answers[ask.index] = dlg.GetSelection();
        } else {
            const auto &opt = optionals[ask.index];
            const wxString preview = wxString::FromUTF8(
                opt.body.size() > 400 ? opt.body.substr(0, 397) + "..." : opt.body);
            const int rc = wxMessageBox(
                preview.empty() ? wxString::FromUTF8("Include this optional section in the prompt?")
                                : preview,
                wxString::FromUTF8(opt.title), wxYES_NO | wxCANCEL | wxICON_QUESTION, this);
            if (rc == wxCANCEL) {
                return false;
            }
            optional_include[ask.index] = (rc == wxYES);
        }
    }

    *resolved_body = apply_step_interactions(step.body, selects, select_answers, optionals,
                                             optional_include, prior);
    return true;
}

bool MainFrame::RunAgentFlow(const SopStep &step) {
    if (agent_busy_) {
        SetStatusBarMessage(wxString::FromUTF8("An agent is already running."), true);
        return false;
    }
    std::string prompt;
    if (!ResolveStepInteractions(step, &prompt)) {
        SetStatusBarMessage(wxString::FromUTF8("Step interaction cancelled."), true);
        return false;
    }

    const SopAgents agents = load_agents(engine_->options().project_dir);
    const std::string cmdline = agent_cmdline_for_role(agents, step.role_slug);
    if (cmdline.empty()) {
        SetStatusBarMessage(wxString::FromUTF8("No agent configured for this role."), true);
        return false;
    }

    const std::string step_id = sop_step_id(step);
    agent_step_id_ = step_id;
    SetAgentBusy(true);
    SetStatusBarMessage(wxString::FromUTF8("Running agent…"));
    append_log(1, "agent: " + cmdline);

    const std::string project_dir = engine_->options().project_dir;
    const std::string sop_dir = engine_->options().sop_dir;
    const int loglevel = engine_->options().verbose;
    const bool is_chat = step.is_chat_get() || step.role == SopRole::Chat;
    const bool is_code = step.is_code();
    const SopInteraction interaction = step.interaction;
    SopStep step_copy = step;

    std::thread([this, step_id, cmdline, prompt, project_dir, sop_dir, loglevel, is_chat, is_code,
                 interaction, step_copy]() {
        std::string agent_log;
        auto result = run_agent_blocking(project_dir, sop_dir, step_copy, cmdline, prompt, loglevel,
                                         nullptr, nullptr, &agent_log);
        CallAfter([this, step_id, result, agent_log, is_chat, is_code, interaction, step_copy]() {
            if (!agent_log.empty()) {
                append_log(2, agent_log);
            }
            if (!result.ok) {
                SetStatusBarMessage(wxString::FromUTF8(result.message), true);
                append_log(0, result.message);
                SetAgentBusy(false);
                agent_step_id_.clear();
                RefreshAll();
                return;
            }
            if (is_chat) {
                std::vector<SopGptAttachment> empty;
                auto saved =
                    save_gpt_response(engine_->options().project_dir, step_copy, result.response_text, empty);
                if (!saved.ok && result.response_text.empty()) {
                    /* confirm-style empty ok for stage-only */
                    engine_->mark_complete(step_id, true);
                    SetStatusBarMessage(wxString::FromUTF8("Agent finished (no response body)."), true);
                } else {
                    auto applied = engine_->apply_gpt_save_result(step_id, saved);
                    append_log(applied.ok ? 1 : 0, applied.message);
                    SetStatusBarMessage(wxString::FromUTF8(applied.message), true);
                    if (applied.ok && interaction == SopInteraction::Select) {
                        SopRenderResponseDialog render(this, saved);
                        render.ShowModal();
                    }
                }
            } else if (is_code) {
                engine_->mark_complete(step_id, true);
                SetStatusBarMessage(wxString::FromUTF8("Code agent finished."), true);
                append_log(1, "code agent complete");
            } else {
                engine_->mark_complete(step_id, true);
                SetStatusBarMessage(wxString::FromUTF8(result.message), true);
            }
            SetAgentBusy(false);
            agent_step_id_.clear();
            RefreshAll();
        });
    }).detach();
    return true;
}

bool MainFrame::MaybeOpenChatAgentForCurrent() {
    const SopStep *step = engine_->step_at(engine_->current_index());
    if (!step || !step->is_chat_get()) {
        return false;
    }
    const std::string sid = engine_->current_step_id();
    if (engine_->is_complete(sid)) {
        return false;
    }
    const bool ok = RunAgentFlow(*step);
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
    if (agent_busy_) {
        return;
    }
    if (!engine_->definition().steps.count(step_id)) {
        return;
    }
    const SopStep &step = engine_->definition().steps.at(step_id);
    if (step.is_chat_get() || step.is_code() || step.is_prompt_copy()) {
        RunAgentFlow(step);
        RefreshAll();
        return;
    }
    if (step.is_user()) {
        SetStatusBarMessage(wxString::FromUTF8("Complete this user step, then Make."));
        append_log(1, "user step: complete manually, then Make");
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
    if (step->is_chat_get() || step->is_code()) {
        RunAgentFlow(*step);
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

    wxString label = wxString::FromUTF8("Execute");
    wxArtID art = wxART_EXECUTABLE_FILE;
    wxString help = wxString::FromUTF8("Run this step and stay here (Ctrl+Enter)");
    wxString menu_label = wxString::FromUTF8("&Execute\tCtrl-Enter");

    if (step && step->is_gpt_get()) {
        label = wxString::FromUTF8("Get");
        art = wxART_GO_DOWN;
        help = wxString::FromUTF8("Copy prompt and open Paste Response (Ctrl+Enter)");
        menu_label = wxString::FromUTF8("&Get\tCtrl-Enter");
    } else if (step && step->is_codex()) {
        label = wxString::FromUTF8("Code");
        art = wxART_COPY;
        help = wxString::FromUTF8("Run code agent (Ctrl+Enter)");
        menu_label = wxString::FromUTF8("&Code\tCtrl-Enter");
    }

    int pos = toolbar_->GetToolPos(ID_EXECUTE);
    if (pos == wxNOT_FOUND) {
        pos = static_cast<int>(toolbar_->GetToolsCount());
    } else {
        toolbar_->DeleteTool(ID_EXECUTE);
    }
    /* Drop legacy ID_COPY tool if a prior build left it. */
    if (toolbar_->GetToolPos(ID_COPY) != wxNOT_FOUND) {
        toolbar_->DeleteTool(ID_COPY);
    }

    toolbar_->InsertTool(pos, ID_EXECUTE, label, wxArtProvider::GetBitmap(art, wxART_TOOLBAR), wxNullBitmap,
                         wxITEM_NORMAL, help);
    toolbar_->Realize();

    if (action_menu_item_) {
        action_menu_item_->SetItemLabel(menu_label);
    }
}

void MainFrame::RefreshRunTools() {
    if (!toolbar_) {
        return;
    }
    const bool running = engine_->auto_run_state() == SopAutoRunState::Running;
    const std::string sid = engine_->current_step_id();
    const bool shell_running = !sid.empty() && engine_->is_shell_running(sid);
    toolbar_->EnableTool(ID_START_RESUME, !running);
    toolbar_->EnableTool(ID_PAUSE, running);
    toolbar_->EnableTool(ID_STOP, shell_running);
    if (GetMenuBar()) {
        GetMenuBar()->Enable(ID_START_RESUME, !running);
        GetMenuBar()->Enable(ID_PAUSE, running);
        GetMenuBar()->Enable(ID_STOP, shell_running);
    }
}

void MainFrame::RefreshNav() {
    if (toolbar_) {
        const bool has_step = engine_->current_index() < engine_->active_steps().size();
        const bool enable = has_step && !agent_busy_;
        toolbar_->EnableTool(ID_MAKE, enable);
        toolbar_->EnableTool(ID_EXECUTE, !agent_busy_);
        toolbar_->EnableTool(ID_START_RESUME, !agent_busy_ && engine_->auto_run_state() != SopAutoRunState::Running);
        if (GetMenuBar()) {
            GetMenuBar()->Enable(ID_MAKE, enable);
            GetMenuBar()->Enable(ID_EXECUTE, !agent_busy_);
            GetMenuBar()->Enable(ID_BACK, !agent_busy_ && engine_->current_index() > 0);
            GetMenuBar()->Enable(ID_START_RESUME, !agent_busy_);
        }
    }
    RefreshRunTools();
    RefreshActionTool();
}
