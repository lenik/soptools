#ifndef MODEL_STEP_INTERACT_HPP
#define MODEL_STEP_INTERACT_HPP

#include "model/model.hpp"

#include <string>
#include <vector>

struct SopSelectOption {
    std::string text; /* raw option text (may contain \response) */
};

struct SopSelectSpec {
    std::string title;
    std::vector<SopSelectOption> options;
    size_t start = 0; /* byte offset of <select in body */
    size_t end = 0;   /* byte offset after </select> */
};

struct SopOptionalSpec {
    std::string title;
    std::string body; /* inner markdown (may contain \response) */
    size_t start = 0;
    size_t end = 0;
};

std::vector<SopSelectSpec> parse_selects(const std::string &body);
std::vector<SopOptionalSpec> parse_optionals(const std::string &body);

/* Expand \response using nearest prior chat step response content. */
std::string expand_response_token(const std::string &text, const std::string &prior_chat_response);

/*
 * Replace each <select>…</select> with the chosen option text (already expanded).
 * answers[i] is the selected option index for selects[i].
 */
std::string apply_select_answers(const std::string &body,
                                 const std::vector<SopSelectSpec> &selects,
                                 const std::vector<int> &answers,
                                 const std::string &prior_chat_response);

/*
 * include[i]==true → replace <optional> with expanded inner body;
 * false → remove the block.
 */
std::string apply_optional_answers(const std::string &body,
                                   const std::vector<SopOptionalSpec> &optionals,
                                   const std::vector<bool> &include,
                                   const std::string &prior_chat_response);

/* Apply selects + optionals from the end so offsets stay valid. */
std::string apply_step_interactions(const std::string &body,
                                    const std::vector<SopSelectSpec> &selects,
                                    const std::vector<int> &select_answers,
                                    const std::vector<SopOptionalSpec> &optionals,
                                    const std::vector<bool> &optional_include,
                                    const std::string &prior_chat_response);

/* Walk active_steps upward from before current to find nearest chat response body. */
std::string find_nearest_chat_response(const SopDefinition &def,
                                       const std::vector<std::string> &active_steps,
                                       size_t current_index,
                                       const std::string &project_dir);

#endif /* MODEL_STEP_INTERACT_HPP */
