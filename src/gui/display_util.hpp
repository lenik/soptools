#ifndef GUI_DISPLAY_UTIL_HPP
#define GUI_DISPLAY_UTIL_HPP

#include <string>
#include <wx/window.h>

/* Centre win on the display that contains ref (or the mouse if ref is null). */
void centre_on_same_display(wxWindow *win, wxWindow *ref = nullptr);

/* Encode / decode display client area as "x,y,w,h" for cross-process sopdlg. */
std::string encode_display_geometry(wxWindow *ref);
void centre_on_display_geometry(wxWindow *win, const std::string &geometry);

#endif /* GUI_DISPLAY_UTIL_HPP */
