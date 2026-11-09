/*
 * Copyright (C) 2026 Lenik <soptools@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "gui/gui.hpp"
#include "gui/theme.hpp"
#include "model/model.hpp"

#include <wx/artprov.h>
#include <wx/button.h>
#include <wx/choice.h>
#include <wx/frame.h>
#include <wx/listbox.h>
#include <wx/msgdlg.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/toolbar.h>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <vector>

namespace fs = std::filesystem;

namespace {

class SopEditorFrame : public wxFrame {
public:
    explicit SopEditorFrame(const std::string &sop_dir)
        : wxFrame(nullptr, wxID_ANY, wxString::Format(wxString::FromUTF8("SOP Editor — %s"),
                                                      wxString::FromUTF8(sop_dir)),
                  wxDefaultPosition, wxSize(1100, 780)),
          sop_dir_(sop_dir) {
        SetBackgroundColour(ios_bg());
        auto *tb = CreateToolBar();
        tb->AddTool(wxID_NEW, wxString::FromUTF8("New"),
                    wxArtProvider::GetBitmap(wxART_NEW, wxART_TOOLBAR));
        tb->AddTool(wxID_SAVE, wxString::FromUTF8("Save"),
                    wxArtProvider::GetBitmap(wxART_FILE_SAVE, wxART_TOOLBAR));
        tb->AddTool(wxID_DELETE, wxString::FromUTF8("Delete"),
                    wxArtProvider::GetBitmap(wxART_DELETE, wxART_TOOLBAR));
        tb->AddTool(wxID_REFRESH, wxString::FromUTF8("Reload"),
                    wxArtProvider::GetBitmap(wxART_REDO, wxART_TOOLBAR));
        tb->Realize();
        Bind(wxEVT_TOOL, &SopEditorFrame::OnNew, this, wxID_NEW);
        Bind(wxEVT_TOOL, &SopEditorFrame::OnSave, this, wxID_SAVE);
        Bind(wxEVT_TOOL, &SopEditorFrame::OnDelete, this, wxID_DELETE);
        Bind(wxEVT_TOOL, &SopEditorFrame::OnReload, this, wxID_REFRESH);

        auto *root = new wxBoxSizer(wxHORIZONTAL);
        list_ = new wxListBox(this, wxID_ANY);
        list_->Bind(wxEVT_LISTBOX, &SopEditorFrame::OnSelect, this);
        root->Add(list_, 0, wxEXPAND | wxALL, 8);

        auto *right = new wxPanel(this);
        right->SetBackgroundColour(ios_card());
        auto *form = new wxBoxSizer(wxVERTICAL);
        auto *meta = new wxBoxSizer(wxHORIZONTAL);
        meta->Add(new wxStaticText(right, wxID_ANY, wxString::FromUTF8("seq")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
        seq_ = new wxSpinCtrl(right, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(70, -1),
                              wxSP_ARROW_KEYS, 0, 999, 0);
        meta->Add(seq_, 0, wxRIGHT, 12);
        meta->Add(new wxStaticText(right, wxID_ANY, wxString::FromUTF8("variant")), 0,
                  wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
        variant_ = new wxChoice(right, wxID_ANY);
        variant_->Append(wxString::FromUTF8("a"));
        variant_->Append(wxString::FromUTF8("z"));
        variant_->SetSelection(0);
        meta->Add(variant_, 0, wxRIGHT, 12);
        meta->Add(new wxStaticText(right, wxID_ANY, wxString::FromUTF8("role")), 0,
                  wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
        role_ = new wxChoice(right, wxID_ANY);
        role_->Append(wxString::FromUTF8("shell"));
        role_->Append(wxString::FromUTF8("chat"));
        role_->Append(wxString::FromUTF8("code"));
        role_->SetSelection(1);
        meta->Add(role_, 0, wxRIGHT, 12);
        meta->Add(new wxStaticText(right, wxID_ANY, wxString::FromUTF8("ext")), 0,
                  wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
        ext_ = new wxChoice(right, wxID_ANY);
        ext_->Append(wxString::FromUTF8("md"));
        ext_->Append(wxString::FromUTF8("get"));
        ext_->Append(wxString::FromUTF8("sh"));
        ext_->SetSelection(0);
        meta->Add(ext_, 0);
        form->Add(meta, 0, wxEXPAND | wxALL, 8);

        form->Add(new wxStaticText(right, wxID_ANY, wxString::FromUTF8("title slug")), 0, wxLEFT | wxRIGHT, 8);
        name_ = new wxTextCtrl(right, wxID_ANY);
        form->Add(name_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

        form->Add(new wxStaticText(right, wxID_ANY, wxString::FromUTF8(".get headers (Save-As / Parse / Append-To / …)")), 0,
                  wxLEFT | wxRIGHT, 8);
        headers_ = new wxTextCtrl(right, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(-1, 80),
                                  wxTE_MULTILINE);
        form->Add(headers_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

        form->Add(new wxStaticText(right, wxID_ANY, wxString::FromUTF8("body")), 0, wxLEFT | wxRIGHT, 8);
        body_ = new wxTextCtrl(right, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                               wxTE_MULTILINE | wxTE_RICH2 | wxTE_PROCESS_TAB);
        form->Add(body_, 1, wxEXPAND | wxALL, 8);
        right->SetSizer(form);
        root->Add(right, 1, wxEXPAND | wxALL, 8);
        SetSizer(root);

        Reload();
    }

private:
    void Reload() {
        def_ = load_sop_directory(sop_dir_);
        ids_.clear();
        list_->Clear();
        std::vector<std::string> ids;
        for (const auto &kv : def_.steps) {
            ids.push_back(kv.first);
        }
        std::sort(ids.begin(), ids.end());
        for (const auto &id : ids) {
            const SopStep &s = def_.steps.at(id);
            list_->Append(wxString::Format(wxString::FromUTF8("%s  %s"), wxString::FromUTF8(id),
                                           wxString::FromUTF8(s.display_title())));
            ids_.push_back(id);
        }
        if (!ids_.empty()) {
            list_->SetSelection(0);
            LoadStep(ids_[0]);
        }
    }

    void LoadStep(const std::string &id) {
        current_id_ = id;
        const SopStep &s = def_.steps.at(id);
        seq_->SetValue(s.seq);
        variant_->SetSelection(s.variant == 'z' ? 1 : 0);
        if (s.role_slug == "shell") {
            role_->SetSelection(0);
        } else if (s.role_slug == "code" || s.role_slug == "codex") {
            role_->SetSelection(2);
        } else {
            role_->SetSelection(1);
        }
        if (s.extension == "get") {
            ext_->SetSelection(1);
        } else if (s.extension == "sh") {
            ext_->SetSelection(2);
        } else {
            ext_->SetSelection(0);
        }
        name_->SetValue(wxString::FromUTF8(s.name));
        body_->SetValue(wxString::FromUTF8(s.extension == "sh" ? s.shell_script : s.body));
        std::string hdr;
        if (!s.save_as.empty()) {
            hdr += "Save-As: " + s.save_as + "\n";
        }
        if (s.file_link == SopFileLink::Inode) {
            hdr += "File-Link: inode\n";
        } else if (s.file_link == SopFileLink::Sym) {
            hdr += "File-Link: sym\n";
        } else if (s.file_link == SopFileLink::None) {
            hdr += "File-Link: none\n";
        }
        if (!s.parse_formats.empty()) {
            hdr += "Parse: ";
            for (size_t i = 0; i < s.parse_formats.size(); i++) {
                if (i) {
                    hdr += ", ";
                }
                hdr += s.parse_formats[i];
            }
            hdr += "\n";
        }
        if (s.interaction == SopInteraction::Select) {
            hdr += "Interaction: select\n";
        }
        for (const auto &rule : s.append_tos) {
            hdr += "Append-To: " + rule.dest + " from " + rule.pattern;
            if (rule.delete_src) {
                hdr += " (delete)";
            }
            hdr += "\n";
        }
        headers_->SetValue(wxString::FromUTF8(hdr));
    }

    void OnSelect(wxCommandEvent &) {
        const int sel = list_->GetSelection();
        if (sel >= 0 && static_cast<size_t>(sel) < ids_.size()) {
            LoadStep(ids_[static_cast<size_t>(sel)]);
        }
    }

    void OnReload(wxCommandEvent &) { Reload(); }

    void OnNew(wxCommandEvent &) {
        SopStep s;
        s.seq = 999;
        s.variant = 'a';
        s.role_slug = "chat";
        s.role = SopRole::Chat;
        s.name = "new_step";
        s.extension = "get";
        s.body = "# New step\n";
        s.kind = SopStepKind::AiOutput;
        std::string err;
        if (!write_sop_file(s, sop_dir_, &err)) {
            wxMessageBox(wxString::FromUTF8(err), wxString::FromUTF8("New failed"), wxOK | wxICON_ERROR, this);
            return;
        }
        Reload();
    }

    void OnSave(wxCommandEvent &) {
        if (current_id_.empty()) {
            return;
        }
        SopStep s = def_.steps.at(current_id_);
        const std::string old_path = s.filepath;
        s.seq = seq_->GetValue();
        s.variant = variant_->GetSelection() == 1 ? 'z' : 'a';
        const int ri = role_->GetSelection();
        s.role_slug = ri == 0 ? "shell" : (ri == 2 ? "code" : "chat");
        s.role = parse_role_slug(s.role_slug);
        const int ei = ext_->GetSelection();
        s.extension = ei == 1 ? "get" : (ei == 2 ? "sh" : "md");
        s.name = std::string(name_->GetValue().ToUTF8());
        const std::string body = std::string(body_->GetValue().ToUTF8());
        if (s.extension == "sh") {
            s.shell_script = body;
            s.body = body;
        } else {
            s.body = body;
        }
        /* Re-parse headers from the headers box for .get */
        s.save_as.clear();
        s.parse_formats.clear();
        s.append_tos.clear();
        s.file_link = SopFileLink::Default;
        s.interaction = SopInteraction::None;
        if (s.extension == "get") {
            const std::string hdr = std::string(headers_->GetValue().ToUTF8()) + "\n\n";
            SopStep tmp = s;
            /* reuse parse via temporary content */
            const std::string tmp_name =
                "999a." + pad_role_slug("chat") + ".tmp.get";
            const fs::path tmp_path = fs::path(sop_dir_) / tmp_name;
            {
                std::ofstream out(tmp_path);
                out << hdr << body;
            }
            auto parsed = parse_sop_file(tmp_path.string());
            fs::remove(tmp_path);
            if (parsed) {
                s.save_as = parsed->save_as;
                s.file_link = parsed->file_link;
                s.parse_formats = parsed->parse_formats;
                s.interaction = parsed->interaction;
                s.append_tos = parsed->append_tos;
                s.body = parsed->body;
            }
        }
        std::string err;
        if (!write_sop_file(s, sop_dir_, &err)) {
            wxMessageBox(wxString::FromUTF8(err), wxString::FromUTF8("Save failed"), wxOK | wxICON_ERROR, this);
            return;
        }
        /* Remove old file if renamed */
        char seq_buf[16];
        std::snprintf(seq_buf, sizeof(seq_buf), "%03d", s.seq);
        const std::string new_name = std::string(seq_buf) + s.variant + "." + pad_role_slug(s.role_slug) + "." +
                                     s.name + "." + s.extension;
        const std::string new_path = sop_dir_ + "/" + new_name;
        if (!old_path.empty() && old_path != new_path && fs::exists(old_path)) {
            fs::remove(old_path);
        }
        Reload();
        SetStatusText(wxString::FromUTF8("Saved ") + wxString::FromUTF8(new_name));
    }

    void OnDelete(wxCommandEvent &) {
        if (current_id_.empty()) {
            return;
        }
        const SopStep &s = def_.steps.at(current_id_);
        if (wxMessageBox(wxString::Format(wxString::FromUTF8("Delete %s?"), wxString::FromUTF8(s.filename)),
                         wxString::FromUTF8("Delete"), wxYES_NO | wxICON_WARNING, this) != wxYES) {
            return;
        }
        if (!s.filepath.empty()) {
            fs::remove(s.filepath);
        }
        Reload();
    }

    std::string sop_dir_;
    SopDefinition def_;
    std::vector<std::string> ids_;
    std::string current_id_;
    wxListBox *list_ = nullptr;
    wxSpinCtrl *seq_ = nullptr;
    wxChoice *variant_ = nullptr;
    wxChoice *role_ = nullptr;
    wxChoice *ext_ = nullptr;
    wxTextCtrl *name_ = nullptr;
    wxTextCtrl *headers_ = nullptr;
    wxTextCtrl *body_ = nullptr;
};

} /* namespace */

wxFrame *create_sop_editor_frame(const std::string &sop_dir) {
    if (!fs::is_directory(sop_dir)) {
        wxMessageBox(wxString::FromUTF8("Not a directory: ") + wxString::FromUTF8(sop_dir),
                     wxString::FromUTF8("sopwin edit"), wxOK | wxICON_ERROR);
        return nullptr;
    }
    return new SopEditorFrame(sop_dir);
}
