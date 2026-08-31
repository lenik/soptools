#ifndef SOP_GRAPH_STYLE_HPP
#define SOP_GRAPH_STYLE_HPP

#include "../sop_model.hpp"

#include <wx/colour.h>
#include <wx/gdicmn.h>

struct SopGraphNode;

struct SopGraphNodeStyle {
    wxColour fill;
    wxColour border;
    wxColour text;
    int border_w = 2;
    bool border_dashed = false;
    bool bold = false;
};

wxColour SopGraphBgColour();
wxColour SopGraphCurrentBeige();
wxColour SopGraphHoverYellow();
wxColour SopGraphAccentColour();
wxColour SopGraphForkColour();
wxColour SopGraphTextColour();
wxColour SopGraphMutedColour();

SopGraphNodeStyle StyleForGraphNode(const SopGraphNode &node, const SopStep &step);

#endif /* SOP_GRAPH_STYLE_HPP */
