/*
 * Copyright (C) 2026 Lenik <soptools@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "gui/graph_style.hpp"
#include "gui/graph_canvas.hpp"

#include <algorithm>

namespace {

wxColour kWhite() { return wxColour(255, 255, 255); }
wxColour kGray() { return wxColour(200, 200, 205); }
wxColour kGreen() { return wxColour(198, 239, 206); }
wxColour kRed() { return wxColour(255, 205, 200); }
wxColour kOrange() { return wxColour(255, 214, 165); }
wxColour kSkyBlue() { return wxColour(186, 230, 253); }
wxColour kDefaultBorder() { return wxColour(209, 209, 214); }

wxColour Lighten(const wxColour &c, int amount) {
    return wxColour(std::min(255, static_cast<int>(c.Red()) + amount),
                    std::min(255, static_cast<int>(c.Green()) + amount),
                    std::min(255, static_cast<int>(c.Blue()) + amount));
}

/* Soft sky tint for selection — border stays as-is. */
wxColour SelectionFill(const wxColour &base) {
    return wxColour((static_cast<int>(base.Red()) + 186) / 2,
                    (static_cast<int>(base.Green()) + 220) / 2,
                    (static_cast<int>(base.Blue()) + 253) / 2);
}

} /* namespace */

wxColour SopGraphBgColour() { return wxColour(242, 242, 247); }
wxColour SopGraphCurrentBeige() { return SopGraphCurrentGreen(); }
/* Distinct mint vs Complete success green (198,239,206). */
wxColour SopGraphCurrentGreen() { return wxColour(220, 245, 228); }
wxColour SopGraphHoverYellow() { return wxColour(255, 249, 196); }
wxColour SopGraphAccentColour() { return wxColour(0, 122, 255); }
wxColour SopGraphForkColour() { return wxColour(199, 199, 204); }
wxColour SopGraphTextColour() { return wxColour(30, 30, 30); }
wxColour SopGraphMutedColour() { return wxColour(110, 110, 115); }

SopGraphNodeStyle StyleForGraphNode(const SopGraphNode &node, const SopStep &step) {
    SopGraphNodeStyle st;
    st.fill = kWhite();
    st.border = kDefaultBorder();
    st.text = SopGraphTextColour();
    st.border_w = 2;
    st.border_dashed = false;
    st.bold = false;

    if (!node.included) {
        st.fill = kGray();
        st.border = SopGraphMutedColour();
        st.text = SopGraphMutedColour();
    } else if (step.is_user()) {
        st.fill = kWhite();
    } else {
        switch (step.status) {
        case SopStepStatus::Complete:
            st.fill = kGreen();
            st.border = wxColour(52, 168, 83);
            break;
        case SopStepStatus::Error:
            st.fill = kRed();
            st.border = wxColour(255, 59, 48);
            break;
        case SopStepStatus::Running:
            st.fill = kOrange();
            st.border = wxColour(255, 149, 0);
            break;
        case SopStepStatus::Pending:
        case SopStepStatus::Waiting:
            if (node.started) {
                st.fill = kSkyBlue();
                st.border = wxColour(56, 170, 230);
            } else {
                st.fill = kWhite();
            }
            break;
        default:
            st.fill = kWhite();
            break;
        }
    }

    /* Current position: mint fill + bold border (not Complete green). */
    if (node.is_current) {
        st.fill = SopGraphCurrentGreen();
        st.border = wxColour(34, 120, 70);
        st.border_w = 4;
        st.border_dashed = false;
        st.bold = true;
    } else if (node.selected) {
        /* Selection changes fill only; keep status/default border. */
        st.fill = SelectionFill(st.fill);
    }

    if (node.hovered) {
        st.fill = Lighten(st.fill, 28);
    }

    return st;
}
