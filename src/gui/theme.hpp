#ifndef GUI_THEME_HPP
#define GUI_THEME_HPP

#include "engine/engine.hpp"
#include "model/model.hpp"

#include <wx/wx.h>
#include <wx/artprov.h>
#include <wx/bmpbuttn.h>
#include <wx/menu.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

#include <functional>
#include <initializer_list>
#include <utility>

wxColour ios_bg();
wxColour ios_card();
wxColour ios_accent();
wxColour ios_muted();
wxColour ios_success();
wxColour ios_error();
wxColour ios_orange();
wxColour ios_sky();

wxBitmap role_icon(const SopStep &step, const wxSize &size);
wxString status_text(const SopStep &step);
wxColour status_colour(const SopStep &step);
void count_text_stats(const wxString &text, size_t &lines, size_t &chars);
bool copy_text_to_clipboard(const wxString &text);
wxString status_bar_text(const SopStep *step, SopEngine *engine);

wxBitmapButton *MakeDetachButton(wxWindow *parent, bool detached);
void UpdateDetachButton(wxBitmapButton *btn, wxWindow *parent, bool detached);
wxPanel *MakePaneHeader(wxWindow *parent, wxBitmapButton **button_out,
                        const std::function<void()> &on_click);
wxStaticText *MakeSectionTitle(wxWindow *parent, const wxString &title);
void AddShortcutRows(wxWindow *parent, wxFlexGridSizer *grid,
                     std::initializer_list<std::pair<const char *, const char *>> rows);
wxPanel *MakeShortcutSection(wxWindow *parent, const wxString &title,
                             std::initializer_list<std::pair<const char *, const char *>> rows);
wxMenuItem *AppendIconMenuItem(wxMenu *menu, int id, const wxString &label, const wxArtID &art);
wxMenuItem *AppendIconCheckItem(wxMenu *menu, int id, const wxString &label, const wxArtID &art);

#endif /* GUI_THEME_HPP */
