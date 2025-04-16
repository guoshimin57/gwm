/* *************************************************************************
 *     file.h：與file.c相應的頭文件。
 *     版權 (C) 2020-2025 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef FILE_H 
#define FILE_H 

#include "misc.h"

#define SH_CMD(cmd_str) ((char *const []){"/bin/sh", "-c", (char *const)cmd_str, NULL})

Strings *get_files_in_paths(const char *paths, const char *regex, bool fullname);
void exec_cmd(char *const cmd[]);
void exec_autostart(void);
bool is_accessible(const char *filename);

#endif
