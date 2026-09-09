/*
 * Copyright (C) 2026 Lenik <soptools@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "ui/gui/graph_canvas.hpp"
#include "ui/gui/graph_style.hpp"

#include <wx/dcbuffer.h>
#include <wx/dcclient.h>
#include <wx/graphics.h>
#include <wx/artprov.h>
#include <wx/menu.h>
#include <algorithm>
#include <cmath>
#include <map>

#include "ui/gui/graph_canvas_layout.hpp"


wxBEGIN_EVENT_TABLE(SopGraphCanvas, wxPanel)
    EVT_PAINT(SopGraphCanvas::OnPaint)
    EVT_SIZE(SopGraphCanvas::OnSize)
    EVT_MOUSEWHEEL(SopGraphCanvas::OnMouseWheel)
    EVT_LEFT_DOWN(SopGraphCanvas::OnMouseDown)
    EVT_LEFT_UP(SopGraphCanvas::OnMouseUp)
    EVT_MOTION(SopGraphCanvas::OnMouseMove)
    EVT_LEAVE_WINDOW(SopGraphCanvas::OnMouseLeave)
    EVT_ENTER_WINDOW(SopGraphCanvas::OnMouseEnter)
    EVT_KEY_DOWN(SopGraphCanvas::OnKeyDown)
    EVT_CONTEXT_MENU(SopGraphCanvas::OnContextMenu)
    EVT_MENU(SopGraphCanvas::ID_REDRAW, SopGraphCanvas::OnRedraw)
    EVT_MENU(SopGraphCanvas::ID_CTX_EXCLUDED, SopGraphCanvas::OnCtxExcluded)
    EVT_MENU(SopGraphCanvas::ID_CTX_EXECUTE, SopGraphCanvas::OnCtxExecute)
    EVT_MENU(SopGraphCanvas::ID_CTX_MOVE_HERE, SopGraphCanvas::OnCtxMoveHere)
wxEND_EVENT_TABLE()

SopGraphCanvas::SopGraphCanvas(wxWindow *parent, SopEngine *engine)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(-1, 200), wxBORDER_SIMPLE | wxCLIP_CHILDREN),
      engine_(engine),
      pan_anim_timer_(this) {
    SetBackgroundColour(SopGraphBgColour());
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetDoubleBuffered(true);
    SetMinSize(wxSize(-1, 120));
    SetFocus();
    pan_anim_timer_.Bind(wxEVT_TIMER, &SopGraphCanvas::OnPanAnimTimer, this);
    RedrawGraph();
}

void SopGraphCanvas::SetCurrentIndex(size_t index) {
    selected_step_id_.clear();
    for (auto &n : nodes_) {
        n.is_current = n.on_active_path && n.path_index == index;
        n.selected = false;
    }
    Refresh(false);
}

wxPoint SopGraphCanvas::PanToShowNode(const SopGraphNode &node, bool center_in_view) const {
    const wxSize client = GetClientSize();
    if (client.x <= 0 || client.y <= 0) {
        return pan_;
    }
    if (center_in_view) {
        const int target_x =
            (client.x - static_cast<int>(node.size.x * zoom_)) / 2 - static_cast<int>(node.pos.x * zoom_);
        const int target_y =
            (client.y - static_cast<int>(node.size.y * zoom_)) / 2 - static_cast<int>(node.pos.y * zoom_);
        return wxPoint(target_x, target_y);
    }
    const int margin = 32;
    const int left = pan_.x + static_cast<int>(node.pos.x * zoom_);
    const int top = pan_.y + static_cast<int>(node.pos.y * zoom_);
    const int right = left + static_cast<int>(node.size.x * zoom_);
    const int bottom = top + static_cast<int>(node.size.y * zoom_);

    int target_x = pan_.x;
    int target_y = pan_.y;
    if (left < margin) {
        target_x += margin - left;
    } else if (right > client.x - margin) {
        target_x -= right - (client.x - margin);
    }
    if (top < margin) {
        target_y += margin - top;
    } else if (bottom > client.y - margin) {
        target_y -= bottom - (client.y - margin);
    }
    return wxPoint(target_x, target_y);
}

void SopGraphCanvas::StartPanAnimation(const wxPoint &target) {
    if (target == pan_) {
        Refresh(false);
        return;
    }
    pan_anim_start_ = pan_;
    pan_anim_target_ = target;
    pan_anim_step_ = 0;
    pan_anim_timer_.Start(16);
}

void SopGraphCanvas::FinishPanAnimation() {
    pan_anim_timer_.Stop();
    pan_ = pan_anim_target_;
    pan_anim_step_ = kPanAnimSteps;
    Refresh(false);
}

void SopGraphCanvas::ScrollToStepId(const std::string &step_id, bool animated) {
    const SopGraphNode *target = nullptr;
    for (const auto &node : nodes_) {
        if (node.step_id == step_id) {
            target = &node;
            break;
        }
    }
    if (!target) {
        return;
    }
    const wxSize client = GetClientSize();
    if (client.x < 20 || client.y < 20) {
        CallAfter([this, step_id, animated]() {
            CallAfter([this, step_id, animated]() { ScrollToStepId(step_id, animated); });
        });
        return;
    }
    /* Already on screen: do not pan (click / in-view current step). */
    if (IsNodeInViewport(*target)) {
        return;
    }
    if (pan_anim_timer_.IsRunning()) {
        pan_anim_timer_.Stop();
    }
    const wxPoint pan_target = PanToShowNode(*target, false);
    if (pan_target == pan_) {
        return;
    }
    if (animated) {
        StartPanAnimation(pan_target);
    } else {
        pan_ = pan_target;
        Refresh(false);
    }
}

