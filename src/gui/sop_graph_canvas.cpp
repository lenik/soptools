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
constexpr int kBranchGap = 14;
/* Orthogonal bends need stub + turn; branch columns use 2x horizontal gap. */
constexpr int kBranchColGap = kColGap * 2;
constexpr int kMarginX = 48;
constexpr int kMarginY = 36;
constexpr int kRowGap = 28;
constexpr int kHitPad = 4;
constexpr int kMinNodeW = 88;
constexpr int kMaxNodeW = 132;
constexpr int kMinNodeH = 40;
constexpr int kTextLeft = 30;
constexpr int kPadX = 8;
constexpr int kPadY = 8;
constexpr int kStatusW = 16;
constexpr int kRouteStub = 10;
constexpr int kRouteCornerGraph = 8;
constexpr int kColBoxPad = 12;
constexpr int kWrapGutter = 28;

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

constexpr int kEdgePenActive = 2;
constexpr int kEdgePenFork = 1;
constexpr double kArrowHeadActive = 10.0;

wxPoint NodeLeftPort(const wxPoint &pos, const wxSize &size) {
    return wxPoint(pos.x, pos.y + size.y / 2);
}

wxPoint NodeRightPort(const wxPoint &pos, const wxSize &size) {
    return wxPoint(pos.x + size.x, pos.y + size.y / 2);
}

void DedupePoints(std::vector<wxPoint> &pts) {
    std::vector<wxPoint> out;
    for (const wxPoint &p : pts) {
        if (out.empty() || out.back().x != p.x || out.back().y != p.y) {
            out.push_back(p);
        }
    }
    pts.swap(out);
}

// Connect right-mid of source to left-mid of destination (same row).
std::vector<wxPoint> RouteBetweenPorts(const wxPoint &from_right, const wxPoint &to_left) {
    std::vector<wxPoint> pts;
    pts.push_back(from_right);

    if (from_right.y == to_left.y) {
        pts.push_back(to_left);
        DedupePoints(pts);
        return pts;
    }

    const int x_stub = from_right.x + kRouteStub;
    pts.emplace_back(x_stub, from_right.y);
    pts.emplace_back(x_stub, to_left.y);
    pts.push_back(to_left);
    DedupePoints(pts);
    return pts;
}

/* Wrap to next row: right → down → left past row start → down → right into next node. */
std::vector<wxPoint> RouteWrapToNextRow(const wxPoint &from_right, const wxPoint &to_left, int row_right_x,
                                        int next_row_left_x, int gutter_y) {
    const int x_out = std::max(from_right.x + kRouteStub, row_right_x + kWrapGutter);
    int x_in = std::min(to_left.x - kRouteStub, next_row_left_x - kWrapGutter);
    if (x_in >= to_left.x) {
        x_in = to_left.x - kRouteStub;
    }
    if (x_in >= x_out) {
        x_in = std::min(x_out - kWrapGutter, to_left.x - kRouteStub);
    }

    std::vector<wxPoint> pts;
    pts.push_back(from_right);
    pts.emplace_back(x_out, from_right.y); /* right */
    pts.emplace_back(x_out, gutter_y);     /* down */
    pts.emplace_back(x_in, gutter_y);      /* left past row start */
    pts.emplace_back(x_in, to_left.y);     /* down */
    pts.push_back(to_left);                /* right into node */
    DedupePoints(pts);
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
    const double wing = size * 0.45;
    wxGraphicsPath path = gc->CreatePath();
    path.MoveToPoint(tip_x, tip_y);
    path.AddLineToPoint(back_x - uy * wing, back_y + ux * wing);
    path.AddLineToPoint(back_x + uy * wing, back_y - ux * wing);
    path.CloseSubpath();
    gc->SetBrush(wxBrush(col));
    gc->SetPen(*wxTRANSPARENT_PEN);
    gc->FillPath(path);
}

void StrokeRoundedPath(wxGraphicsContext *gc, const std::vector<wxPoint> &pts, double corner_r) {
    if (pts.size() < 2) {
        return;
    }
    if (pts.size() == 2) {
        gc->StrokeLine(pts[0].x, pts[0].y, pts[1].x, pts[1].y);
        return;
    }

    wxGraphicsPath path = gc->CreatePath();
    path.MoveToPoint(pts[0].x, pts[0].y);

    for (size_t i = 1; i + 1 < pts.size(); i++) {
        const wxPoint &prev = pts[i - 1];
        const wxPoint &corner = pts[i];
        const wxPoint &next = pts[i + 1];

        const double in_x = corner.x - prev.x;
        const double in_y = corner.y - prev.y;
        const double out_x = next.x - corner.x;
        const double out_y = next.y - corner.y;
        const double in_len = std::hypot(in_x, in_y);
        const double out_len = std::hypot(out_x, out_y);
        if (in_len < 0.001 || out_len < 0.001) {
            path.AddLineToPoint(corner.x, corner.y);
            continue;
        }

        const double r = std::min(corner_r, std::min(in_len, out_len) * 0.45);
        const double in_ux = in_x / in_len;
        const double in_uy = in_y / in_len;
        const double out_ux = out_x / out_len;
        const double out_uy = out_y / out_len;

        const double start_x = corner.x - in_ux * r;
        const double start_y = corner.y - in_uy * r;
        const double end_x = corner.x + out_ux * r;
        const double end_y = corner.y + out_uy * r;

        path.AddLineToPoint(start_x, start_y);
        path.AddQuadCurveToPoint(corner.x, corner.y, end_x, end_y);
    }

    path.AddLineToPoint(pts.back().x, pts.back().y);
    gc->StrokePath(path);
}

void AppendEdge(std::vector<SopGraphEdge> &edges, const std::vector<wxPoint> &pts, bool on_active,
                bool is_fork, bool show_arrow = false) {
    if (pts.size() < 2) {
        return;
    }
    SopGraphEdge edge;
    edge.points = pts;
    edge.on_active_path = on_active;
    edge.is_fork = is_fork;
    edge.show_arrow = show_arrow;
    edges.push_back(edge);
}

/* Hub (column box port) to a node left-mid, or node right-mid to hub. */
std::vector<wxPoint> RouteHubToPort(const wxPoint &hub, const wxPoint &port, bool hub_is_start) {
    std::vector<wxPoint> pts;
    const wxPoint &from = hub_is_start ? hub : port;
    const wxPoint &to = hub_is_start ? port : hub;
    pts.push_back(from);
    if (from.y != to.y) {
        pts.emplace_back(hub.x, to.y);
    }
    pts.push_back(to);
    DedupePoints(pts);
    return pts;
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
