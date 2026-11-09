/*
 * Copyright (C) 2026 Lenik <soptools@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "gui/gui.hpp"
#include "gui/display_util.hpp"
#include "gui/main_frame.hpp"
#include "gui/theme.hpp"
#include "engine/stage_files.hpp"

#include <wx/artprov.h>
#include <wx/button.h>
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/dnd.h>
#include <wx/frame.h>
#include <wx/listbox.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/statline.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/wx.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

enum class GuiEntryMode {
    Main,
    SopDlg,
    Edit,
};

GuiEntryMode g_gui_mode = GuiEntryMode::Main;
SopEngine *g_sop_engine = nullptr;
std::string g_edit_sop_dir;

struct SopAppendRule {
    std::string dest;
    std::string pattern;
    bool delete_src = false;
};

struct SopDlgOpts {
    std::string title = "sopwin sopdlg";
    std::string status = "Prompt has been copied to the clipboard";
    bool confirm = false;
    std::string stage_dir;
    std::vector<std::string> expected;
    std::string prompt;
    std::string response_path;
    std::string project_dir;
    std::string save_as;
    std::string parse;
    std::string file_link;
    std::vector<SopAppendRule> append_tos;
    std::string parent_display;
};

SopDlgOpts g_sopdlg;
int g_sopdlg_exit = 1;

std::string read_file(const std::string &path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return {};
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

std::string read_all_stdin() {
    std::ostringstream ss;
    ss << std::cin.rdbuf();
    return ss.str();
}

bool write_file(const std::string &path, const std::string &text) {
    if (path.empty()) {
        std::cout << text;
        return static_cast<bool>(std::cout);
    }
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        return false;
    }
    out << text;
    return true;
}

bool append_file(const std::string &path, const std::string &text) {
    fs::path p(path);
    std::error_code ec;
    if (p.has_parent_path()) {
        fs::create_directories(p.parent_path(), ec);
    }
    const bool exists = fs::exists(p, ec);
    std::ofstream out(path, std::ios::binary | std::ios::app);
    if (!out) {
        return false;
    }
    if (exists && !text.empty()) {
        out << "\n";
    }
    out << text;
    return static_cast<bool>(out);
}

bool copy_to_clipboard(const std::string &text) {
    if (!wxTheClipboard->Open()) {
        return false;
    }
    wxTheClipboard->SetData(new wxTextDataObject(wxString::FromUTF8(text)));
    wxTheClipboard->Close();
    return true;
}

std::vector<SopAppendRule> parse_append_env(const std::string &raw) {
    /* Lines: dest|pattern|0|1 */
    std::vector<SopAppendRule> out;
    std::istringstream in(raw);
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            continue;
        }
        const size_t a = line.find('|');
        const size_t b = a == std::string::npos ? std::string::npos : line.find('|', a + 1);
        if (a == std::string::npos || b == std::string::npos) {
            continue;
        }
        SopAppendRule r;
        r.dest = line.substr(0, a);
        r.pattern = line.substr(a + 1, b - a - 1);
        r.delete_src = line.substr(b + 1) == "1" || line.substr(b + 1) == "true" ||
                       line.substr(b + 1) == "delete";
        if (!r.dest.empty() && !r.pattern.empty()) {
            out.push_back(r);
        }
    }
    return out;
}

const SopAppendRule *match_append_rule(const std::vector<SopAppendRule> &rules,
                                       const std::string &basename) {
    const std::string norm = normalize_stage_basename(basename);
    for (const auto &r : rules) {
        if (match_simple_glob(r.pattern, basename) || match_simple_glob(r.pattern, norm)) {
            return &r;
        }
    }
    return nullptr;
}

class SopDlgFrame;

class SopDlgFileDropTarget : public wxFileDropTarget {
public:
    explicit SopDlgFileDropTarget(SopDlgFrame *frame) : frame_(frame) {}
    bool OnDropFiles(wxCoord, wxCoord, const wxArrayString &filenames) override;

private:
    SopDlgFrame *frame_;
};