bool SopGraphCanvas::IsNodeInViewport(const SopGraphNode &node) const {
    const wxSize client = GetClientSize();
    if (client.x <= 0 || client.y <= 0) {
        return false;
    }
    const int margin = 8;
    const int left = pan_.x + static_cast<int>(node.pos.x * zoom_);
    const int top = pan_.y + static_cast<int>(node.pos.y * zoom_);
    const int right = left + static_cast<int>(node.size.x * zoom_);
    const int bottom = top + static_cast<int>(node.size.y * zoom_);
    return left >= margin && top >= margin && right <= client.x - margin && bottom <= client.y - margin;
}

void SopGraphCanvas::ScrollToCurrentNode(bool animated) {
    for (const auto &node : nodes_) {
        if (node.is_current) {
            ScrollToStepId(node.step_id, animated);
            return;
        }
    }
}

void SopGraphCanvas::OnPanAnimTimer(wxTimerEvent &) {
    pan_anim_step_++;
    const double t = std::min(1.0, static_cast<double>(pan_anim_step_) / kPanAnimSteps);
    const double ease = 1.0 - (1.0 - t) * (1.0 - t) * (1.0 - t);
    pan_.x = pan_anim_start_.x + static_cast<int>((pan_anim_target_.x - pan_anim_start_.x) * ease);
    pan_.y = pan_anim_start_.y + static_cast<int>((pan_anim_target_.y - pan_anim_start_.y) * ease);
    Refresh(false);
    if (pan_anim_step_ >= kPanAnimSteps) {
        FinishPanAnimation();
    }
}

void SopGraphCanvas::SetSelectedStep(const std::string &step_id) {
    selected_step_id_ = step_id;
    for (auto &n : nodes_) {
        n.selected = n.step_id == selected_step_id_;
    }
    Refresh(false);
}

void SopGraphCanvas::SyncNodeStates() {
    if (!engine_ || nodes_.empty()) {
        return;
    }
    const auto &order = engine_->active_steps();
    std::map<std::string, size_t> path_index;
    for (size_t i = 0; i < order.size(); i++) {
        path_index[order[i]] = i;
    }
    for (auto &node : nodes_) {
        node.included = engine_->is_included(node.step_id);
        node.on_active_path = path_index.count(node.step_id) > 0;
        node.started = engine_->is_started(node.step_id);
        node.is_current = node.on_active_path && path_index[node.step_id] == engine_->current_index();
        node.selected = node.step_id == selected_step_id_;
        if (node.on_active_path) {
            node.path_index = path_index[node.step_id];
        }
        const SopStep &step = engine_->definition().steps.at(node.step_id);
        node.show_check = step.is_action() && engine_->is_complete(node.step_id);
        node.show_error = step.status == SopStepStatus::Error;
    }
    Refresh(false);
}

void SopGraphCanvas::Rebuild() {
    RelayoutIfNeeded();
}

