/* *************************************************************************
 *     file.c：實現文件操作的相關功能。
 *     版權 (C) 2020-2024 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "gwm.h"

static char *dedup_paths(const char *paths);
static size_t get_files_in_path(const char *path, const char *regex, Strings *head, Order order, bool is_fullname);
static bool match(const char *s, const char *r, const char *os);
static bool regcmp(const char *s, const char *regex);
static int cmp_basename(const char *s1, const char *s2);
static void create_file_node(Strings *head, const char *path, char *filename, bool is_fullname);


Strings *get_files_in_paths(const char *paths, const char *regex, Order order, bool is_fullname, int *n)
{
    int sum=0;
    char *p=NULL, *ps=dedup_paths(paths);
    Strings *head=malloc_s(sizeof(Strings));

    head->next=NULL, head->str=NULL;
    for(p=strtok(ps, ":"); p; p=strtok(NULL, ":"))
        sum+=get_files_in_path(p, regex, head, order, is_fullname);
    free(ps);
    if(n)
        *n=sum;
    return head;
}

static char *dedup_paths(const char *paths)
{
    ssize_t readlink(const char *restrict, char *restrict, size_t);
    size_t n=1, i=0, j=0, len=strlen(paths);
    char *p=NULL, *ps=copy_string(paths);

    for(p=ps; *p; p++)
        if(*p == ':')
            n++;

    char list[n][len+1], buf[len+1], *result=malloc_s(len+1);
    for(*result='\0', p=strtok(ps, ":"); p; p=strtok(NULL, ":"))
    {
        ssize_t count=readlink(p, buf, len);
        char *pp = count>0 ? buf : p;
        for(j=0; j<i; j++)
            if(strcmp(pp, list[j])==0)
                break;
        if(j==i)
            strcpy(list[i++], pp), strcat(result, pp), strcat(result, i<n ? ":" : "");
    }
    free(ps);
    return result;
}

static size_t get_files_in_path(const char *path, const char *regex, Strings *head, Order order, bool is_fullname)
{
    size_t n=0;
    char *fn;
    DIR *dir=opendir(path);

    for(struct dirent *d=NULL; dir && (d=readdir(dir));)
        if(strcmp(".", fn=d->d_name) && strcmp("..", fn) && regcmp(fn, regex))
            for(Strings *f=head; f; f=f->next)
                if(!order || !f->next || cmp_basename(fn, f->next->str)/order>0)
                    { create_file_node(f, path, fn, is_fullname); n++; break; }
    if(dir)
        closedir(dir);
    return n;
}

static bool regcmp(const char *s, const char *regex) // 簡單的全文(即整個s)正則表達式匹配
{
    return match(s, regex, s);
}

// *匹配>=0個字符，.匹配一個字符，|匹配其兩側的表達式
static bool match(const char *s, const char *r, const char *os)
{
    char *p=NULL;
    switch(*r)
    {
        case '\0':  return !*s;
        case '*':   return match(s, r+1, os) || (*s && match(s+1, r, os));
        case '.':   return *s && match(s+1, r+1, os);
        case '|':   return (!*(r+1) && !*s) || (*(r+1) && match(os, r+1, os));
        default:    return (*r==*s && match(s+1, r+1, os))
                        || ((p=strchr(r, '|')) && match(os, p+1, os));
    }
}

static int cmp_basename(const char *s1, const char *s2)
{
    const char *p1=strrchr(s1, '/'), *p2=strrchr(s2, '/');
    p1=(p1 ? p1+1 : s1), p2=(p2 ? p2+1 : s2);
    return strcmp(p1, p2);
}

static void create_file_node(Strings *head, const char *path, char *filename, bool is_fullname)
{
    Strings *file=malloc_s(sizeof(Strings));
    file->str = is_fullname ? copy_strings(path, "/", filename, NULL) : copy_string(filename);
    file->next=head->next;
    head->next=file;
}

void exec_cmd(char *const *cmd)
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
    const char *sh_cmd[]={"/bin/sh", "-c", cfg->autostart, NULL};
    exec_cmd((char *const *)sh_cmd);
}

bool is_accessible(const char *filename)
{
    if(filename == NULL)
        return false;

    struct stat buf;
    return !stat(filename, &buf);
}
