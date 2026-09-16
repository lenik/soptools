/*
 * Copyright (C) 2026 Lenik <soptools@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "gui/gui.hpp"
#include "gui/main_frame.hpp"
#include "gui/theme.hpp"

#include <wx/artprov.h>
#include <wx/button.h>
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/frame.h>
#include <wx/listbox.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/statline.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/wx.h>

#include <cstdlib>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

enum class GuiEntryMode {
    Main,
    CopyDlg,
    Edit,
};

GuiEntryMode g_gui_mode = GuiEntryMode::Main;
SopEngine *g_sop_engine = nullptr;
std::string g_edit_sop_dir;

struct CopyDlgOpts {
    std::string title = "sopwin copydlg";
    std::string status = "Prompt has been copied to the clipboard";
    bool confirm = false;
    std::string stage_dir;
    std::vector<std::string> expected;
    std::string prompt;
    std::string response_path;
};

CopyDlgOpts g_copydlg;
int g_copydlg_exit = 1;

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

bool copy_to_clipboard(const std::string &text) {
    if (!wxTheClipboard->Open()) {
        return false;
    }
    wxTheClipboard->SetData(new wxTextDataObject(wxString::FromUTF8(text)));
    wxTheClipboard->Close();
    return true;
}

class CopyDlgFrame : public wxFrame {
public:
    explicit CopyDlgFrame(const CopyDlgOpts &opts)
        : wxFrame(nullptr, wxID_ANY, wxString::FromUTF8(opts.title), wxDefaultPosition,
                  opts.confirm ? wxSize(480, 280) : wxSize(920, 740)),
          opts_(opts) {
        SetBackgroundColour(ios_bg());
        if (opts_.confirm) {
            BuildConfirm();
        } else {
            BuildPaste();
        }
        Centre();
    }

    int exit_code() const { return exit_code_; }

private:
    void Finish(int code) {
        exit_code_ = code;
        g_copydlg_exit = code;
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
                g_copydlg_exit = 1;
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
            wxString::FromUTF8("Paste the model reply below, then Save."));
        hint->SetForegroundColour(ios_muted());
        col->Add(hint, 0);
        header->Add(col, 1, wxEXPAND);
        root->Add(header, 0, wxEXPAND | wxALL, 12);
        root->Add(new wxStaticLine(this), 0, wxEXPAND | wxLEFT | wxRIGHT, 12);

        text_ = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                               wxTE_MULTILINE | wxTE_RICH2 | wxTE_PROCESS_TAB);
        root->Add(text_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);

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
        root->Add(paste_btn, 0, wxALL, 10);

        files_ = new wxListBox(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 100));
        for (const auto &rel : opts_.expected) {
            files_->Append(wxString::FromUTF8(rel));
        }
        root->Add(files_, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);

        auto *btn_row = new wxBoxSizer(wxHORIZONTAL);
        btn_row->AddStretchSpacer(1);
        auto *save = new wxButton(this, wxID_ANY, wxString::FromUTF8("Save"));
        auto *cancel = new wxButton(this, wxID_CANCEL, wxString::FromUTF8("Cancel"));
        save->Bind(wxEVT_BUTTON, [this](wxCommandEvent &) {
            const std::string body = std::string(text_->GetValue().ToUTF8());
            if (!write_file(opts_.response_path, body)) {
                return;
            }
            Finish(0);
        });
        cancel->Bind(wxEVT_BUTTON, [this](wxCommandEvent &) { Finish(1); });
        btn_row->Add(save, 0, wxRIGHT, 8);
        btn_row->Add(cancel, 0);
        root->Add(btn_row, 0, wxEXPAND | wxALL, 12);

        status_ = new wxStaticText(this, wxID_ANY, wxString::FromUTF8(opts_.status));
        status_->SetForegroundColour(ios_muted());
        root->Add(status_, 0, wxEXPAND | wxALL, 12);
        SetSizer(root);
        Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent &evt) {
            if (exit_code_ < 0) {
                g_copydlg_exit = 1;
                exit_code_ = 1;
            }
            evt.Skip();
        });
    }

    CopyDlgOpts opts_;
    int exit_code_ = -1;
    wxTextCtrl *text_ = nullptr;
    wxListBox *files_ = nullptr;
    wxStaticText *status_ = nullptr;
};

class SoptoolsApp : public wxApp {
public:
    bool OnInit() override {
        if (!wxApp::OnInit()) {
            return false;
        }
        if (g_gui_mode == GuiEntryMode::CopyDlg) {
            copy_to_clipboard(g_copydlg.prompt);
            auto *frame = new CopyDlgFrame(g_copydlg);
            frame->Show(true);
            return true;
        }
        if (g_gui_mode == GuiEntryMode::Edit) {
            wxFrame *frame = create_sop_editor_frame(g_edit_sop_dir);
            if (!frame) {
                return false;
            }
            frame->Show(true);
            return true;
        }
        if (!g_sop_engine) {
            return false;
        }
        wxFrame *frame = create_main_frame(g_sop_engine);
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

int run_copydlg_mode(int argc, char **argv) {
    g_gui_mode = GuiEntryMode::CopyDlg;
    g_copydlg = CopyDlgOpts{};
    enum { OPT_PASTE = 300, OPT_CONFIRM = 301, OPT_STAGE = 302, OPT_EXPECTED = 303 };
    std::vector<char *> args;
    args.push_back(const_cast<char *>("sopwin-copydlg"));
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
            g_copydlg.title = optarg;
            break;
        case 's':
            g_copydlg.status = optarg;
            break;
        case OPT_PASTE:
            g_copydlg.confirm = false;
            break;
        case OPT_CONFIRM:
            g_copydlg.confirm = true;
            break;
        case OPT_STAGE:
            g_copydlg.stage_dir = optarg;
            break;
        case OPT_EXPECTED:
            g_copydlg.expected.push_back(optarg);
            break;
        case 'h':
            fputs("Usage: sopwin copydlg [--paste|--confirm] [-t TITLE] [-s STATUS]\n", stdout);
            return 0;
        default:
            return 1;
        }
    }
    if (const char *pf = getenv("PROMPT_FILE")) {
        if (pf[0]) {
            g_copydlg.prompt = read_file(pf);
        }
    }
    if (g_copydlg.prompt.empty()) {
        g_copydlg.prompt = read_all_stdin();
    }
    if (const char *rf = getenv("RESPONSE_FILE")) {
        if (rf[0]) {
            g_copydlg.response_path = rf;
        }
    }
    char *fake_argv[] = {const_cast<char *>("sopwin-copydlg"), nullptr};
    int fake_argc = 1;
    const int rc = wxEntry(fake_argc, fake_argv);
    (void)rc;
    return g_copydlg_exit;
}

int run_edit_mode(const std::string &sop_dir, int /*argc*/, char ** /*argv*/) {
    g_gui_mode = GuiEntryMode::Edit;
    g_edit_sop_dir = sop_dir;
    char *fake_argv[] = {const_cast<char *>("sopwin-edit"), nullptr};
    int fake_argc = 1;
    return wxEntry(fake_argc, fake_argv);
}
