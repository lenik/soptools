#ifndef UTIL_SUITE_META_HPP
#define UTIL_SUITE_META_HPP

#include <string>

/* Metadata for a shipped SOP suite (suite/<name>/). */
struct SuiteMeta {
    std::string name;    /* e.g. worldman */
    std::string version; /* from suite/<name>/VERSION; empty if unknown */
    std::string icon_png; /* resolved path to suite icon PNG; may be empty */
};

/* Infer suite name from .../suite/<name>/<branch>/ and load VERSION + icon. */
SuiteMeta load_suite_meta(const std::string &sop_dir);

/* "worldman 1.1" or just the name / "SOP" when unknown. */
std::string suite_badge_label(const SuiteMeta &meta);

#endif /* UTIL_SUITE_META_HPP */
