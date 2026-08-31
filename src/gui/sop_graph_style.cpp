/*
 * Copyright (C) 2026 Lenik <soptools@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "sop_graph_style.hpp"
#include "sop_graph_canvas.hpp"

namespace {

wxColour kWhite() { return wxColour(255, 255, 255); }
wxColour kGray() { return wxColour(200, 200, 205); }
wxColour kGreen() { return wxColour(198, 239, 206); }
wxColour kRed() { return wxColour(255, 205, 200); }
wxColour kOrange() { return wxColour(255, 214, 165); }
wxColour kSkyBlue() { return wxColour(186, 230, 253); }

wxColour StatusFill(const SopGraphNode &node, const wxColour &base) {
    if (node.hovered) {
        return SopGraphHoverYellow();
    }
    if (node.is_current) {
        return SopGraphCurrentBeige();
    }
    return base;
}

void ApplySelectionBorder(SopGraphNodeStyle &st) {
    st.border = SopGraphTextColour();
    st.border_w = 3;
    st.border_dashed = true;
    st.bold = true;
}

} /* namespace */

wxColour SopGraphBgColour() { return wxColour(242, 242, 247); }
wxColour SopGraphCurrentBeige() { return wxColour(245, 235, 210); }
wxColour SopGraphHoverYellow() { return wxColour(255, 249, 196); }
wxColour SopGraphAccentColour() { return wxColour(0, 122, 255); }
wxColour SopGraphForkColour() { return wxColour(199, 199, 204); }
wxColour SopGraphTextColour() { return wxColour(30, 30, 30); }
wxColour SopGraphMutedColour() { return wxColour(110, 110, 115); }

SopGraphNodeStyle StyleForGraphNode(const SopGraphNode &node, const SopStep &step) {
    SopGraphNodeStyle st;
    st.fill = kWhite();
    st.border = wxColour(209, 209, 214);
    st.text = SopGraphTextColour();
    st.border_w = 2;
    st.border_dashed = false;
    st.bold = false;

    if (!node.included) {
        st.fill = StatusFill(node, kGray());
        st.border = SopGraphMutedColour();
        st.text = SopGraphMutedColour();
        if (node.selected) {
            ApplySelectionBorder(st);
        }
        return st;
    }

    if (step.is_user()) {
        st.fill = StatusFill(node, kWhite());
        if (node.selected) {
            ApplySelectionBorder(st);
        }
        return st;
    }

    switch (step.status) {
    case SopStepStatus::Complete:
        st.fill = StatusFill(node, kGreen());
        st.border = wxColour(52, 168, 83);
        break;
    case SopStepStatus::Error:
        st.fill = StatusFill(node, kRed());
        st.border = wxColour(255, 59, 48);
        break;
    case SopStepStatus::Running:
        st.fill = StatusFill(node, kOrange());
        st.border = wxColour(255, 149, 0);
        break;
    case SopStepStatus::Pending:
    case SopStepStatus::Waiting:
        if (node.started) {
            st.fill = StatusFill(node, kSkyBlue());
            st.border = wxColour(56, 170, 230);
        } else {
            st.fill = StatusFill(node, kWhite());
        }
        break;
    default:
        st.fill = StatusFill(node, kWhite());
        break;
    }

    if (node.selected) {
        ApplySelectionBorder(st);
    }

    return st;
}