class SopDlgFrame : public wxFrame {
public:
    explicit SopDlgFrame(const SopDlgOpts &opts)
        : wxFrame(nullptr, wxID_ANY, wxString::FromUTF8(opts.title), wxDefaultPosition,
                  opts.confirm ? wxSize(480, 280) : wxSize(920, 740)),
          opts_(opts) {
        SetBackgroundColour(ios_bg());
        if (opts_.confirm) {
            BuildConfirm();
        } else {
            BuildPaste();
        }
        if (!opts_.parent_display.empty()) {
            centre_on_display_geometry(this, opts_.parent_display);
        } else {
            centre_on_same_display(this, nullptr);
        }
    }

    int exit_code() const { return exit_code_; }

    void AddDroppedFiles(const wxArrayString &filenames) {
        int added = 0;
        int skipped = 0;
        for (size_t i = 0; i < filenames.size(); i++) {
            const std::string path = std::string(filenames[i].ToUTF8());
            const std::string base = fs::path(path).filename().string();
            const SopAppendRule *rule = match_append_rule(opts_.append_tos, base);
            if (!rule) {
                skipped++;
                continue;
            }
            DroppedItem item;
            item.dest = rule->dest;
            item.src_path = path;
            item.delete_src = rule->delete_src;
            drops_.push_back(item);
            files_->Append(wxString::FromUTF8(rule->dest));
            added++;
        }
        if (status_) {
            if (added && skipped) {
                status_->SetLabel(wxString::Format(wxString::FromUTF8("Added %d file(s); skipped %d unmatched."),
                                                   added, skipped));
            } else if (added) {
                status_->SetLabel(wxString::Format(wxString::FromUTF8("Added %d file(s) for Append-To."), added));
            } else if (skipped) {
                status_->SetLabel(wxString::FromUTF8("No dropped file matched an Append-To pattern."));
            }
        }
    }

private:
    struct DroppedItem {
        std::string dest;
        std::string src_path;
        bool delete_src = false;
    };

    void Finish(int code) {
        exit_code_ = code;
        g_sopdlg_exit = code;
        Close(true);
    }

