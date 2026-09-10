/*
 * Copyright (C) 2026 Lenik <soptools@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "gui/graph_canvas.hpp"
#include "gui/graph_style.hpp"

#include <wx/dcbuffer.h>
#include <wx/dcclient.h>
#include <wx/graphics.h>
#include <wx/artprov.h>
#include <wx/menu.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <vector>

#include "gui/graph_canvas_layout.hpp"

namespace {

wxMenuItem *AppendMenuItem(wxMenu *menu, int id, const wxString &label, const wxArtID &art) {
    auto *item = new wxMenuItem(menu, id, label);
    item->SetBitmap(wxArtProvider::GetBitmap(art, wxART_MENU, wxSize(16, 16)));
    menu->Append(item);
    return item;
}

wxMenuItem *AppendCheckMenuItem(wxMenu *menu, int id, const wxString &label, const wxArtID & /*art*/) {
    /* GTK rejects SetBitmap on wxITEM_CHECK (not an image menu item). */
    return menu->AppendCheckItem(id, label);
}

} /* namespace */

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
    std::vector<const SopGraphNode *> nodes{&node};
    if (center_in_view) {
        return PanToCenterNodes(nodes);
    }
    return PanToShowNodes(nodes, pan_);
}

wxPoint SopGraphCanvas::PanToShowNodes(const std::vector<const SopGraphNode *> &nodes, wxPoint pan) const {
    const wxSize client = GetClientSize();
    if (client.x <= 0 || client.y <= 0 || nodes.empty()) {
        return pan;
    }
    const int margin = 32;
    for (const SopGraphNode *node : nodes) {
        if (!node) {
            continue;
        }
        const int left = pan.x + static_cast<int>(node->pos.x * zoom_);
        const int top = pan.y + static_cast<int>(node->pos.y * zoom_);
        const int right = left + static_cast<int>(node->size.x * zoom_);
        const int bottom = top + static_cast<int>(node->size.y * zoom_);
        if (left < margin) {
            pan.x += margin - left;
        } else if (right > client.x - margin) {
            pan.x -= right - (client.x - margin);
        }
        if (top < margin) {
            pan.y += margin - top;
        } else if (bottom > client.y - margin) {
            pan.y -= bottom - (client.y - margin);
        }
    }
    return pan;
}

wxPoint SopGraphCanvas::PanToCenterNodes(const std::vector<const SopGraphNode *> &nodes) const {
    const wxSize client = GetClientSize();
    if (client.x <= 0 || client.y <= 0 || nodes.empty()) {
        return pan_;
    }
    int min_x = std::numeric_limits<int>::max();
    int min_y = std::numeric_limits<int>::max();
    int max_x = std::numeric_limits<int>::min();
    int max_y = std::numeric_limits<int>::min();
    for (const SopGraphNode *node : nodes) {
        if (!node) {
            continue;
        }
        min_x = std::min(min_x, node->pos.x);
        min_y = std::min(min_y, node->pos.y);
        max_x = std::max(max_x, node->pos.x + node->size.x);
        max_y = std::max(max_y, node->pos.y + node->size.y);
    }
    if (min_x > max_x || min_y > max_y) {
        return pan_;
    }
    const double cx = (static_cast<double>(min_x) + static_cast<double>(max_x)) / 2.0;
    const double cy = (static_cast<double>(min_y) + static_cast<double>(max_y)) / 2.0;
    return wxPoint(client.x / 2 - static_cast<int>(cx * zoom_),
                   client.y / 2 - static_cast<int>(cy * zoom_));
}

wxPoint SopGraphCanvas::ClampPanPoint(wxPoint pan) const {
    const wxSize client = GetClientSize();
    if (client.x < 1 || client.y < 1) {
        return pan;
    }
    const int margin = 8;
    const int gw = static_cast<int>(graph_size_.x * zoom_);
    const int gh = static_cast<int>(graph_size_.y * zoom_);
    if (gw + 2 * margin <= client.x) {
        pan.x = std::clamp(pan.x, margin, client.x - gw - margin);
    } else {
        pan.x = std::clamp(pan.x, client.x - gw - margin, margin);
    }
    if (gh + 2 * margin <= client.y) {
        pan.y = std::clamp(pan.y, margin, client.y - gh - margin);
    } else {
        pan.y = std::clamp(pan.y, client.y - gh - margin, margin);
    }
    return pan;
}

wxPoint SopGraphCanvas::FillHorizontalPan(wxPoint pan) const {
    const wxSize client = GetClientSize();
    if (client.x < 1) {
        return pan;
    }
    const int margin = 8;
    const int gw = static_cast<int>(graph_size_.x * zoom_);
    /* Prefer a filled viewport: no empty gutters when the graph is wider;
     * when narrower, pin left so empty space is only on the right. */
    if (gw + 2 * margin <= client.x) {
        pan.x = margin;
    } else {
        pan.x = std::clamp(pan.x, client.x - gw - margin, margin);
    }
    return pan;
}

