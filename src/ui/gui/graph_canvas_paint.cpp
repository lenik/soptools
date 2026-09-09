/*
 * Copyright (C) 2026 Lenik <soptools@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "ui/gui/graph_canvas.hpp"
#include "ui/gui/graph_canvas_layout.hpp"
#include "ui/gui/graph_style.hpp"

#include <wx/dcbuffer.h>
#include <wx/dcclient.h>
#include <wx/graphics.h>
#include <wx/artprov.h>

#include <algorithm>
#include <cmath>
#include <map>

int SopGraphCanvas::LayoutColumnsPerRow() const {
    int client_w = GetClientSize().x;
    if (client_w <= 0 && GetParent()) {
        client_w = GetParent()->GetClientSize().x - 24;
    }
    if (client_w <= 0) {
        client_w = 800;
    }
    /* Use branch column pitch so wrapping leaves room for bent edges. */
    const int col_pitch = kMaxNodeW + kBranchColGap;
    const int usable = static_cast<int>(client_w / zoom_) - kMarginX * 2;
    return std::max(1, usable / col_pitch);
}

int SopGraphCanvas::LayoutColumnsKey() const {
    const int cols = LayoutColumnsPerRow();
    return cols * 1000 + static_cast<int>(zoom_ * 100);
}

void SopGraphCanvas::RelayoutIfNeeded() {
    const int key = LayoutColumnsKey();
    if (key == layout_cols_key_ && !nodes_.empty()) {
        return;
    }
    LayoutGraph();
    layout_cols_key_ = key;
}

void SopGraphCanvas::CenterPan() {
    const wxSize client = GetClientSize();
    if (client.x <= 0 || client.y <= 0) {
        pan_ = wxPoint(0, 0);
        return;
    }
    pan_.x = (client.x - static_cast<int>(graph_size_.x * zoom_)) / 2;
    pan_.y = (client.y - static_cast<int>(graph_size_.y * zoom_)) / 2;
}

void SopGraphCanvas::ClampPan() {
    const wxSize client = GetClientSize();
    if (client.x < 1 || client.y < 1) {
        return;
    }

    const int margin = 8;
    const int gw = static_cast<int>(graph_size_.x * zoom_);
    const int gh = static_cast<int>(graph_size_.y * zoom_);

    if (gw + 2 * margin <= client.x) {
        pan_.x = std::clamp(pan_.x, margin, client.x - gw - margin);
    } else {
        pan_.x = std::clamp(pan_.x, client.x - gw - margin, margin);
    }

    if (gh + 2 * margin <= client.y) {
        pan_.y = std::clamp(pan_.y, margin, client.y - gh - margin);
    } else {
        pan_.y = std::clamp(pan_.y, client.y - gh - margin, margin);
    }
}

std::vector<wxPoint> SopGraphCanvas::RouteEdge(const wxPoint &from, const wxPoint &to, int from_row,
                                               int to_row, bool is_fork) const {
    (void)from_row;
    (void)to_row;
    (void)is_fork;
    return RouteBetweenPorts(from, to);
}