void SopGraphCanvas::RedrawGraph() {
    zoom_ = default_zoom_;
    user_panned_ = false;
    layout_cols_key_ = 0;
    RelayoutIfNeeded();
    CenterPan();
    ScrollToCurrentNode(false);
}

void SopGraphCanvas::OnSize(wxSizeEvent &evt) {
    evt.Skip();
    if (!engine_) {
        return;
    }
    /* Keep pan_.x/y so the graph stays visually fixed to the viewport left;
     * only column wrapping may change with width. */
    RelayoutIfNeeded();
    Refresh(false);
}

void SopGraphCanvas::OnMouseWheel(wxMouseEvent &evt) {
    const double old_zoom = zoom_;
    if (evt.GetWheelRotation() > 0) {
        zoom_ = std::min(2.5, zoom_ * 1.1);
    } else {
        zoom_ = std::max(0.35, zoom_ / 1.1);
    }
    const wxPoint mouse = evt.GetPosition();
    pan_.x = mouse.x - static_cast<int>((mouse.x - pan_.x) * (zoom_ / old_zoom));
    pan_.y = mouse.y - static_cast<int>((mouse.y - pan_.y) * (zoom_ / old_zoom));
    user_panned_ = true;
    RelayoutIfNeeded();
    ClampPan();
    Refresh(false);
}

void SopGraphCanvas::ShowNodeContextMenu(const wxPoint &screen_pos, const std::string &step_id) {
    context_step_id_ = step_id;
    const SopStep &step = engine_->definition().steps.at(step_id);
    wxMenu menu;
    menu.AppendCheckItem(ID_CTX_EXCLUDED, wxString::FromUTF8("Excluded"));
    menu.Check(ID_CTX_EXCLUDED, engine_->is_excluded(step_id));
    const SopGraphNode *node = nullptr;
    for (const auto &n : nodes_) {
        if (n.step_id == step_id) {
            node = &n;
            break;
        }
    }
    wxMenuItem *move_item =
        menu.Append(ID_CTX_MOVE_HERE, wxString::FromUTF8("Command: Move to here"));
    if (!node || !node->on_active_path) {
        move_item->Enable(false);
    }
    wxString cmd_label = wxString::FromUTF8("Command: Execute");
    if (step.is_prompt_copy()) {
        cmd_label = wxString::FromUTF8("Command: Copy");
    } else if (!step.is_automatable()) {
        cmd_label = wxString::FromUTF8("Command: Run");
    }
    menu.Append(ID_CTX_EXECUTE, cmd_label);
    menu.AppendSeparator();
    menu.Append(ID_REDRAW, wxString::FromUTF8("Command: Redraw"));
    PopupMenu(&menu, screen_pos);
}

void SopGraphCanvas::OnMouseDown(wxMouseEvent &evt) {
    SetFocus();
    if (evt.RightDown()) {
        return;
    }

    const wxPoint gpt = ScreenToGraph(evt.GetPosition());
    const int hit = HitTestNode(gpt);
    if (hit >= 0) {
        const SopGraphNode &node = nodes_[static_cast<size_t>(hit)];
        const auto &def = engine_->definition();
        const auto git = def.branch_groups.find(node.seq);
        if (git != def.branch_groups.end() && git->second.step_ids.size() > 1 && branch_activate_fn_) {
            const bool inactive = engine_->is_excluded(node.step_id) ||
                                  (git->second.kind == SopBranchKind::Select && !node.on_active_path);
            if (inactive) {
                branch_activate_fn_(node.seq, node.step_id);
                return;
            }
        }
        selected_step_id_ = node.step_id;
        tab_focus_node_ = hit;
        for (auto &n : nodes_) {
            n.selected = n.step_id == selected_step_id_;
        }
        if (step_select_) {
            step_select_(node.step_id);
        }
        Refresh(false);
        return;
    }

    panning_ = true;
    user_panned_ = true;
    if (pan_anim_timer_.IsRunning()) {
        pan_anim_timer_.Stop();
    }
    pan_start_ = evt.GetPosition();
    pan_origin_ = pan_;
    CaptureMouse();
    SetCursor(wxCursor(wxCURSOR_HAND));
}

void SopGraphCanvas::OnMouseUp(wxMouseEvent &) {
    if (panning_) {
        panning_ = false;
        if (HasCapture()) {
            ReleaseMouse();
        }
        SetCursor(wxCursor(wxCURSOR_ARROW));
    }
}