std::vector<const SopGraphNode *> SopGraphCanvas::CollectFocusNodes(const std::string &step_id) const {
    std::vector<const SopGraphNode *> focus;
    for (const auto &node : nodes_) {
        if (node.selected) {
            focus.push_back(&node);
        }
    }
    if (!focus.empty()) {
        return focus;
    }
    for (const auto &node : nodes_) {
        if (node.step_id == step_id) {
            focus.push_back(&node);
            break;
        }
    }
    return focus;
}

std::vector<const SopGraphNode *> SopGraphCanvas::CollectVisibleNodes(wxPoint pan) const {
    std::vector<const SopGraphNode *> visible;
    for (const auto &node : nodes_) {
        if (IsNodeInViewport(node, pan)) {
            visible.push_back(&node);
        }
    }
    return visible;
}

bool SopGraphCanvas::AreNodesInViewport(const std::vector<const SopGraphNode *> &nodes) const {
    return AreNodesInViewport(nodes, pan_);
}

bool SopGraphCanvas::AreNodesInViewport(const std::vector<const SopGraphNode *> &nodes, wxPoint pan) const {
    if (nodes.empty()) {
        return true;
    }
    for (const SopGraphNode *node : nodes) {
        if (!node || !IsNodeInViewport(*node, pan)) {
            return false;
        }
    }
    return true;
}

void SopGraphCanvas::ReportStatus(const wxString &message) {
    if (status_fn_) {
        status_fn_(message);
    }
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
    const auto focus = CollectFocusNodes(step_id);
    if (focus.empty()) {
        return;
    }
    const wxSize client = GetClientSize();
    if (client.x < 20 || client.y < 20) {
        CallAfter([this, step_id, animated]() {
            CallAfter([this, step_id, animated]() { ScrollToStepId(step_id, animated); });
        });
        return;
    }
    /* Already fully on screen: keep current pan. */
    if (AreNodesInViewport(focus)) {
        return;
    }
    if (pan_anim_timer_.IsRunning()) {
        pan_anim_timer_.Stop();
    }
    /* Keep zoom fixed. visible(selection) → bbox(visible nodes) → center Y only;
     * X only nudges as needed, then fill so left/right are not left empty. */
    const wxPoint show_pan = PanToShowNodes(focus, pan_);
    auto visible = CollectVisibleNodes(show_pan);
    if (visible.empty()) {
        visible = focus;
    }
    wxPoint pan_target(show_pan.x, PanToCenterNodes(visible).y);
    if (!AreNodesInViewport(focus, pan_target)) {
        pan_target = PanToShowNodes(focus, pan_target);
    }
    pan_target = FillHorizontalPan(pan_target);
    pan_target = ClampPanPoint(pan_target);
    if (!AreNodesInViewport(focus, pan_target)) {
        pan_target = ClampPanPoint(PanToShowNodes(focus, pan_target));
        pan_target = FillHorizontalPan(pan_target);
        pan_target = ClampPanPoint(pan_target);
    }
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
    return IsNodeInViewport(node, pan_);
}

bool SopGraphCanvas::IsNodeInViewport(const SopGraphNode &node, wxPoint pan) const {
    const wxSize client = GetClientSize();
    if (client.x <= 0 || client.y <= 0) {
        return false;
    }
    const int margin = 8;
    const int left = pan.x + static_cast<int>(node.pos.x * zoom_);
    const int top = pan.y + static_cast<int>(node.pos.y * zoom_);
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
    if (pan_anim_timer_.IsRunning()) {
        pan_anim_timer_.Stop();
    }
    if (evt.ControlDown()) {
        const double old_zoom = zoom_;
        if (evt.GetWheelRotation() > 0) {
            zoom_ = std::min(2.5, zoom_ * 1.1);
        } else {
            zoom_ = std::max(0.35, zoom_ / 1.1);
        }
        const wxPoint mouse = evt.GetPosition();
        pan_.x = mouse.x - static_cast<int>((mouse.x - pan_.x) * (zoom_ / old_zoom));
        pan_.y = mouse.y - static_cast<int>((mouse.y - pan_.y) * (zoom_ / old_zoom));
        RelayoutIfNeeded();
    } else {
        /* Wheel pans vertically; delta scales with the OS wheel line size. */
        const int delta = evt.GetWheelRotation() * 40 / std::max(1, evt.GetWheelDelta());
        pan_.y += delta;
    }
    user_panned_ = true;
    ClampPan();
    Refresh(false);
}

