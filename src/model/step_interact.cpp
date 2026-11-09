/*
 * Copyright (C) 2026 Lenik <soptools@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "model/step_interact.hpp"
#include "engine/gpt_response.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace {

std::string trim_copy(std::string s) {
    while (!s.empty() && (s.back() == '\r' || s.back() == '\n' || s.back() == ' ' || s.back() == '\t')) {
        s.pop_back();
    }
    size_t i = 0;
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) {
        i++;
    }
    if (i) {
        s.erase(0, i);
    }
    return s;
}

std::string parse_title_attr(const std::string &open_tag, const std::string &fallback) {
    const std::string key = "title=\"";
    const size_t p = open_tag.find(key);
    if (p == std::string::npos) {
        return fallback;
    }
    const size_t start = p + key.size();
    const size_t end = open_tag.find('"', start);
    if (end == std::string::npos) {
        return fallback;
    }
    return open_tag.substr(start, end - start);
}

struct Replacement {
    size_t start = 0;
    size_t end = 0;
    std::string text;
};

} /* namespace */

std::vector<SopSelectSpec> parse_selects(const std::string &body) {
    std::vector<SopSelectSpec> out;
    size_t pos = 0;
    while (pos < body.size()) {
        const size_t open = body.find("<select", pos);
        if (open == std::string::npos) {
            break;
        }
        const size_t open_end = body.find('>', open);
        if (open_end == std::string::npos) {
            break;
        }
        const size_t close = body.find("</select>", open_end);
        if (close == std::string::npos) {
            break;
        }
        SopSelectSpec spec;
        spec.start = open;
        spec.end = close + 9;
        spec.title = parse_title_attr(body.substr(open, open_end - open + 1), "Choose");
        const std::string inner = body.substr(open_end + 1, close - (open_end + 1));
        size_t ip = 0;
        while (ip < inner.size()) {
            const size_t op = inner.find("<option", ip);
            if (op == std::string::npos) {
                break;
            }
            const size_t oe = inner.find('>', op);
            if (oe == std::string::npos) {
                break;
            }
            size_t text_start = oe + 1;
            size_t text_end = inner.find("<option", text_start);
            if (text_end == std::string::npos) {
                text_end = inner.size();
            }
            const size_t close_opt = inner.find("</option>", text_start);
            if (close_opt != std::string::npos && close_opt < text_end) {
                text_end = close_opt;
            }
            SopSelectOption opt;
            opt.text = trim_copy(inner.substr(text_start, text_end - text_start));
            if (!opt.text.empty()) {
                spec.options.push_back(opt);
            }
            ip = text_end;
        }
        out.push_back(spec);
        pos = spec.end;
    }
    return out;
}

std::vector<SopOptionalSpec> parse_optionals(const std::string &body) {
    std::vector<SopOptionalSpec> out;
    size_t pos = 0;
    while (pos < body.size()) {
        const size_t open = body.find("<optional", pos);
        if (open == std::string::npos) {
            break;
        }
        const size_t open_end = body.find('>', open);
        if (open_end == std::string::npos) {
            break;
        }
        const size_t close = body.find("</optional>", open_end);
        if (close == std::string::npos) {
            break;
        }
        SopOptionalSpec spec;
        spec.start = open;
        spec.end = close + 11;
        spec.title = parse_title_attr(body.substr(open, open_end - open + 1), "Optional");
        spec.body = trim_copy(body.substr(open_end + 1, close - (open_end + 1)));
        out.push_back(spec);
        pos = spec.end;
    }
    return out;
}

bool step_has_input_variables(const std::string &body) {
    return !parse_selects(body).empty() || !parse_optionals(body).empty();
}

bool step_has_input_variables(const SopStep &step) {
    return step_has_input_variables(step.body);
}

std::string expand_response_token(const std::string &text, const std::string &prior_chat_response) {
    std::string out = text;
    const std::string token = "\\response";
    size_t p = 0;
    while ((p = out.find(token, p)) != std::string::npos) {
        out.replace(p, token.size(), prior_chat_response);
        p += prior_chat_response.size();
    }
    return out;
}

std::string apply_select_answers(const std::string &body,
                                 const std::vector<SopSelectSpec> &selects,
                                 const std::vector<int> &answers,
                                 const std::string &prior_chat_response) {
    return apply_step_interactions(body, selects, answers, {}, {}, prior_chat_response);
}

std::string apply_optional_answers(const std::string &body,
                                   const std::vector<SopOptionalSpec> &optionals,
                                   const std::vector<bool> &include,
                                   const std::string &prior_chat_response) {
    return apply_step_interactions(body, {}, {}, optionals, include, prior_chat_response);
}

std::string apply_step_interactions(const std::string &body,
                                    const std::vector<SopSelectSpec> &selects,
                                    const std::vector<int> &select_answers,
                                    const std::vector<SopOptionalSpec> &optionals,
                                    const std::vector<bool> &optional_include,
                                    const std::string &prior_chat_response) {
    std::vector<Replacement> reps;
    for (size_t i = 0; i < selects.size(); i++) {
        Replacement r;
        r.start = selects[i].start;
        r.end = selects[i].end;
        if (i < select_answers.size() && select_answers[i] >= 0 &&
            static_cast<size_t>(select_answers[i]) < selects[i].options.size()) {
            r.text = expand_response_token(
                selects[i].options[static_cast<size_t>(select_answers[i])].text, prior_chat_response);
        }
        reps.push_back(std::move(r));
    }
    for (size_t i = 0; i < optionals.size(); i++) {
        Replacement r;
        r.start = optionals[i].start;
        r.end = optionals[i].end;
        if (i < optional_include.size() && optional_include[i]) {
            r.text = expand_response_token(optionals[i].body, prior_chat_response);
        }
        reps.push_back(std::move(r));
    }
    std::sort(reps.begin(), reps.end(),
              [](const Replacement &a, const Replacement &b) { return a.start > b.start; });

    std::string out = body;
    for (const auto &r : reps) {
        if (r.end > out.size() || r.start > r.end) {
            continue;
        }
        out.replace(r.start, r.end - r.start, r.text);
    }
    return expand_response_token(out, prior_chat_response);
}

std::string find_nearest_chat_response(const SopDefinition &def,
                                       const std::vector<std::string> &active_steps,
                                       size_t current_index,
                                       const std::string &project_dir) {
    if (current_index == 0 || current_index > active_steps.size()) {
        return {};
    }
    for (size_t i = current_index; i-- > 0;) {
        const std::string &id = active_steps[i];
        const auto it = def.steps.find(id);
        if (it == def.steps.end()) {
            continue;
        }
        const SopStep &step = it->second;
        if (step.role != SopRole::Chat) {
            continue;
        }
        const fs::path seq_dir = fs::path(project_dir) / "sop" / sop_seq_dir_name(step.seq);
        if (!fs::is_directory(seq_dir)) {
            continue;
        }
        const fs::path md = seq_dir / "response.md";
        if (fs::is_regular_file(md)) {
            std::ifstream in(md);
            std::ostringstream ss;
            ss << in.rdbuf();
            return ss.str();
        }
        for (const auto &entry : fs::directory_iterator(seq_dir)) {
            if (!entry.is_regular_file()) {
                continue;
            }
            const std::string name = entry.path().filename().string();
            if (name.rfind("response.", 0) == 0) {
                std::ifstream in(entry.path());
                std::ostringstream ss;
                ss << in.rdbuf();
                return ss.str();
            }
        }
    }
    return {};
}