    void BuildConfirm() {
        auto *root = new wxBoxSizer(wxVERTICAL);
        root->AddSpacer(24);
        auto *emoji = new wxStaticText(this, wxID_ANY, wxString::FromUTF8("📋"));
        wxFont emoji_font = emoji->GetFont();
        emoji_font.SetPointSize(emoji_font.GetPointSize() + 18);
        emoji->SetFont(emoji_font);
        root->Add(emoji, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, 12);
        auto *msg = new wxStaticText(this, wxID_ANY, wxString::FromUTF8(opts_.status), wxDefaultPosition,
                                     wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
        msg->Wrap(400);
        root->Add(msg, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT, 24);
        root->AddSpacer(20);
        root->Add(new wxStaticLine(this), 0, wxEXPAND | wxLEFT | wxRIGHT, 16);
        auto *btns = new wxBoxSizer(wxHORIZONTAL);
        btns->AddStretchSpacer(1);
        auto *ok = new wxButton(this, wxID_OK, wxString::FromUTF8("OK"));
        auto *cancel = new wxButton(this, wxID_CANCEL, wxString::FromUTF8("Cancel"));
        ok->SetDefault();
        ok->Bind(wxEVT_BUTTON, [this](wxCommandEvent &) {
            write_file(opts_.response_path, "");
            Finish(0);
        });
        cancel->Bind(wxEVT_BUTTON, [this](wxCommandEvent &) { Finish(1); });
        btns->Add(ok, 0, wxRIGHT, 8);
        btns->Add(cancel, 0);
        root->Add(btns, 0, wxEXPAND | wxALL, 16);
        SetSizer(root);
        Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent &evt) {
            if (exit_code_ < 0) {
                g_sopdlg_exit = 1;
                exit_code_ = 1;
            }
            evt.Skip();
        });
    }

    void BuildPaste() {
        auto *root = new wxBoxSizer(wxVERTICAL);
        auto *header = new wxBoxSizer(wxHORIZONTAL);
        header->Add(new wxStaticBitmap(this, wxID_ANY,
                                       wxArtProvider::GetBitmap(wxART_INFORMATION, wxART_OTHER, wxSize(32, 32))),
                    0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
        auto *col = new wxBoxSizer(wxVERTICAL);
        auto *title = new wxStaticText(this, wxID_ANY, wxString::FromUTF8("Chat response capture"));
        wxFont tf = title->GetFont();
        tf.MakeBold();
        title->SetFont(tf);
        title->SetForegroundColour(ios_accent());
        col->Add(title, 0, wxBOTTOM, 2);
        auto *hint = new wxStaticText(
            this, wxID_ANY,
            wxString::FromUTF8("Paste the model reply below, drop attachments, then Accept."));
        hint->SetForegroundColour(ios_muted());
        col->Add(hint, 0);
        header->Add(col, 1, wxEXPAND);
        root->Add(header, 0, wxEXPAND | wxALL, 12);
        root->Add(new wxStaticLine(this), 0, wxEXPAND | wxLEFT | wxRIGHT, 12);

        text_ = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                               wxTE_MULTILINE | wxTE_RICH2 | wxTE_PROCESS_TAB);
        root->Add(text_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);

        auto *paste_row = new wxBoxSizer(wxHORIZONTAL);
        auto *paste_btn = new wxButton(this, wxID_ANY, wxString::FromUTF8("Paste from clipboard"));
        paste_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent &) {
            if (!wxTheClipboard->Open()) {
                return;
            }
            if (wxTheClipboard->IsSupported(wxDF_TEXT)) {
                wxTextDataObject data;
                wxTheClipboard->GetData(data);
                text_->SetValue(data.GetText());
            }
            wxTheClipboard->Close();
        });
        paste_row->Add(paste_btn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
        const std::string save_label =
            opts_.save_as.empty() ? std::string("(no Save-As)") : ("Save-As: " + opts_.save_as);
        auto *save_as_lbl = new wxStaticText(this, wxID_ANY, wxString::FromUTF8(save_label));
        save_as_lbl->SetForegroundColour(ios_muted());
        paste_row->Add(save_as_lbl, 0, wxALIGN_CENTER_VERTICAL);
        root->Add(paste_row, 0, wxEXPAND | wxALL, 10);

        auto *files_title = new wxStaticText(this, wxID_ANY, wxString::FromUTF8("Files"));
        wxFont ff = files_title->GetFont();
        ff.MakeBold();
        files_title->SetFont(ff);
        root->Add(files_title, 0, wxLEFT | wxRIGHT, 10);

        files_ = new wxListBox(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 120));
        files_->SetDropTarget(new SopDlgFileDropTarget(this));
        root->Add(files_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);

        auto *btn_row = new wxBoxSizer(wxHORIZONTAL);
        btn_row->AddStretchSpacer(1);
        auto *accept = new wxButton(this, wxID_ANY, wxString::FromUTF8("Accept"));
        auto *cancel = new wxButton(this, wxID_CANCEL, wxString::FromUTF8("Cancel"));
        accept->SetDefault();
        accept->Bind(wxEVT_BUTTON, [this](wxCommandEvent &) { OnAccept(); });
        cancel->Bind(wxEVT_BUTTON, [this](wxCommandEvent &) { Finish(1); });
        btn_row->Add(accept, 0, wxRIGHT, 8);
        btn_row->Add(cancel, 0);
        root->Add(btn_row, 0, wxEXPAND | wxALL, 12);

        status_ = new wxStaticText(this, wxID_ANY, wxString::FromUTF8(opts_.status));
        status_->SetForegroundColour(ios_muted());
        root->Add(status_, 0, wxEXPAND | wxALL, 12);
        SetSizer(root);
        Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent &evt) {
            if (exit_code_ < 0) {
                g_sopdlg_exit = 1;
                exit_code_ = 1;
            }
            evt.Skip();
        });
    }

    void OnAccept() {
        const std::string body = std::string(text_->GetValue().ToUTF8());
        if (!write_file(opts_.response_path, body)) {
            wxMessageBox(wxString::FromUTF8("Failed to write response file."),
                         wxString::FromUTF8("Accept"), wxOK | wxICON_ERROR, this);
            return;
        }

        const std::string project =
            !opts_.project_dir.empty()
                ? opts_.project_dir
                : (getenv("SOP_PROJECT_DIR") ? std::string(getenv("SOP_PROJECT_DIR")) : std::string());

        /* Apply Save-As for non-empty response when running as pseudo-agent. */
        if (!opts_.save_as.empty() && !body.empty() && !project.empty()) {
            const fs::path dest = fs::path(project) / opts_.save_as;
            std::error_code ec;
            fs::create_directories(dest.parent_path(), ec);
            if (!write_file(dest.string(), body)) {
                wxMessageBox(wxString::FromUTF8("Failed to write Save-As path."),
                             wxString::FromUTF8("Accept"), wxOK | wxICON_ERROR, this);
                return;
            }
        }

        for (const auto &item : drops_) {
            const std::string content = read_file(item.src_path);
            if (content.empty() && !fs::exists(item.src_path)) {
                wxMessageBox(wxString::Format(wxString::FromUTF8("Missing dropped file:\n%s"),
                                              wxString::FromUTF8(item.src_path)),
                             wxString::FromUTF8("Accept"), wxOK | wxICON_ERROR, this);
                return;
            }
            std::string dest_path = item.dest;
            if (!project.empty() && dest_path.find('/') != 0) {
                dest_path = (fs::path(project) / item.dest).string();
            }
            if (!append_file(dest_path, content)) {
                wxMessageBox(wxString::Format(wxString::FromUTF8("Failed to append to %s"),
                                              wxString::FromUTF8(item.dest)),
                             wxString::FromUTF8("Accept"), wxOK | wxICON_ERROR, this);
                return;
            }
            if (item.delete_src) {
                std::error_code ec;
                fs::remove(item.src_path, ec);
            }
        }

        Finish(0);
    }

    SopDlgOpts opts_;
    int exit_code_ = -1;
    wxTextCtrl *text_ = nullptr;
    wxListBox *files_ = nullptr;
    wxStaticText *status_ = nullptr;
    std::vector<DroppedItem> drops_;
};

