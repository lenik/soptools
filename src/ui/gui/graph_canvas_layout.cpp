/*
 * Copyright (C) 2026 Lenik <soptools@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "ui/gui/graph_canvas_layout.hpp"
#include "ui/gui/graph_style.hpp"

#include <wx/artprov.h>
#include <wx/dcbuffer.h>
#include <wx/dcclient.h>
#include <wx/graphics.h>

#include <algorithm>
#include <cmath>
#include <map>


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
                bool is_fork, bool show_arrow) {
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


