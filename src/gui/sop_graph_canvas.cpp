/*
 * Copyright (C) 2026 Lenik <soptools@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "sop_graph_canvas.hpp"
#include "sop_graph_style.hpp"

#include <wx/dcbuffer.h>
#include <wx/dcclient.h>
#include <wx/graphics.h>
#include <wx/artprov.h>
#include <wx/menu.h>
#include <algorithm>
#include <cmath>
#include <map>

namespace {

constexpr int kColGap = 18;
constexpr int kBranchGap = 10;
constexpr int kMarginX = 48;
constexpr int kMarginY = 36;
constexpr int kRowGap = 28;
constexpr int kHitPad = 4;
constexpr int kForkHubGap = 24;
constexpr int kMinNodeW = 88;
constexpr int kMaxNodeW = 132;
constexpr int kMinNodeH = 40;
constexpr int kEdgeOutGap = 12;
constexpr int kEdgeInGap = 6;
constexpr int kTextLeft = 30;
constexpr int kPadX = 8;
constexpr int kPadY = 8;
constexpr int kStatusW = 16;

const wxString kCheckMark = wxString::FromUTF8("\u2714");

wxBitmap RoleBitmap(const SopStep &step, const wxSize &size) {
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

wxString SimplifiedTitle(const SopStep &step) {
    return wxString::FromUTF8(humanize_name(step.name));
}

wxFont NodeFont(const wxFont &base) {
    wxFont font = base;
    font.SetPointSize(std::max(8, font.GetPointSize() - 1));
    return font;
}

wxString WrapTitle(wxDC &dc, const wxString &text, int max_text_w, int max_lines, int &line_count) {
    wxArrayString words = wxSplit(text, ' ');
    wxString out;
    wxString line;
    line_count = 0;
    for (size_t wi = 0; wi < words.size() && line_count < max_lines; wi++) {
        wxString trial = line.empty() ? words[wi] : line + " " + words[wi];
        if (dc.GetTextExtent(trial).x <= max_text_w) {
            line = trial;
            continue;
        }
        if (!line.empty()) {
            if (!out.empty()) {
                out += "\n";
            }
            out += line;
            line_count++;
            line = words[wi];
            continue;
        }
        wxString chunk;
        for (size_t ci = 0; ci < words[wi].length() && line_count < max_lines; ci++) {
            chunk += words[wi][ci];
            if (dc.GetTextExtent(chunk).x > max_text_w && chunk.length() > 1) {
                wxString emit = chunk.substr(0, chunk.length() - 1);
                if (!out.empty()) {
                    out += "\n";
                }
                out += emit;
                line_count++;
                chunk = words[wi][ci];
            }
        }
        line = chunk;
    }
    if (line_count < max_lines && !line.empty()) {
        if (!out.empty()) {
            out += "\n";
        }
        out += line;
        line_count++;
    }
    return out;
}

struct NodeMeasure {
    wxSize size;
    wxString title_text;
    bool show_check = false;
    bool show_error = false;
};

NodeMeasure MeasureNode(const SopStep &step, const std::string &step_id, SopEngine *engine, wxDC &dc) {
    NodeMeasure m;
    const wxFont font = NodeFont(dc.GetFont());
    dc.SetFont(font);
    m.show_check = step.is_action() && engine->is_complete(step_id);
    m.show_error = step.status == SopStepStatus::Error;
    const int check_w = m.show_check ? dc.GetTextExtent(kCheckMark).x + 4 : 0;
    const int err_w = m.show_error ? kStatusW : 0;
    const int suffix_w = std::max(check_w, err_w);
    const wxString raw = SimplifiedTitle(step);

    int max_text_w = kMaxNodeW - kTextLeft - suffix_w - kPadX;
    int line_count = 0;
    m.title_text = WrapTitle(dc, raw, max_text_w, 3, line_count);
    wxSize full = dc.GetMultiLineTextExtent(m.title_text);
    int w = kTextLeft + full.x + suffix_w + kPadX;
    w = std::clamp(w, kMinNodeW, kMaxNodeW);

    max_text_w = w - kTextLeft - suffix_w - kPadX;
    line_count = 0;
    m.title_text = WrapTitle(dc, raw, max_text_w, 3, line_count);
    full = dc.GetMultiLineTextExtent(m.title_text);
    int h = kPadY * 2 + std::max(full.y, 18);
    h = std::max(h, kMinNodeH);
    m.size = wxSize(std::min(w, kMaxNodeW), h);
    return m;
}

void DrawErrorMark(wxGraphicsContext *gc, double x, double y) {
    wxGraphicsPath tri = gc->CreatePath();
    tri.MoveToPoint(x, y);
    tri.AddLineToPoint(x + 7.0, y + 12.0);
    tri.AddLineToPoint(x - 7.0, y + 12.0);
    tri.CloseSubpath();
    gc->SetBrush(wxBrush(wxColour(255, 59, 48)));
    gc->SetPen(*wxTRANSPARENT_PEN);
    gc->FillPath(tri);
    wxFont warn_font(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
    gc->SetFont(warn_font, *wxWHITE);
    gc->DrawText("!", x - 2.5, y + 1.0);
}

constexpr int kLongSegScreen = 72;
constexpr int kMidArrowScreen = 56;
constexpr int kEdgePenActive = 2;
constexpr int kEdgePenFork = 1;
constexpr double kArrowHeadActive = 16.0;
constexpr double kArrowHeadFork = 11.0;
constexpr double kArrowMid = 8.0;

std::vector<wxPoint> RouteSpine(const wxPoint &from, const wxPoint &to, int from_row, int to_row, int lane_y) {
    std::vector<wxPoint> pts;
    pts.push_back(from);
    if (from_row == to_row) {
        if (std::abs(from.y - to.y) <= 2) {
            pts.push_back(to);
            return pts;
        }
        const int bend_x = (to.x < from.x - 4) ? to.x : from.x + kEdgeOutGap + 4;
        pts.emplace_back(bend_x, from.y);
        pts.emplace_back(bend_x, to.y);
        pts.push_back(to);
        return pts;
    }

    const int exit_x = from.x + kEdgeOutGap + 4;
    const int approach_x = to.x;
    pts.emplace_back(exit_x, from.y);
    pts.emplace_back(exit_x, lane_y);
    pts.emplace_back(approach_x, lane_y);
    pts.emplace_back(approach_x, to.y);
    pts.push_back(to);
    return pts;
}

void DrawArrowHead(wxGraphicsContext *gc, double tip_x, double tip_y, double dir_x, double dir_y,
                   double size, const wxColour &col) {
    const double len = std::hypot(dir_x, dir_y);
    if (len < 0.001) {
        return;
    }
    const double ux = dir_x / len;
    const double uy = dir_y / len;
    const double back_x = tip_x - ux * size;
    const double back_y = tip_y - uy * size;
    const double wing = size * 0.5;
    wxGraphicsPath path = gc->CreatePath();
    path.MoveToPoint(tip_x, tip_y);
    path.AddLineToPoint(back_x - uy * wing, back_y + ux * wing);
    path.AddLineToPoint(back_x + uy * wing, back_y - ux * wing);
    path.CloseSubpath();
    gc->SetBrush(wxBrush(col));
    gc->SetPen(*wxTRANSPARENT_PEN);
    gc->FillPath(path);
}

void AppendEdge(std::vector<SopGraphEdge> &edges, const std::vector<wxPoint> &pts, bool on_active,
                bool is_fork) {
    if (pts.size() < 2) {
        return;
    }
    SopGraphEdge edge;
    edge.points = pts;
    edge.on_active_path = on_active;
    edge.is_fork = is_fork;
    edges.push_back(edge);
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
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(-1, 200), wxBORDER_NONE),
      engine_(engine),
      pan_anim_timer_(this) {
    SetBackgroundColour(SopGraphBgColour());
    SetBackgroundStyle(wxBG_STYLE_PAINT);
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
    ScrollToCurrentNode(true);
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

void SopGraphCanvas::ScrollToCurrentNode(bool animated) {
    const SopGraphNode *current = nullptr;
    for (const auto &node : nodes_) {
        if (node.is_current) {
            current = &node;
            break;
        }
    }
    if (!current) {
        return;
    }
    const wxSize client = GetClientSize();
    if (client.x < 20 || client.y < 20) {
        CallAfter([this, animated]() { ScrollToCurrentNode(animated); });
        return;
    }
    const wxPoint target = PanToShowNode(*current, true);
    user_panned_ = false;
    if (animated && (target.x != pan_.x || target.y != pan_.y)) {
        StartPanAnimation(target);
    } else if (target.x != pan_.x || target.y != pan_.y) {
        pan_ = target;
        Refresh(false);
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
    ScrollToCurrentNode(false);
}

void SopGraphCanvas::RedrawGraph() {
    zoom_ = default_zoom_;
    user_panned_ = false;
    layout_cols_key_ = 0;
    RelayoutIfNeeded();
    CenterPan();
    ScrollToCurrentNode(false);
}

int SopGraphCanvas::LayoutColumnsPerRow() const {
    int client_w = GetClientSize().x;
    if (client_w <= 0 && GetParent()) {
        client_w = GetParent()->GetClientSize().x - 24;
    }
    if (client_w <= 0) {
        client_w = 800;
    }
    const int col_pitch = kMaxNodeW + kColGap;
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

std::vector<wxPoint> SopGraphCanvas::RouteEdge(const wxPoint &from, const wxPoint &to, int from_row,
                                               int to_row, bool is_fork) const {
    (void)is_fork;
    const int lane_y = from.y + kMinNodeH / 2 + kRowGap / 2;
    return RouteSpine(from, to, from_row, to_row, lane_y);
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
        std::vector<std::string> step_ids;
        std::map<std::string, NodeMeasure> measures;
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
        for (SeqCol *col : kv.second) {
            col->x = x;
            x += col->col_w + kColGap;
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

    wxPoint prev_out;
    int prev_wrap_row = 0;
    bool have_prev = false;

    for (const SeqCol &col : columns) {
        const int count = static_cast<int>(col.step_ids.size());
        const int base_y = row_y[col.wrap_row];
        const int block_h = row_block_h[col.wrap_row];
        const int center_y = base_y + block_h / 2;

        int stack_h = 0;
        for (const std::string &id : col.step_ids) {
            stack_h += col.measures.at(id).size.y;
        }
        if (count > 1) {
            stack_h += (count - 1) * kBranchGap;
        }
        int y = center_y - stack_h / 2;

        wxPoint active_center;
        int active_w = 0;
        bool have_active = false;

        struct BranchNode {
            wxPoint center;
            wxPoint node_in;
            wxPoint node_out;
            bool on_active = false;
            bool included = false;
        };
        std::vector<BranchNode> branch_nodes;

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

            BranchNode bn;
            bn.center = wxPoint(node.pos.x + node.size.x / 2, node.pos.y + node.size.y / 2);
            bn.node_in = wxPoint(node.pos.x - kEdgeInGap, bn.center.y);
            bn.node_out = wxPoint(node.pos.x + node.size.x + kEdgeOutGap, bn.center.y);
            bn.on_active = node.on_active_path;
            bn.included = node.included;
            branch_nodes.push_back(bn);

            if (node.on_active_path && node.included) {
                active_center = bn.center;
                active_w = node.size.x;
                have_active = true;
            }

            y += m.size.y + kBranchGap;
        }

        if (count > 1) {
            const int hub_x = col.x - kForkHubGap;
            const int spine_y = have_active ? active_center.y : center_y;
            const int lane_y = row_y[prev_wrap_row] + row_block_h[prev_wrap_row] + kRowGap / 2;

            if (have_prev) {
                AppendEdge(edges_,
                           RouteSpine(prev_out, wxPoint(hub_x, spine_y), prev_wrap_row, col.wrap_row, lane_y),
                           true, false);
            }

            for (const BranchNode &bn : branch_nodes) {
                if (!bn.included) {
                    continue;
                }
                std::vector<wxPoint> spoke = {wxPoint(hub_x, spine_y), wxPoint(bn.node_in.x, bn.center.y), bn.node_in};
                AppendEdge(edges_, spoke, bn.on_active && bn.included, true);
            }
        } else if (have_prev && have_active) {
            const int lane_y = row_y[prev_wrap_row] + row_block_h[prev_wrap_row] + kRowGap / 2;
            AppendEdge(edges_,
                       RouteSpine(prev_out,
                                  wxPoint(active_center.x - active_w / 2 - kEdgeInGap, active_center.y),
                                  prev_wrap_row, col.wrap_row, lane_y),
                       true, false);
        }

        if (have_active) {
            prev_out = wxPoint(active_center.x + active_w / 2 + kEdgeOutGap, active_center.y);
            prev_wrap_row = col.wrap_row;
            have_prev = true;
        }
    }

    int max_x = kMarginX;
    for (const SeqCol &col : columns) {
        max_x = std::max(max_x, col.x + col.col_w);
    }

    graph_size_.x = max_x + kMarginX;
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
    if (st.bold) {
        font.MakeBold();
    }
    gc->SetFont(font, st.text);

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
}

void SopGraphCanvas::DrawEdge(wxGraphicsContext *gc, const SopGraphEdge &edge) const {
    if (edge.points.size() < 2) {
        return;
    }
    const int pen_w = edge.on_active_path ? kEdgePenActive : kEdgePenFork;
    const wxPen pen(edge.on_active_path ? SopGraphAccentColour() : SopGraphForkColour(), pen_w,
                    edge.on_active_path ? wxPENSTYLE_SOLID : wxPENSTYLE_SHORT_DASH);
    const double head = edge.on_active_path ? kArrowHeadActive : kArrowHeadFork;
    const wxColour col = pen.GetColour();

    for (size_t i = 1; i < edge.points.size(); i++) {
        const wxPoint &a = edge.points[i - 1];
        const wxPoint &b = edge.points[i];
        const double ax = pan_.x + a.x * zoom_;
        const double ay = pan_.y + a.y * zoom_;
        const double bx = pan_.x + b.x * zoom_;
        const double by = pan_.y + b.y * zoom_;
        gc->SetPen(pen);
        gc->StrokeLine(ax, ay, bx, by);

        const double dx = bx - ax;
        const double dy = by - ay;
        const double len = std::hypot(dx, dy);
        if (len < 1.0) {
            continue;
        }
        const double ux = dx / len;
        const double uy = dy / len;
        const bool terminal = i + 1 == edge.points.size();

        if (len >= kLongSegScreen) {
            for (double d = kMidArrowScreen; d < len - kMidArrowScreen * 0.6; d += kMidArrowScreen) {
                DrawArrowHead(gc, ax + ux * d, ay + uy * d, ux, uy, kArrowMid, col);
            }
        }
        if (terminal) {
            const double inset = std::min(len * 0.5, head + 4.0);
            DrawArrowHead(gc, bx - ux * inset, by - uy * inset, ux, uy, head, col);
        }
    }
}

void SopGraphCanvas::OnPaint(wxPaintEvent &) {
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(wxBrush(SopGraphBgColour()));
    dc.Clear();

    std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
    if (!gc) {
        return;
    }

    for (const SopGraphEdge &edge : edges_) {
        DrawEdge(gc.get(), edge);
    }

    gc->Translate(pan_.x, pan_.y);
    gc->Scale(zoom_, zoom_);

    for (const SopGraphNode &node : nodes_) {
        DrawNode(gc.get(), node);
    }
}

void SopGraphCanvas::OnSize(wxSizeEvent &evt) {
    evt.Skip();
    if (!engine_) {
        return;
    }
    RelayoutIfNeeded();
    if (!user_panned_) {
        CenterPan();
        ScrollToCurrentNode(false);
    }
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
