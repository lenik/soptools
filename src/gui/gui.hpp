#ifndef GUI_HPP
#define GUI_HPP

#include "engine/engine.hpp"

#include <string>

int run_gui_mode(SopEngine &engine, int argc, char **argv);
int run_copydlg_mode(int argc, char **argv);
int run_edit_mode(const std::string &sop_dir, int argc, char **argv);
class wxFrame;
wxFrame *create_sop_editor_frame(const std::string &sop_dir);

#endif /* GUI_HPP */