void SopGraphCanvas::LayoutGraph() {
    nodes_.clear();
    edges_.clear();
    if (!engine_) {
        return;
    }

    wxClientDC dc(this);
    dc.SetFont(NodeFont(GetFont()));

    cols_per_row_ = LayoutColumnsPerRow();

    const SopDefinition &def = engine_->definition();
    const auto &order = engine_->active_steps();

    std::map<std::string, size_t> path_index;
    for (size_t i = 0; i < order.size(); i++) {
        path_index[order[i]] = i;
    }

    struct SeqCol {
        int seq = 0;
        int flow_index = 0;
        int wrap_row = 0;
        int wrap_col = 0;
        int col_w = kMinNodeW;
        int col_h = 0;
        int x = 0;
        wxRect box;
        wxPoint port_left;
        wxPoint port_right;
        std::vector<std::string> step_ids;
        std::map<std::string, NodeMeasure> measures;
        std::vector<SopGraphNode *> nodes;
    };
    std::vector<SeqCol> columns;
    int flow_index = 0;
    for (int seq : def.seq_order) {
        const auto git = def.branch_groups.find(seq);
        if (git == def.branch_groups.end()) {
            continue;
        }
        SeqCol col;
        col.seq = seq;
        col.flow_index = flow_index++;
        col.wrap_row = col.flow_index / cols_per_row_;
        col.wrap_col = col.flow_index % cols_per_row_;
        col.step_ids = git->second.step_ids;
        for (const std::string &id : col.step_ids) {
            const NodeMeasure m = MeasureNode(def.steps.at(id), id, engine_, dc);
            col.measures[id] = m;
            col.col_w = std::max(col.col_w, m.size.x);
            col.col_h += m.size.y;
        }
        if (col.step_ids.size() > 1) {
            col.col_h += static_cast<int>(col.step_ids.size() - 1) * kBranchGap;
        }
        columns.push_back(col);
    }

    std::map<int, std::vector<SeqCol *>> row_cols;
    for (SeqCol &col : columns) {
        row_cols[col.wrap_row].push_back(&col);
    }
    for (auto &kv : row_cols) {
        std::sort(kv.second.begin(), kv.second.end(),
                  [](const SeqCol *a, const SeqCol *b) { return a->wrap_col < b->wrap_col; });
        int x = kMarginX;
        for (size_t i = 0; i < kv.second.size(); i++) {
            SeqCol *col = kv.second[i];
            col->x = x;
            const bool branch_col = col->step_ids.size() > 1;
            const bool next_branch =
                i + 1 < kv.second.size() && kv.second[i + 1]->step_ids.size() > 1;
            const int gap = (branch_col || next_branch) ? kBranchColGap : kColGap;
            x += col->col_w + gap;
        }
    }

    std::map<int, int> row_block_h;
    for (const SeqCol &col : columns) {
        row_block_h[col.wrap_row] = std::max(row_block_h[col.wrap_row], col.col_h + 8);
    }

    std::map<int, int> row_y;
    int y_cursor = kMarginY;
    for (const auto &kv : row_block_h) {
        row_y[kv.first] = y_cursor;
        y_cursor += kv.second + kRowGap;
    }

    for (const SeqCol &col : columns) {
        const int count = static_cast<int>(col.step_ids.size());
        const int base_y = row_y[col.wrap_row];
        const int block_h = row_block_h[col.wrap_row];

        int stack_h = 0;
        for (const std::string &id : col.step_ids) {
            stack_h += col.measures.at(id).size.y;
        }
        if (count > 1) {
            stack_h += (count - 1) * kBranchGap;
        }
        int y = base_y + (block_h - stack_h) / 2;

        for (int i = 0; i < count; i++) {
            const std::string &id = col.step_ids[static_cast<size_t>(i)];
            const NodeMeasure &m = col.measures.at(id);

            SopGraphNode node;
            node.step_id = id;
            node.seq = col.seq;
            node.wrap_row = col.wrap_row;
            node.wrap_col = col.wrap_col;
            node.pos = wxPoint(col.x, y);
            node.size = m.size;
            node.title_text = m.title_text;
            node.show_check = m.show_check;
            node.show_error = m.show_error;
            node.included = engine_->is_included(id);
            node.on_active_path = path_index.count(id) > 0;
            node.started = engine_->is_started(id);
            node.is_current = node.on_active_path && path_index[id] == engine_->current_index();
            node.selected = id == selected_step_id_;
            if (node.on_active_path) {
                node.path_index = path_index[id];
            }
            nodes_.push_back(node);

            y += m.size.y + kBranchGap;
        }
    }

    std::map<std::string, SopGraphNode *> node_by_id;
    for (auto &node : nodes_) {
        node_by_id[node.step_id] = &node;
    }
    for (SeqCol &col : columns) {
        col.nodes.clear();
        for (const std::string &id : col.step_ids) {
            const auto it = node_by_id.find(id);
            if (it != node_by_id.end()) {
                col.nodes.push_back(it->second);
            }
        }
    }

    /* Column = bounding box + padding; left/right mid are the external ports. */
    for (SeqCol &col : columns) {
        if (col.nodes.empty()) {
            continue;
        }
        int left = col.nodes.front()->pos.x;
        int top = col.nodes.front()->pos.y;
        int right = left + col.nodes.front()->size.x;
        int bottom = top + col.nodes.front()->size.y;
        for (const SopGraphNode *n : col.nodes) {
            left = std::min(left, n->pos.x);
            top = std::min(top, n->pos.y);
            right = std::max(right, n->pos.x + n->size.x);
            bottom = std::max(bottom, n->pos.y + n->size.y);
        }
        col.box = wxRect(left - kColBoxPad, top - kColBoxPad, (right - left) + 2 * kColBoxPad,
                         (bottom - top) + 2 * kColBoxPad);
        col.port_left = wxPoint(col.box.GetLeft(), col.box.GetTop() + col.box.GetHeight() / 2);
        col.port_right = wxPoint(col.box.GetRight(), col.box.GetTop() + col.box.GetHeight() / 2);
    }

    /* Column-to-column: same row = direct; wrap = right-down-left-down-right. */
    std::map<int, int> row_right_x;
    std::map<int, int> row_left_x;
    for (const SeqCol &col : columns) {
        if (col.nodes.empty()) {
            continue;
        }
        const int left = col.box.GetLeft();
        const int right = col.box.GetRight();
        if (!row_left_x.count(col.wrap_row) || left < row_left_x[col.wrap_row]) {
            row_left_x[col.wrap_row] = left;
        }
        if (!row_right_x.count(col.wrap_row) || right > row_right_x[col.wrap_row]) {
            row_right_x[col.wrap_row] = right;
        }
    }

    int wrap_left_extent = kMarginX;
    int wrap_right_extent = kMarginX;
    for (size_t i = 0; i + 1 < columns.size(); i++) {
        const SeqCol &a = columns[i];
        const SeqCol &b = columns[i + 1];
        if (a.nodes.empty() || b.nodes.empty()) {
            continue;
        }
        if (a.wrap_row == b.wrap_row) {
            AppendEdge(edges_, RouteBetweenPorts(a.port_right, b.port_left), true, false, true);
            continue;
        }
        const int gutter_y = row_y[a.wrap_row] + row_block_h[a.wrap_row] + kRowGap / 2;
        const int a_right = row_right_x.count(a.wrap_row) ? row_right_x[a.wrap_row] : a.box.GetRight();
        const int b_left = row_left_x.count(b.wrap_row) ? row_left_x[b.wrap_row] : b.box.GetLeft();
        const auto pts = RouteWrapToNextRow(a.port_right, b.port_left, a_right, b_left, gutter_y);
        for (const wxPoint &p : pts) {
            wrap_left_extent = std::min(wrap_left_extent, p.x);
            wrap_right_extent = std::max(wrap_right_extent, p.x);
        }
        AppendEdge(edges_, pts, true, false, true);
    }

    /* Inside a branch column: box ports fan in/out to each node. */
    for (const SeqCol &col : columns) {
        if (col.nodes.size() <= 1) {
            continue;
        }
        for (const SopGraphNode *node : col.nodes) {
            const bool active = node->on_active_path;
            const wxPoint left = NodeLeftPort(node->pos, node->size);
            const wxPoint right = NodeRightPort(node->pos, node->size);
            AppendEdge(edges_, RouteHubToPort(col.port_left, left, true), active, true, active);
            AppendEdge(edges_, RouteHubToPort(col.port_right, right, false), active, true, false);
        }
    }

    int max_x = kMarginX;
    for (const SeqCol &col : columns) {
        max_x = std::max(max_x, col.x + col.col_w);
    }
    max_x = std::max(max_x, wrap_right_extent);

    graph_size_.x = max_x + kMarginX + kWrapGutter;
    /* Leave room if wrap arm goes left of the normal margin. */
    if (wrap_left_extent < 0) {
        const int shift = -wrap_left_extent + kWrapGutter;
        for (auto &node : nodes_) {
            node.pos.x += shift;
        }
        for (auto &edge : edges_) {
            for (auto &p : edge.points) {
                p.x += shift;
            }
        }
        graph_size_.x += shift;
    }
    graph_size_.y = y_cursor + kMarginY;
    if (graph_size_.y < 140) {
        graph_size_.y = 140;
    }
}