void SopGraphCanvas::OnMouseMove(wxMouseEvent &evt) {
    if (panning_) {
        pan_ = pan_origin_ + (evt.GetPosition() - pan_start_);
        ClampPan();
        Refresh(false);
        return;
    }
    evt.SetEventObject(this);
    UpdateHover(HitTestNode(ScreenToGraph(evt.GetPosition())));
}

void SopGraphCanvas::OnMouseEnter(wxMouseEvent &evt) {
    UpdateHover(HitTestNode(ScreenToGraph(evt.GetPosition())));
}

void SopGraphCanvas::OnMouseLeave(wxMouseEvent &) {
    const wxPoint client = ScreenToClient(wxGetMousePosition());
    if (GetClientRect().Contains(client)) {
        UpdateHover(HitTestNode(ScreenToGraph(client)));
        return;
    }
    UpdateHover(-1);
}

void SopGraphCanvas::OnKeyDown(wxKeyEvent &evt) {
    if (nodes_.empty()) {
        return;
    }
    if (evt.GetKeyCode() == WXK_TAB) {
        if (tab_focus_node_ < 0) {
            tab_focus_node_ = 0;
        } else {
            tab_focus_node_ = (tab_focus_node_ + (evt.ShiftDown() ? -1 : 1) + static_cast<int>(nodes_.size())) %
                              static_cast<int>(nodes_.size());
        }
        for (size_t i = 0; i < nodes_.size(); i++) {
            nodes_[i].hovered = static_cast<int>(i) == tab_focus_node_;
        }
        Refresh(false);
        evt.Skip();
        return;
    }
    if (evt.GetKeyCode() == ' ') {
        int idx = tab_focus_node_ >= 0 ? tab_focus_node_ : HitTestNode(ScreenToGraph(ScreenToClient(wxGetMousePosition())));
        if (idx < 0 && !engine_->active_steps().empty()) {
            const std::string &cur = engine_->active_steps()[engine_->current_index()];
            for (size_t i = 0; i < nodes_.size(); i++) {
                if (nodes_[i].step_id == cur) {
                    idx = static_cast<int>(i);
                    break;
                }
            }
        }
        if (idx >= 0) {
            const SopGraphNode &node = nodes_[static_cast<size_t>(idx)];
            selected_step_id_ = node.step_id;
            tab_focus_node_ = idx;
            for (auto &n : nodes_) {
                n.selected = n.step_id == selected_step_id_;
                n.hovered = false;
            }
            if (step_select_) {
                step_select_(node.step_id);
            }
            Refresh(false);
        }
        return;
    }
    evt.Skip();
}

void SopGraphCanvas::OnContextMenu(wxContextMenuEvent &evt) {
    wxPoint client;
    const wxPoint pos = evt.GetPosition();
    if (pos.x == -1 && pos.y == -1) {
        client = wxPoint(GetClientSize().x / 2, GetClientSize().y / 2);
    } else {
        client = ScreenToClient(pos);
    }
    const int hit = HitTestNode(ScreenToGraph(client));
    if (hit >= 0) {
        ShowNodeContextMenu(client, nodes_[static_cast<size_t>(hit)].step_id);
    } else {
        wxMenu menu;
        menu.Append(ID_REDRAW, wxString::FromUTF8("Redraw"));
        PopupMenu(&menu, client);
    }
}

void SopGraphCanvas::OnRedraw(wxCommandEvent &) {
    RedrawGraph();
}

void SopGraphCanvas::OnCtxExcluded(wxCommandEvent &evt) {
    if (context_step_id_.empty() || !exclude_fn_) {
        return;
    }
    exclude_fn_(context_step_id_, evt.IsChecked());
}

void SopGraphCanvas::OnCtxMoveHere(wxCommandEvent &) {
    if (context_step_id_.empty() || !move_to_fn_) {
        return;
    }
    for (const auto &node : nodes_) {
        if (node.step_id == context_step_id_ && node.on_active_path) {
            move_to_fn_(node.step_id, node.path_index);
            return;
        }
    }
}

void SopGraphCanvas::OnCtxExecute(wxCommandEvent &) {
    if (context_step_id_.empty() || !execute_fn_) {
        return;
    }
    execute_fn_(context_step_id_);
}
