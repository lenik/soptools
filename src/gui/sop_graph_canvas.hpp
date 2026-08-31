#ifndef SOP_GRAPH_CANVAS_HPP
#define SOP_GRAPH_CANVAS_HPP

#include "../sop_runtime.hpp"

#include <wx/panel.h>
#include <wx/timer.h>
#include <functional>
#include <string>
#include <vector>

class wxGraphicsContext;
class wxMenu;

struct SopGraphNode {
    std::string step_id;
    int seq = 0;
    int wrap_row = 0;
    int wrap_col = 0;
    wxPoint pos;
    wxSize size{96, 44};
    bool included = true;
    bool on_active_path = false;
    bool started = false;
    bool is_current = false;
    bool selected = false;
    bool hovered = false;
    size_t path_index = 0;
    wxString title_text;
    bool show_check = false;
    bool show_error = false;
};

struct SopGraphEdge {
    std::vector<wxPoint> points;
    bool on_active_path = false;
    bool is_fork = false;
};

class SopGraphCanvas : public wxPanel {
public:
    using StepSelectFn = std::function<void(const std::string &step_id)>;
    using StepMoveFn = std::function<void(const std::string &step_id, size_t path_index)>;
    using StepActionFn = std::function<void(const std::string &step_id)>;
    using BranchActivateFn = std::function<void(int seq, const std::string &step_id)>;

    SopGraphCanvas(wxWindow *parent, SopEngine *engine);

    void Rebuild();
    void RedrawGraph();
    void SyncNodeStates();
    void SetCurrentIndex(size_t index);
    void ScrollToCurrentNode(bool animated = true);
    void SetSelectedStep(const std::string &step_id);
    void SetStepSelectHandler(StepSelectFn fn) { step_select_ = std::move(fn); }
    void SetMoveToHandler(StepMoveFn fn) { move_to_fn_ = std::move(fn); }
    void SetExecuteHandler(StepActionFn fn) { execute_fn_ = std::move(fn); }
    void SetExcludeHandler(std::function<void(const std::string &, bool)> fn) { exclude_fn_ = std::move(fn); }
    void SetBranchActivateHandler(BranchActivateFn fn) { branch_activate_fn_ = std::move(fn); }

private:
    wxDECLARE_EVENT_TABLE();

    static constexpr int ID_REDRAW = wxID_HIGHEST + 50;
    static constexpr int ID_CTX_EXCLUDED = wxID_HIGHEST + 51;
    static constexpr int ID_CTX_EXECUTE = wxID_HIGHEST + 52;
    static constexpr int ID_CTX_MOVE_HERE = wxID_HIGHEST + 53;

    SopEngine *engine_;
    std::vector<SopGraphNode> nodes_;
    std::vector<SopGraphEdge> edges_;
    wxSize graph_size_{800, 160};
    double zoom_ = 1.0;
    double default_zoom_ = 1.0;
    wxPoint pan_{0, 0};
    bool panning_ = false;
    bool user_panned_ = false;
    wxPoint pan_start_;
    wxPoint pan_origin_;
    int cols_per_row_ = 6;
    int hover_node_ = -1;
    int tab_focus_node_ = -1;
    std::string selected_step_id_;
    std::string context_step_id_;
    StepSelectFn step_select_;
    StepMoveFn move_to_fn_;
    StepActionFn execute_fn_;
    std::function<void(const std::string &, bool)> exclude_fn_;
    BranchActivateFn branch_activate_fn_;
    wxTimer pan_anim_timer_;
    wxPoint pan_anim_start_{0, 0};
    wxPoint pan_anim_target_{0, 0};
    int pan_anim_step_ = 0;
    int layout_cols_key_ = 0;
    static constexpr int kPanAnimSteps = 16;

    void LayoutGraph();
    void CenterPan();
    wxPoint PanToShowNode(const SopGraphNode &node, bool center_in_view = false) const;
    void StartPanAnimation(const wxPoint &target);
    void FinishPanAnimation();
    int LayoutColumnsPerRow() const;
    std::vector<wxPoint> RouteEdge(const wxPoint &from, const wxPoint &to, int from_row, int to_row,
                                   bool is_fork) const;
    wxPoint ScreenToGraph(const wxPoint &screen) const;
    int HitTestNode(const wxPoint &graph_pt) const;
    void ShowNodeContextMenu(const wxPoint &screen_pos, const std::string &step_id);
    wxRect NodeScreenRect(const SopGraphNode &node) const;
    void UpdateHover(int node_index);

    void OnPaint(wxPaintEvent &);
    void OnSize(wxSizeEvent &);
    void OnMouseWheel(wxMouseEvent &);
    void OnMouseDown(wxMouseEvent &);
    void OnMouseUp(wxMouseEvent &);
    void OnMouseMove(wxMouseEvent &);
    void OnMouseLeave(wxMouseEvent &);
    void OnMouseEnter(wxMouseEvent &);
    void OnKeyDown(wxKeyEvent &);
    void OnContextMenu(wxContextMenuEvent &);
    void OnRedraw(wxCommandEvent &);
    void OnCtxExcluded(wxCommandEvent &);
    void OnCtxExecute(wxCommandEvent &);
    void OnCtxMoveHere(wxCommandEvent &);
    void OnPanAnimTimer(wxTimerEvent &);
    void DrawNode(wxGraphicsContext *gc, const SopGraphNode &node);
    void DrawEdge(wxGraphicsContext *gc, const SopGraphEdge &edge) const;
    void RelayoutIfNeeded();
    int LayoutColumnsKey() const;
};

#endif /* SOP_GRAPH_CANVAS_HPP */
