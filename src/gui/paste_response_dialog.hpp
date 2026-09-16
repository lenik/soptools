#ifndef GUI_PASTE_RESPONSE_DIALOG_HPP
#define GUI_PASTE_RESPONSE_DIALOG_HPP

#include "engine/gpt_response.hpp"
#include "model/model.hpp"

#include <wx/dialog.h>

#include <string>
#include <vector>

class SopPasteResponseDialog : public wxDialog {
public:
    SopPasteResponseDialog(wxWindow *parent,
                           const std::string &project_dir,
                           const SopStep &step,
                           const std::string &stage_dir = {});

    /* Non-empty after successful Save. */
    const SopGptSaveResult &save_result() const { return save_result_; }
    bool saved() const { return saved_; }

private:
    void SetDialogStatus(const wxString &text);
    void RebuildFileList();
    void OnTextChanged(wxCommandEvent &);
    void OnPasteClipboard(wxCommandEvent &);
    void OnSave(wxCommandEvent &);
    void OnCancel(wxCommandEvent &);
    void DoSave();

    std::string project_dir_;
    std::string stage_dir_;
    SopStep step_;
    std::vector<std::string> expected_files_;
    SopGptSaveResult save_result_;
    bool saved_ = false;

    class wxTextCtrl *text_ = nullptr;
    class wxListBox *files_ = nullptr;
    class wxButton *save_btn_ = nullptr;
    class wxStaticText *hint_label_ = nullptr;
    class wxStaticText *status_label_ = nullptr;
};

#endif /* GUI_PASTE_RESPONSE_DIALOG_HPP */