void SopGraphCanvas::ShowNodeContextMenu(const wxPoint &screen_pos, const std::string &step_id) {
    context_step_id_ = step_id;
    const SopStep &step = engine_->definition().steps.at(step_id);
    wxMenu menu;
    auto *excluded = AppendCheckMenuItem(&menu, ID_CTX_EXCLUDED, wxString::FromUTF8("Excluded"), wxART_DELETE);
    excluded->Check(engine_->is_excluded(step_id));
    const SopGraphNode *node = nullptr;
    for (const auto &n : nodes_) {
        if (n.step_id == step_id) {
            node = &n;
            break;
        }
    }
    wxMenuItem *move_item =
        AppendMenuItem(&menu, ID_CTX_MOVE_HERE, wxString::FromUTF8("Move to here"), wxART_GOTO_FIRST);
    if (!node || !node->on_active_path) {
        move_item->Enable(false);
    }
    wxString cmd_label = wxString::FromUTF8("Execute");
    wxArtID cmd_art = wxART_EXECUTABLE_FILE;
    if (step.is_gpt_get()) {
        cmd_label = wxString::FromUTF8("Get");
        cmd_art = wxART_GO_DOWN;
    } else if (step.is_prompt_copy()) {
        cmd_label = wxString::FromUTF8("Copy");
        cmd_art = wxART_COPY;
    } else if (!step.is_automatable()) {
        cmd_label = wxString::FromUTF8("Run");
        cmd_art = wxART_GO_FORWARD;
    }
    AppendMenuItem(&menu, ID_CTX_EXECUTE, cmd_label, cmd_art);
    menu.AppendSeparator();
    AppendMenuItem(&menu, ID_REDRAW, wxString::FromUTF8("Redraw"), wxART_REFRESH);
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
    if (evt.GetKeyCode() == WXK_RETURN || evt.GetKeyCode() == WXK_NUMPAD_ENTER) {
        int idx = tab_focus_node_;
        if (idx < 0 && !selected_step_id_.empty()) {
            for (size_t i = 0; i < nodes_.size(); i++) {
                if (nodes_[i].step_id == selected_step_id_) {
                    idx = static_cast<int>(i);
                    break;
                }
            }
        }
        if (idx < 0) {
            idx = HitTestNode(ScreenToGraph(ScreenToClient(wxGetMousePosition())));
        }
        if (idx >= 0 && move_to_fn_) {
            const SopGraphNode &node = nodes_[static_cast<size_t>(idx)];
            selected_step_id_ = node.step_id;
            tab_focus_node_ = idx;
            for (auto &n : nodes_) {
                n.selected = n.step_id == selected_step_id_;
            }
            if (node.on_active_path) {
                move_to_fn_(node.step_id, node.path_index);
                ReportStatus(wxString::Format(wxString::FromUTF8("Location set to %s."),
                                              wxString::FromUTF8(node.step_id)));
            } else {
                ReportStatus(wxString::FromUTF8("Cannot set location: step is not on the active path."));
            }
            Refresh(false);
        }
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
        AppendMenuItem(&menu, ID_REDRAW, wxString::FromUTF8("Redraw"), wxART_REFRESH);
        PopupMenu(&menu, client);
    }
}

void SopGraphCanvas::OnRedraw(wxCommandEvent &) {
    RedrawGraph();
    ReportStatus(wxString::FromUTF8("Graph redrawn."));
}

void SopGraphCanvas::OnCtxExcluded(wxCommandEvent &evt) {
    if (context_step_id_.empty() || !exclude_fn_) {
        return;
    }
    exclude_fn_(context_step_id_, evt.IsChecked());
    ReportStatus(wxString::Format(wxString::FromUTF8("%s %s."),
                                  evt.IsChecked() ? wxString::FromUTF8("Excluded")
                                                  : wxString::FromUTF8("Included"),
                                  wxString::FromUTF8(context_step_id_)));
}

void SopGraphCanvas::OnCtxMoveHere(wxCommandEvent &) {
    if (context_step_id_.empty() || !move_to_fn_) {
        return;
    }
    for (const auto &node : nodes_) {
        if (node.step_id == context_step_id_ && node.on_active_path) {
            move_to_fn_(node.step_id, node.path_index);
            ReportStatus(wxString::Format(wxString::FromUTF8("Location set to %s."),
                                          wxString::FromUTF8(node.step_id)));
            return;
        }
    }
    ReportStatus(wxString::FromUTF8("Cannot set location: step is not on the active path."));
}

void SopGraphCanvas::OnCtxExecute(wxCommandEvent &) {
    if (context_step_id_.empty() || !execute_fn_) {
        return;
    }
    ReportStatus(wxString::Format(wxString::FromUTF8("Running %s…"),
                                  wxString::FromUTF8(context_step_id_)));
    execute_fn_(context_step_id_);
}
