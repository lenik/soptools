#ifndef SOP_RENDER_RESPONSE_DIALOG_HPP
#define SOP_RENDER_RESPONSE_DIALOG_HPP

#include "sop_gpt_response.hpp"

#include <wx/dialog.h>

#include <string>

class SopRenderResponseDialog : public wxDialog {
public:
    SopRenderResponseDialog(wxWindow *parent, const SopGptSaveResult &result);

private:
    void RefreshView();
    void RefreshLabels();
    void CopySelection();
    void OnContentsSelect(wxCommandEvent &);
    void OnAttachSelect(wxCommandEvent &);
    void OnCopy(wxCommandEvent &);
    void OnClose(wxCommandEvent &);
    void OnAutoCopy(wxCommandEvent &);

    std::string LoadFileText(const std::string &path) const;
    wxString MarkdownToHtml(const std::string &md) const;
    bool CopyText(const wxString &text);

    SopGptSaveResult result_;
    int selected_part_ = 0;
    int selected_attach_ = -1;
    int clipboard_part_ = -1;
    int clipboard_attach_ = -1;
    bool auto_copy_ = false;

    class wxListBox *contents_ = nullptr;
    class wxHtmlWindow *html_ = nullptr;
    class wxListBox *attachments_ = nullptr;
    class wxCheckBox *auto_copy_box_ = nullptr;
};

#endif /* SOP_RENDER_RESPONSE_DIALOG_HPP */
