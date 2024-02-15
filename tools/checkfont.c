/* *************************************************************************
 *     checkfont.c：實現检测指定字体时否包含指定字符或者检测哪些字体包含指定
 * 字符的功能。
 *     版權 (C) 2024 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>

FcFontSet *get_font_set(void)
{
    FcPattern *pat=FcNameParse((FcChar8 *)":");
    FcObjectSet *os=FcObjectSetCreate();
    FcObjectSetAdd(os, "family");
    FcFontSet *fs=FcFontList(NULL, pat, os);
    FcObjectSetDestroy(os);
    FcPatternDestroy(pat);
    return fs;
}

int get_utf8_codepoint(const char *str, uint32_t *codepoint)
{
    const uint8_t *p=(const uint8_t*)str;
    int len=0;
    
    if(*p < 0x80) // 單字節字符
        len=1, *codepoint=*p;
    else if((*p>>5) == 0x06) // 雙字節字符
        len=2, *codepoint=(*p & 0x1F)<<6 | (*(p+1) & 0x3F);
    else if((*p>>4) == 0x0E) // 三字節字符
        len=3, *codepoint=(*p & 0x0F)<<12 | (*(p+1) & 0x3F)<<6 | (*(p+2) & 0x3F);
    else if((*p>>3) == 0x1E) // 四字節字符
        len=4, *codepoint=(*p & 0x07)<<18 | (*(p+1) & 0x3F)<<12 | (*(p+2) & 0x3F)<<6 | (*(p+3) & 0x3F);
    else // 非utf8編碼字符
        len=0;

    return len;
}

bool check_font(const char *s, const char *fontname)
{
    bool result=false;
    uint32_t codepoint;
    Display *display=XOpenDisplay(NULL);
    int screen=DefaultScreen(display);
    XftFont *font=XftFontOpenName(display, screen, fontname);

    if(font == NULL)
    {
        fprintf(stderr, "%s不存在", fontname);
        return false;
    }

    get_utf8_codepoint(s, &codepoint);
    if((result=XftCharExists(display, font, codepoint)))
        printf("%s: %s\n", fontname, s);
    XftFontClose(display, font);
    XCloseDisplay(display);
    return result;
}

bool check_fonts(const char *s)
{
    bool result=false;
    const FcChar8 *fmt=(const FcChar8 *)"%{=fclist}";
    FcFontSet *fs=get_font_set();

    for(int i=0; i<fs->nfont; i++)
        if(check_font(s, (char *)FcPatternFormat(fs->fonts[i], fmt)))
            result=true;
    return result;
}

int main(int argc, char **argv)
{
    if(argc == 3)
        return !check_font(argv[1], argv[2]);
    else if(argc == 2)
        return !check_fonts(argv[1]);
    else
    {
        fprintf(stderr, "用法：%s <待測字符> [字體名稱]\n", argv[0]);
        exit(1);
    }
}