bool SopDlgFileDropTarget::OnDropFiles(wxCoord, wxCoord, const wxArrayString &filenames) {
    if (frame_) {
        frame_->AddDroppedFiles(filenames);
    }
    return true;
}

class SoptoolsApp : public wxApp {
public:
    bool OnInit() override {
        if (!wxApp::OnInit()) {
            return false;
        }
        wxInitAllImageHandlers();
        if (g_gui_mode == GuiEntryMode::SopDlg) {
            copy_to_clipboard(g_sopdlg.prompt);
            auto *frame = new SopDlgFrame(g_sopdlg);
            frame->Show(true);
            return true;
        }
        if (g_gui_mode == GuiEntryMode::Edit) {
            wxFrame *frame = create_sop_editor_frame(g_edit_sop_dir);
            if (!frame) {
                return false;
            }
            centre_on_same_display(frame, nullptr);
            frame->Show(true);
            return true;
        }
        if (!g_sop_engine) {
            return false;
        }
        wxFrame *frame = create_main_frame(g_sop_engine);
        centre_on_same_display(frame, nullptr);
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP_NO_MAIN(SoptoolsApp);

} /* namespace */

int run_gui_mode(SopEngine &engine, int argc, char **argv) {
    g_gui_mode = GuiEntryMode::Main;
    g_sop_engine = &engine;
    return wxEntry(argc, argv);
}

int run_sopdlg_mode(int argc, char **argv) {
    g_gui_mode = GuiEntryMode::SopDlg;
    g_sopdlg = SopDlgOpts{};
    enum { OPT_PASTE = 300, OPT_CONFIRM = 301, OPT_STAGE = 302, OPT_EXPECTED = 303 };
    std::vector<char *> args;
    args.push_back(const_cast<char *>("sopwin-sopdlg"));
    for (int i = 0; i < argc; i++) {
        args.push_back(argv[i]);
    }
    args.push_back(nullptr);
    optind = 1;
    static const struct option long_opts[] = {
        {"title", required_argument, nullptr, 't'},
        {"status", required_argument, nullptr, 's'},
        {"paste", no_argument, nullptr, OPT_PASTE},
        {"confirm", no_argument, nullptr, OPT_CONFIRM},
        {"stage-dir", required_argument, nullptr, OPT_STAGE},
        {"expected", required_argument, nullptr, OPT_EXPECTED},
        {"help", no_argument, nullptr, 'h'},
        {nullptr, 0, nullptr, 0},
    };
    const int ac = static_cast<int>(args.size()) - 1;
    char **av = args.data();
    for (;;) {
        const int c = getopt_long(ac, av, "t:s:h", long_opts, nullptr);
        if (c == -1) {
            break;
        }
        switch (c) {
        case 't':
            g_sopdlg.title = optarg;
            break;
        case 's':
            g_sopdlg.status = optarg;
            break;
        case OPT_PASTE:
            g_sopdlg.confirm = false;
            break;
        case OPT_CONFIRM:
            g_sopdlg.confirm = true;
            break;
        case OPT_STAGE:
            g_sopdlg.stage_dir = optarg;
            break;
        case OPT_EXPECTED:
            g_sopdlg.expected.push_back(optarg);
            break;
        case 'h':
            fputs("Usage: sopwin sopdlg [--paste|--confirm] [-t TITLE] [-s STATUS]\n"
                  "       sopwin copydlg …  (alias)\n"
                  "Env: PROMPT_FILE RESPONSE_FILE SOP_PROJECT_DIR SOP_SAVE_AS\n"
                  "     SOP_APPEND_TO SOP_PARSE SOP_FILE_LINK SOP_PARENT_DISPLAY\n",
                  stdout);
            return 0;
        default:
            return 1;
        }
    }
    if (const char *pf = getenv("PROMPT_FILE")) {
        if (pf[0]) {
            g_sopdlg.prompt = read_file(pf);
        }
    }
    if (g_sopdlg.prompt.empty()) {
        g_sopdlg.prompt = read_all_stdin();
    }
    if (const char *rf = getenv("RESPONSE_FILE")) {
        if (rf[0]) {
            g_sopdlg.response_path = rf;
        }
    }
    if (const char *pd = getenv("SOP_PROJECT_DIR")) {
        if (pd[0]) {
            g_sopdlg.project_dir = pd;
        }
    }
    if (const char *sa = getenv("SOP_SAVE_AS")) {
        if (sa[0]) {
            g_sopdlg.save_as = sa;
        }
    }
    if (const char *parse = getenv("SOP_PARSE")) {
        if (parse[0]) {
            g_sopdlg.parse = parse;
        }
    }
    if (const char *fl = getenv("SOP_FILE_LINK")) {
        if (fl[0]) {
            g_sopdlg.file_link = fl;
        }
    }
    if (const char *ap = getenv("SOP_APPEND_TO")) {
        if (ap[0]) {
            g_sopdlg.append_tos = parse_append_env(ap);
        }
    }
    if (const char *disp = getenv("SOP_PARENT_DISPLAY")) {
        if (disp[0]) {
            g_sopdlg.parent_display = disp;
        }
    }
    char *fake_argv[] = {const_cast<char *>("sopwin-sopdlg"), nullptr};
    int fake_argc = 1;
    const int rc = wxEntry(fake_argc, fake_argv);
    (void)rc;
    return g_sopdlg_exit;
}

int run_copydlg_mode(int argc, char **argv) {
    return run_sopdlg_mode(argc, argv);
}

int run_edit_mode(const std::string &sop_dir, int /*argc*/, char ** /*argv*/) {
    g_gui_mode = GuiEntryMode::Edit;
    g_edit_sop_dir = sop_dir;
    char *fake_argv[] = {const_cast<char *>("sopwin-edit"), nullptr};
    int fake_argc = 1;
    return wxEntry(fake_argc, fake_argv);
}
