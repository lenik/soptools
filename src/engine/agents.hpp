#ifndef ENGINE_AGENTS_HPP
#define ENGINE_AGENTS_HPP

#include "model/model.hpp"

#include <map>
#include <string>

struct SopAgents {
    /* role slug -> cmdline */
    std::map<std::string, std::string> by_role;
};

/* Load <project>/sop/AGENTS; empty file or missing keys fall back to defaults. */
SopAgents load_agents(const std::string &project_dir);
std::string default_agent_cmdline(const std::string &role_slug);
std::string agent_cmdline_for_role(const SopAgents &agents, const std::string &role_slug);

struct SopAgentRunResult {
    bool ok = false;
    int exit_code = -1;
    std::string response_text;
    std::string message;
    std::string prompt_path;
    std::string response_path;
};

/*
 * Write prompt, fork agent with stdin←prompt stdout→response.
 * Env: PROGFILE LOGFILE LOGLEVEL KEEP_ALIVE ENV_UPDATE SOP_*.
 * Blocking; intended for a worker thread. progress_cb may be null.
 */
using SopAgentProgressFn = void (*)(void *user, double pct, const char *label);
SopAgentRunResult run_agent_blocking(const std::string &project_dir,
                                     const std::string &sop_dir,
                                     const SopStep &step,
                                     const std::string &cmdline,
                                     const std::string &prompt_text,
                                     int loglevel,
                                     void *progress_user,
                                     SopAgentProgressFn progress_cb,
                                     std::string *log_out);

#endif /* ENGINE_AGENTS_HPP */
