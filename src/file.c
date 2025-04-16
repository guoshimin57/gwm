/* *************************************************************************
 *     file.c：實現文件操作的相關功能。
 *     版權 (C) 2020-2025 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "misc.h"
#include "file.h"
#include "gwm.h"
#include "config.h"
#include "list.h"
#include <unistd.h>
#include <dirent.h>

static void add_files_in_path(Strings *head, const char *path, const char *regex, bool fullname);
static bool is_dir(const char *filename);
static bool regcmp(const char *s, const char *regex);
static bool match(const char *s, const char *r);
static void add_file(Strings *head, const char *path, const char *filename, bool fullname);
static Strings *create_file_node(const char *path, const char *filename, bool fullname);
static Strings *get_file_insert_point(Strings *head, Strings *file, bool order);
static int cmp_basename(const char *s1, const char *s2);

Strings *get_files_in_paths(const char *paths, const char *regex, bool fullname)
{
    char *p=NULL, *ps=copy_string(paths);
    Strings *files=Malloc(sizeof(Strings));

    list_init(&files->list);
    for(p=strtok(ps, ":"); p; p=strtok(NULL, ":"))
        add_files_in_path(files, p, regex, fullname);
    Free(ps);

    return files;
}

static void add_files_in_path(Strings *head, const char *path, const char *regex, bool fullname)
{
    DIR *dir=opendir(path);

    if(dir == NULL)
        return;

    for(struct dirent *d=NULL; (d=readdir(dir));)
    {
        if(strcmp(".", d->d_name) && strcmp("..", d->d_name))
        {
            char *fn=copy_strings(path, "/", d->d_name, NULL);
            if(is_dir(fn))
                add_files_in_path(head, fn, regex, fullname);
            else if(regcmp(d->d_name, regex))
                add_file(head, path, d->d_name, fullname);
            Free(fn);
        }
    }
    closedir(dir);
}

static bool is_dir(const char *filename)
{
    struct stat buf;
    return stat(filename, &buf)==0 && S_ISDIR(buf.st_mode);
}

/* 簡單的正則表達式匹配，僅支持：.、*、| */
static bool regcmp(const char *s, const char *regex)
{
    do
    {
        if(match(s, regex))
            return true;
    } while((regex=strchr(regex, '|')) && ++regex);

    return false;
}

/* 功能：匹配正則表達式r。
 * 說明：*匹配>=0個前一字符，.匹配一個字符，|匹配其兩側的表達式。
 */
static bool match(const char *s, const char *r)
{
    switch(*r)
    {
        case '\0': return !*s;
        case '*':  return match(s, r+1) || (*s && match(s+1, r));
        case '.':  return (*(r+1)=='*' && *s) ? match(s, r+1) : match(s+1, r+1);
        case '|':  return *r==*s ? match(s+1, r+1) : true;
        default:   return (*r==*s && match(s+1, r+1));
    }
}

/* fullname爲真時無序插入，否則按升序插入 */
static void add_file(Strings *head, const char *path, const char *filename, bool fullname)
{
    Strings *file=create_file_node(path, filename, fullname);
    Strings *ip=get_file_insert_point(head, file, !fullname);
    list_add(&file->list, &ip->list);
}

static Strings *create_file_node(const char *path, const char *filename, bool fullname)
{
    Strings *file=Malloc(sizeof(Strings));
    file->str = fullname ? copy_strings(path, "/", filename, NULL) : copy_string(filename);
    return file;
}

static Strings *get_file_insert_point(Strings *head, Strings *file, bool order)
{
    if(!order)
        return head;

    list_for_each_entry(Strings, p, &head->list, list)
        if(cmp_basename(file->str, p->str) <= 0)
            return list_prev_entry(p, Strings, list);

    return list_prev_entry(head, Strings, list);
}

static int cmp_basename(const char *s1, const char *s2)
{
    const char *p1=strrchr(s1, '/'), *p2=strrchr(s2, '/');
    p1=(p1 ? p1+1 : s1), p2=(p2 ? p2+1 : s2);
    return strcmp(p1, p2);
}

void exec_cmd(char *const cmd[])
{
    pid_t pid=fork();
	if(pid == 0)
    {
		if(xinfo.display)
            close(ConnectionNumber(xinfo.display));
		if(!setsid())
            perror(_("未能成功地爲命令創建新會話"));
		if(execvp(cmd[0], cmd) == -1)
            exit_with_perror(_("命令執行錯誤"));
    }
    else if(pid == -1)
        perror(_("未能成功地爲命令創建新進程"));
}

void exec_autostart(void)
{
    char cmd[BUFSIZ];
    sprintf(cmd, "[ -x '%s' ] && '%s'", cfg->autostart, cfg->autostart);
    exec_cmd(SH_CMD(cfg->autostart));
}

bool is_accessible(const char *filename)
{
    if(filename == NULL)
        return false;

    struct stat buf;
    return !stat(filename, &buf);
}