wxPoint SopGraphCanvas::ScreenToGraph(const wxPoint &screen) const {
    const double x = (screen.x - pan_.x) / zoom_;
    const double y = (screen.y - pan_.y) / zoom_;
    return wxPoint(static_cast<int>(x), static_cast<int>(y));
}

int SopGraphCanvas::HitTestNode(const wxPoint &graph_pt) const {
    for (int i = static_cast<int>(nodes_.size()) - 1; i >= 0; i--) {
        const SopGraphNode &n = nodes_[static_cast<size_t>(i)];
        wxRect r(n.pos.x - kHitPad, n.pos.y - kHitPad, n.size.x + kHitPad * 2, n.size.y + kHitPad * 2);
        if (r.Contains(graph_pt)) {
            return i;
        }
    }
    return -1;
}

wxRect SopGraphCanvas::NodeScreenRect(const SopGraphNode &node) const {
    const wxRect graph_rect(node.pos, node.size);
    return wxRect(pan_.x + static_cast<int>(graph_rect.x * zoom_) - kHitPad,
                  pan_.y + static_cast<int>(graph_rect.y * zoom_) - kHitPad,
                  static_cast<int>(graph_rect.width * zoom_) + kHitPad * 2 + 2,
                  static_cast<int>(graph_rect.height * zoom_) + kHitPad * 2 + 2);
}

