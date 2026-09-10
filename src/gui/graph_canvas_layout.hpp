#ifndef GUI_GRAPH_CANVAS_LAYOUT_HPP
#define GUI_GRAPH_CANVAS_LAYOUT_HPP

#include "model/model.hpp"
#include "gui/graph_canvas.hpp"

#include <wx/bitmap.h>
#include <wx/dc.h>
#include <wx/font.h>
#include <wx/graphics.h>
#include <wx/string.h>
#include <vector>


#include "engine/engine.hpp"

#include <wx/dc.h>

constexpr int kColGap = 18;
constexpr int kBranchGap = 14;
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

extern const wxString kCheckMark;

struct NodeMeasure {
    wxSize size;
    wxString title_text;
    int line_count = 1;
    bool show_check = false;
    bool show_error = false;
};

constexpr int kEdgePenActive = 2;
constexpr int kEdgePenFork = 1;
constexpr double kArrowHeadActive = 10.0;

NodeMeasure MeasureNode(const SopStep &step, const std::string &step_id, SopEngine *engine, wxDC &dc);

wxBitmap RoleBitmap(const SopStep &step, const wxSize &size);
wxString SimplifiedTitle(const SopStep &step);
wxFont NodeFont(const wxFont &base);
wxString WrapTitle(wxDC &dc, const wxString &text, int max_text_w, int max_lines, int &line_count);
void DrawErrorMark(wxGraphicsContext *gc, double x, double y);
wxPoint NodeLeftPort(const wxPoint &pos, const wxSize &size);
wxPoint NodeRightPort(const wxPoint &pos, const wxSize &size);
void DedupePoints(std::vector<wxPoint> &pts);
std::vector<wxPoint> RouteBetweenPorts(const wxPoint &from_right, const wxPoint &to_left);
std::vector<wxPoint> RouteWrapToNextRow(const wxPoint &from_right, const wxPoint &to_left, int row_right_x, int next_row_left_x, int gutter_y);
void DrawArrowHead(wxGraphicsContext *gc, double tip_x, double tip_y, double dir_x, double dir_y, double size, const wxColour &col);
void StrokeRoundedPath(wxGraphicsContext *gc, const std::vector<wxPoint> &pts, double corner_r);
void AppendEdge(std::vector<SopGraphEdge> &edges, const std::vector<wxPoint> &pts, bool on_active, bool is_fork, bool show_arrow = false);
std::vector<wxPoint> RouteHubToPort(const wxPoint &hub, const wxPoint &port, bool hub_is_start);

#endif /* GUI_GRAPH_CANVAS_LAYOUT_HPP */
