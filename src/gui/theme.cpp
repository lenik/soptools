/*
 * Copyright (C) 2026 Lenik <soptools@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "gui/theme.hpp"

#include <wx/artprov.h>
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/panel.h>

wxColour ios_bg() { return wxColour(242, 242, 247); }
wxColour ios_card() { return wxColour(255, 255, 255); }
wxColour ios_accent() { return wxColour(0, 122, 255); }
wxColour ios_muted() { return wxColour(142, 142, 147); }
wxColour ios_success() { return wxColour(52, 199, 89); }
wxColour ios_error() { return wxColour(255, 59, 48); }
wxColour ios_orange() { return wxColour(255, 149, 0); }
wxColour ios_sky() { return wxColour(56, 170, 230); }

wxBitmap role_icon(const SopStep &step, const wxSize &size) {
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

wxString status_text(const SopStep &step) {
    if (step.is_user()) {
        return wxString::FromUTF8("User");
    }
    switch (step.status) {
    case SopStepStatus::Pending:
        return wxString::FromUTF8("Pending");
    case SopStepStatus::Running:
        return wxString::FromUTF8("Running");
    case SopStepStatus::Waiting:
        return wxString::FromUTF8("Waiting");
    case SopStepStatus::Complete:
        return wxString::FromUTF8("Complete");
    case SopStepStatus::Error:
        return wxString::FromUTF8("Error");
    case SopStepStatus::User:
        return wxString::FromUTF8("User");
    }
    return wxString::FromUTF8("Unknown");
}

wxColour status_colour(const SopStep &step) {
    if (step.is_user()) {
        return ios_muted();
    }
    switch (step.status) {
    case SopStepStatus::Complete:
        return ios_success();
    case SopStepStatus::Error:
        return ios_error();
    case SopStepStatus::Running:
        return ios_orange();
    case SopStepStatus::Pending:
    case SopStepStatus::Waiting:
        return ios_sky();
    default:
        return ios_muted();
    }
}

void count_text_stats(const wxString &text, size_t &lines, size_t &chars) {
    chars = static_cast<size_t>(text.length());
    if (text.empty()) {
        lines = 0;
        return;
    }
    lines = 1;
    for (wxString::const_iterator it = text.begin(); it != text.end(); ++it) {
        if (*it == '\n') {
            lines++;
        }
    }
}

bool copy_text_to_clipboard(const wxString &text) {
    if (!wxTheClipboard->Open()) {
        return false;
    }
    wxTheClipboard->SetData(new wxTextDataObject(text));
    wxTheClipboard->Close();
    return true;
}

wxString status_bar_text(const SopStep *step, SopEngine *engine) {
    if (!step || !engine) {
        return wxString::FromUTF8("Ready");
    }
    const size_t idx = engine->current_index();
    if (idx >= engine->active_steps().size()) {
        return wxString::FromUTF8("Ready");
    }
    const std::string &sid = engine->active_steps()[idx];
    if (engine->is_shell_running(sid)) {
        return wxString::FromUTF8("Shell script is running…");
    }
    if (step->is_prompt_copy()) {
        if (step->is_gpt_get()) {
            switch (step->status) {
            case SopStepStatus::Complete:
                return wxString::FromUTF8("Step completed.");
            case SopStepStatus::Waiting:
                return wxString::FromUTF8("Paste the GPT response in the dialog, then Save.");
            default:
                return wxString::FromUTF8(
                    "Next/Get copies the prompt and opens Paste Response for the GPT reply.");
            }
        }
        switch (step->status) {
        case SopStepStatus::Complete:
            return wxString::FromUTF8("Step completed.");
        case SopStepStatus::Waiting:
            return wxString::FromUTF8("Paste AI output below and click Apply.");
        default:
            return wxString::FromUTF8("Click Copy to copy the prompt to the clipboard.");
        }
    }
    if (step->is_automatable()) {
        switch (step->status) {
        case SopStepStatus::Running:
            return wxString::FromUTF8("Shell script is running…");
        case SopStepStatus::Complete:
            return wxString::FromUTF8("Shell script has been successfully executed.");
        case SopStepStatus::Error:
            if (!step->status_message.empty()) {
                return wxString::FromUTF8("Shell script failed: ") +
                       wxString::FromUTF8(step->status_message);
            }
            return wxString::FromUTF8("Shell script failed.");
        case SopStepStatus::Pending:
            return wxString::FromUTF8("Ready to execute shell script.");
        default:
            break;
        }
    }
    if (step->is_user()) {
        switch (step->status) {
        case SopStepStatus::Complete:
            return wxString::FromUTF8("Step completed.");
        case SopStepStatus::Waiting:
            return wxString::FromUTF8("Paste AI output below and click Apply.");
        default:
            return wxString::FromUTF8("Complete this user step manually.");
        }
    }
    if (!step->status_message.empty()) {
        return wxString::FromUTF8(step->status_message);
    }
    return wxString::FromUTF8("Ready");
}

wxBitmapButton *MakeDetachButton(wxWindow *parent, bool detached) {
    const wxArtID art = detached ? wxART_UNDO : wxART_NEW;
    const wxString tip = detached ? wxString::FromUTF8("Attach to main window")
                                  : wxString::FromUTF8("Detach to separate window");
    auto *btn = new wxBitmapButton(parent, wxID_ANY,
                                   wxArtProvider::GetBitmap(art, wxART_BUTTON, wxSize(16, 16)),
                                   wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
    btn->SetBackgroundColour(parent->GetBackgroundColour());
    btn->SetToolTip(tip);
    return btn;
}

void UpdateDetachButton(wxBitmapButton *btn, wxWindow *parent, bool detached) {
    if (!btn) {
        return;
    }
    const wxArtID art = detached ? wxART_UNDO : wxART_NEW;
    btn->SetBitmap(wxArtProvider::GetBitmap(art, wxART_BUTTON, wxSize(16, 16)));
    btn->SetToolTip(detached ? wxString::FromUTF8("Attach to main window")
                             : wxString::FromUTF8("Detach to separate window"));
    btn->SetBackgroundColour(parent->GetBackgroundColour());
}

wxPanel *MakePaneHeader(wxWindow *parent, wxBitmapButton **button_out,
                        const std::function<void()> &on_click) {
    auto *header = new wxPanel(parent);
    header->SetBackgroundColour(parent->GetBackgroundColour());
    auto *row = new wxBoxSizer(wxHORIZONTAL);
    row->AddStretchSpacer(1);
    wxBitmapButton *btn = MakeDetachButton(header, false);
    btn->Bind(wxEVT_BUTTON, [on_click](wxCommandEvent &) { on_click(); });
    row->Add(btn, 0, wxALIGN_CENTER_VERTICAL | wxTOP | wxRIGHT, 4);
    header->SetSizer(row);
    header->SetMinSize(wxSize(-1, 28));
    if (button_out) {
        *button_out = btn;
    }
    return header;
}

wxStaticText *MakeSectionTitle(wxWindow *parent, const wxString &title) {
    auto *label = new wxStaticText(parent, wxID_ANY, title);
    wxFont font = label->GetFont();
    font.MakeBold();
    label->SetFont(font);
    return label;
}

void AddShortcutRows(wxWindow *parent, wxFlexGridSizer *grid,
                     std::initializer_list<std::pair<const char *, const char *>> rows) {
    for (const auto &row : rows) {
        auto *key = new wxStaticText(parent, wxID_ANY, wxString::FromUTF8(row.first));
        wxFont key_font = key->GetFont();
        key_font.MakeBold();
        key->SetFont(key_font);
        grid->Add(key, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
        grid->Add(new wxStaticText(parent, wxID_ANY, wxString::FromUTF8(row.second)), 0,
                wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
    }
}

wxPanel *MakeShortcutSection(wxWindow *parent, const wxString &title,
                             std::initializer_list<std::pair<const char *, const char *>> rows) {
    auto *panel = new wxPanel(parent);
    panel->SetBackgroundColour(parent->GetBackgroundColour());
    auto *section = new wxBoxSizer(wxVERTICAL);
    section->Add(MakeSectionTitle(panel, title), 0, wxBOTTOM, 6);
    auto *grid = new wxFlexGridSizer(2, 12, 4);
    grid->AddGrowableCol(1);
    AddShortcutRows(panel, grid, rows);
    section->Add(grid, 0, wxEXPAND);
    panel->SetSizer(section);
    return panel;
}

wxMenuItem *AppendIconMenuItem(wxMenu *menu, int id, const wxString &label, const wxArtID &art) {
    auto *item = new wxMenuItem(menu, id, label);
    item->SetBitmap(wxArtProvider::GetBitmap(art, wxART_MENU, wxSize(16, 16)));
    menu->Append(item);
    return item;
}

wxMenuItem *AppendIconCheckItem(wxMenu *menu, int id, const wxString &label, const wxArtID &art) {
    /* GTK cannot put an image on a check menu item (gtk_image_menu_item_set_image
     * asserts GTK_IS_IMAGE_MENU_ITEM). Keep the check kind; art is unused. */
    (void)art;
    return menu->AppendCheckItem(id, label);
}