void SopGraphCanvas::UpdateHover(int node_index) {
    if (hover_node_ == node_index) {
        return;
    }
    if (hover_node_ >= 0 && hover_node_ < static_cast<int>(nodes_.size())) {
        RefreshRect(NodeScreenRect(nodes_[static_cast<size_t>(hover_node_)]), false);
    }
    hover_node_ = node_index;
    for (size_t i = 0; i < nodes_.size(); i++) {
        nodes_[i].hovered = static_cast<int>(i) == hover_node_;
    }
    if (hover_node_ >= 0 && hover_node_ < static_cast<int>(nodes_.size())) {
        RefreshRect(NodeScreenRect(nodes_[static_cast<size_t>(hover_node_)]), false);
    }
}

void SopGraphCanvas::DrawNode(wxGraphicsContext *gc, const SopGraphNode &node) {
    const SopStep &step = engine_->definition().steps.at(node.step_id);
    const wxRect r(node.pos, node.size);
    const SopGraphNodeStyle st = StyleForGraphNode(node, step);

    gc->SetBrush(wxBrush(st.fill));
    const wxPenStyle pen_style = st.border_dashed ? wxPENSTYLE_LONG_DASH : wxPENSTYLE_SOLID;
    gc->SetPen(wxPen(st.border, st.border_w, pen_style));
    gc->DrawRoundedRectangle(r.x, r.y, r.width, r.height, 12.0);

    wxBitmap icon = RoleBitmap(step, wxSize(18, 18));
    gc->DrawBitmap(icon, r.x + 8, r.y + kPadY, 18, 18);

    wxFont font = NodeFont(GetFont());
    font.SetPointSize(std::max(6, static_cast<int>(std::lround(font.GetPointSize() / zoom_))));
    if (st.bold) {
        font.MakeBold();
    }
    gc->SetFont(font, st.text);

    gc->PushState();
    gc->Clip(r.x + 1.0, r.y + 1.0, r.width - 2.0, r.height - 2.0);

    wxArrayString lines = wxSplit(node.title_text, '\n');
    double line_h = 0;
    gc->GetTextExtent("Ag", nullptr, &line_h);
    double ty = r.y + kPadY;
    for (size_t i = 0; i < lines.size(); i++) {
        const wxString &line = lines[i];
        gc->DrawText(line, r.x + kTextLeft, ty);
        ty += line_h;
    }
    if (node.show_check && !lines.empty()) {
        double check_w = 0;
        gc->GetTextExtent(kCheckMark, &check_w, nullptr);
        const double check_x = r.x + r.width - kPadX - check_w;
        const double check_y = r.y + kPadY + line_h * static_cast<double>(lines.size() - 1);
        gc->SetFont(font, wxColour(52, 168, 83));
        gc->DrawText(kCheckMark, check_x, check_y);
        gc->SetFont(font, st.text);
    }

    if (node.show_error) {
        DrawErrorMark(gc, r.x + r.width - kStatusW + 7, r.y + kPadY);
    }
    gc->PopState();
}

