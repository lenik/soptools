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
                           const SopStep &step);

    /* Non-empty after successful Save. */
    const SopGptSaveResult &save_result() const { return save_result_; }
    bool saved() const { return saved_; }

private:
    void SetDialogStatus(const wxString &text);
    void RebuildLinkList();
    void OnTextChanged(wxCommandEvent &);
    void OnPasteClipboard(wxCommandEvent &);
    void OnDownload(wxCommandEvent &);
    void OnSave(wxCommandEvent &);
    void OnCancel(wxCommandEvent &);
    void StartDownloads(bool then_save);
    void FinishDownloadsAndMaybeSave();
    void MarkLinkDone(size_t index, bool ok, const std::string &path, const std::string &err);
    void DoSave();

    std::string project_dir_;
    SopStep step_;
    std::vector<SopGptAttachment> attachments_;
    SopGptSaveResult save_result_;
    bool saved_ = false;
    bool downloading_ = false;
    bool save_after_download_ = false;
    int pending_downloads_ = 0;

    class wxTextCtrl *text_ = nullptr;
    class wxListBox *links_ = nullptr;
    class wxButton *download_btn_ = nullptr;
    class wxButton *save_btn_ = nullptr;
    class wxStaticText *hint_label_ = nullptr;
    class wxStaticText *status_label_ = nullptr;
};

#endif /* GUI_PASTE_RESPONSE_DIALOG_HPP */
