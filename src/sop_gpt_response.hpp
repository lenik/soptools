#ifndef SOP_GPT_RESPONSE_HPP
#define SOP_GPT_RESPONSE_HPP

#include "sop_model.hpp"

#include <string>
#include <utility>
#include <vector>

struct SopGptPart {
    std::string label; /* "Full", "Part 1", ... */
    std::string relative_path;
    std::string absolute_path;
    std::string content;
};

struct SopGptAttachment {
    std::string url;
    std::string relative_path;
    std::string absolute_path;
    bool downloaded = false;
    std::string error;
};

struct SopGptSavedFile {
    std::string relative_path;
    std::string absolute_path;
};

struct SopGptSaveResult {
    bool ok = false;
    std::string message;
    std::string seq_dir_rel; /* e.g. sop/20 */
    std::string response_rel;
    std::string response_abs;
    std::string full_content;
    std::vector<SopGptPart> parts; /* includes Full as first */
    std::vector<SopGptAttachment> attachments;
    std::vector<SopGptSavedFile> files;
};

std::string sop_seq_dir_name(int seq);
std::string sniff_response_extension(const std::string &text);
std::vector<std::string> extract_download_urls(const std::string &text);
std::vector<std::pair<std::string, std::string>> split_multi_parts(const std::string &text);
bool download_url_to_file(const std::string &url, const std::string &abs_path, std::string *err);

SopGptSaveResult save_gpt_response(const std::string &project_dir,
                                   const SopStep &step,
                                   const std::string &response_text,
                                   const std::vector<SopGptAttachment> &downloaded);

#endif /* SOP_GPT_RESPONSE_HPP */