void SopGraphCanvas::DrawEdge(wxGraphicsContext *gc, const SopGraphEdge &edge) const {
    if (edge.points.size() < 2) {
        return;
    }

    const wxColour col = edge.on_active_path ? SopGraphAccentColour() : SopGraphForkColour();
    const double pen_w = (edge.on_active_path ? kEdgePenActive : kEdgePenFork) / zoom_;
    const wxPenStyle style = edge.on_active_path ? wxPENSTYLE_SOLID : wxPENSTYLE_SHORT_DASH;
    gc->SetPen(wxPen(col, std::max(1, static_cast<int>(std::lround(pen_w))), style));

    const double corner_r = kRouteCornerGraph / zoom_;
    StrokeRoundedPath(gc, edge.points, corner_r);

    if (!edge.show_arrow || edge.points.size() < 2) {
        return;
    }

    const wxPoint &entry = edge.points.back();
    const wxPoint &before = edge.points[edge.points.size() - 2];
    const double dx = entry.x - before.x;
    const double dy = entry.y - before.y;
    const double len = std::hypot(dx, dy);
    if (len < 0.5) {
        return;
    }

    const double head = kArrowHeadActive / zoom_;
    DrawArrowHead(gc, entry.x, entry.y, dx / len, dy / len, head, col);
}

void SopGraphCanvas::OnPaint(wxPaintEvent &) {
    wxAutoBufferedPaintDC dc(this);
    const wxSize client = GetClientSize();
    if (client.x <= 0 || client.y <= 0) {
        return;
    }

    dc.SetBackground(wxBrush(SopGraphBgColour()));
    dc.Clear();
    /* Hard clip: nothing may paint outside this canvas client rect. */
    dc.DestroyClippingRegion();
    dc.SetClippingRegion(0, 0, client.x, client.y);

    std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
    if (!gc) {
        return;
    }

    gc->Translate(pan_.x, pan_.y);
    gc->Scale(zoom_, zoom_);
    /* Clip in graph space to the visible viewport (after pan/zoom). */
    const double view_x = -static_cast<double>(pan_.x) / zoom_;
    const double view_y = -static_cast<double>(pan_.y) / zoom_;
    const double view_w = static_cast<double>(client.x) / zoom_;
    const double view_h = static_cast<double>(client.y) / zoom_;
    gc->Clip(view_x, view_y, view_w, view_h);

    for (const SopGraphEdge &edge : edges_) {
        if (!edge.on_active_path) {
            DrawEdge(gc.get(), edge);
        }
    }
    for (const SopGraphEdge &edge : edges_) {
        if (edge.on_active_path) {
            DrawEdge(gc.get(), edge);
        }
    }

    for (const SopGraphNode &node : nodes_) {
        DrawNode(gc.get(), node);
    }
}

