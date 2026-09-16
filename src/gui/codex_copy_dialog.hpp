#ifndef GUI_CODEX_COPY_DIALOG_HPP
#define GUI_CODEX_COPY_DIALOG_HPP

#include "model/model.hpp"

#include <wx/dialog.h>

/* Styled “copied for Codex” confirm. Returns true if OK. */
bool show_codex_copy_dialog(wxWindow *parent, const SopStep &step);

#endif /* GUI_CODEX_COPY_DIALOG_HPP */
