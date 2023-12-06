#include <ft2build.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include "unistd.h"
#include <locale.h>
#include <wchar.h>
#include <stdexcept>
#include <xgetopt.h>
#include <fcntl.h>

using namespace std;
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H
#define _CRT_SECURE_NO_DEPRECATE 

#define WIDTH   80
#define HEIGHT  80
/* origin is the upper left corner */

typedef unsigned short WORD;

FT_Face face = NULL;
float scale = 0.015625;

char x[1000] = { 0 };
const char* strs;

const char* add(char* str) {

    strcat(x, str);
    strs = x;
    return strs;
}


int moveto(const FT_Vector* to, void* user) {

    char str[20] = { 0 };
    printf("M%.2f %.2f", (to->x) * scale, (to->y) * scale);
    sprintf(str, "M%.2f %.2f", (to->x) * scale, (to->y) * scale);
    add(str);
    return 0;
}

int lineto(const FT_Vector* to, void* user) {

    char str[20] = { 0 };
    printf("L%.2f %.2f", (to->x) * scale, (to->y) * scale);
    sprintf(str, "L%.2f %.2f", (to->x) * scale, (to->y) * scale);
    add(str);
    return 0;
}

int  conicto(const FT_Vector* control, const FT_Vector* to, void* user) {

    char str[30] = { 0 };
    printf("Q%.2f %.2f %.2f %.2f", control->x * scale, control->y * scale, (to->x) * scale, (to->y) * scale);
    sprintf(str, "Q%.2f %.2f %.2f %.2f", control->x * scale, control->y * scale, (to->x) * scale, (to->y) * scale);
    add(str);
    return 0;
}

int  cubicto(const FT_Vector* control1, const FT_Vector* control2, const FT_Vector* to, void* user) {

    char str[35] = { 0 };
    printf("C%.2f %.2f %.2f %.2f %.2f %.2f", control1->x * scale, control1->y * scale, control2->x * scale, control2->y * scale, (to->x) * scale, (to->y) * scale);
    sprintf(str, "C%.2f %.2f %.2f %.2f %.2f %.2f", control1->x * scale, control1->y * scale, control2->x * scale, control2->y * scale, (to->x) * scale, (to->y) * scale);
    add(str);
    return 0;
}

//解析字形到Path，从truetype字体文件中提取出贝塞尔曲线
void ExtractOutline() {

    FT_Outline_Funcs callbacks;

    callbacks.move_to = moveto;
    callbacks.line_to = lineto;
    callbacks.conic_to = conicto;
    callbacks.cubic_to = cubicto;
    callbacks.shift = 0;
    callbacks.delta = 0;

    FT_GlyphSlot slot = face->glyph;
    FT_Outline& outline = slot->outline;
    FT_Error error = FT_Outline_Decompose(&outline, &callbacks, &ExtractOutline);
    if (error)
    {
        printf("不可以提取轮廓");
    }
}

class FreeTypeLibrary
{
public:
    FreeTypeLibrary();
    ~FreeTypeLibrary();

    operator FT_Library() const;

private:
    FreeTypeLibrary(const FreeTypeLibrary&);
    FreeTypeLibrary& operator =(const FreeTypeLibrary&);

private:
    FT_Library m_ftLibrary;
};


inline
FreeTypeLibrary::FreeTypeLibrary()
{
    FT_Error error = FT_Init_FreeType(&m_ftLibrary);

    if (error)
        throw std::runtime_error::runtime_error("Couldn't initialize the library:"
            " FT_Init_FreeType() failed");
}

inline
FreeTypeLibrary::~FreeTypeLibrary()
{
    FT_Done_FreeType(m_ftLibrary);
}


inline
FreeTypeLibrary::operator FT_Library() const
{
    return m_ftLibrary;
}


// Another minimal wrapper for RAII.
class FreeTypeFace
{
public:
    FreeTypeFace(const FreeTypeLibrary& library,
        const char* filename);
    ~FreeTypeFace();

    operator FT_Face() const;

private:
    FreeTypeFace(const FreeTypeFace&);
    FreeTypeFace& operator =(const FreeTypeFace&);

private:
    FT_Face m_ftFace;
};


inline
FreeTypeFace::FreeTypeFace(const FreeTypeLibrary& library,
    const char* filename)
{
    // For simplicity, always use the first face index.
    FT_Error error = FT_New_Face(library, filename, 0, &m_ftFace);

    if (error)
        throw runtime_error("Couldn't load the font file:"
            " FT_New_Face() failed");
}

inline
FreeTypeFace::~FreeTypeFace()
{
    FT_Done_Face(m_ftFace);
}

inline
FreeTypeFace::operator FT_Face() const
{
    return m_ftFace;
}

void usage(){
    printf("Usage:\n");
    printf("\tProject [-h] [-if a.txt] [-of out.svg]");
}

//int getopt(int argc, char* const argv[], const char* optstring);
//控制台命令
void testFun2(int argc, char* argv[]) {
    int o;
    const char* optstring = "hi::o::"; 
    while ((o = xgetopt(argc, argv, optstring)) != -1) {
        switch (o) {
        case 'h':
            printf("Usage:\nProject [-i a.txt] [-o out.svg]\n");
            break;
        case 'i':
            printf("inputfile is %s\n", xoptarg);
            break;
        case 'o':
            printf("outputfile is %s\n",xoptarg);
            break;
        case '?':
            printf("发生错误时提示用户正确的使用方式\n");
            usage(); // 提示使用说明
            break;
        }
    }
}


int main(int argc, char* argv[])
{
    FT_Library library;
    FT_Face face;
   
    FT_Glyph      glyph;
    FT_Matrix     matrix;                 // transformation matrix 
    FT_Vector   delta;
    const FT_Fixed multiplier = 65536L;
    FT_Vector     pen;                    // untransformed origin  
    FT_Error      error;

    double        angle;
 
    error = FT_Init_FreeType(&library);

    error = FT_New_Face(library,
                        "C:/Windows/Fonts/arial.ttf",
                        0,
                        &face);

    error = FT_Set_Pixel_Sizes( face,   /* handle to face object */
                                0,      /* pixel_width           */
                                16);   /* pixel_height          */

    //翻转字体，设置原点
    //float angle = ( 360.0 / 360 ) * 3.14159 * 2;
    //angle = (0.0 / 360) * 3.14159 * 2;      /* use 25 degrees     */ 
    matrix.xx = 1L * multiplier;;
    matrix.xy = 0L * multiplier;
    matrix.yx = 0L * multiplier;
    matrix.yy = -1L * multiplier;
    delta.x = 0;
    delta.y = 1100;
    FT_Set_Transform(face, &matrix, &delta);


    //指引
    fprintf(stderr, "This is a freetype based tool for extracting glyphs.\n");
    fprintf(stderr, "'Project [-h]':usage\n");
    //控制台输入
    testFun2(argc, argv);

    char* str = setlocale(LC_ALL, "zh_CN");//zh_CN.utf8
    if (str == NULL){
        fprintf(stderr, "Your system doesn't support Chinese!\n");
        return 4;
    }
    //写入
    char ch;
    char* f1 = argv[2];// 后台参数
    char* f2 = argv[4];
    FILE* fp1; // 创建文件指针及打开文本文件
    FILE* fp2;
    errno_t err;
    if ((err = fopen_s(&fp1, f1, "rb")) != 0) //以二进制方式读取文件f存于fp
    {
        printf("文件 %s 打开时发生错误", f1);
        return -1;
    }

    if ((err = fopen_s(&fp2, f2, "rb")) != 0) {
        //_wfopen(L"out.txt", L"w, ccs=utf-8")
        printf("文件 %s 打开时发生错误", f2);
        return -1;
    }
    ch = fgetc(fp1);

    rewind(fp1);

    // 添加字符
    //wchar_t* wszString = ;
    WORD word;
    int wszStringLen = wcslen(wszString);
    printf("wszStringLen: %d\n", wszStringLen);
    memcpy(&word, wszString, 2);

    // 通过索引,从face中加载字形
    FT_Load_Glyph(face, FT_Get_Char_Index(face, word), FT_LOAD_DEFAULT | FT_LOAD_NO_BITMAP);
    // 将字形图像(face->glyph)存到glyph变量里
    error = FT_Get_Glyph(face->glyph, &glyph);
    // 解析字形到Path
    ExtractOutline();

    //  free glyph 释放内存
    FT_Done_Glyph(glyph);
    glyph = NULL;

    //  free face
    FT_Done_Face(face);
    face = NULL;

    //  free FreeType Lib
    FT_Done_FreeType(library);
    library = NULL;

   
    while ((ch = fgetc(fp1)) != EOF) {
        //putchar(ch);
        printf("读取字符内容：%c", ch);
        
        fputc(ch, fp2);             //txt输出
        //fwprintf(fp2, L"%s", ch);
    }
    putchar('\n');

    if (ferror(fp1)) {
        puts("读取出错");
    }
    else {
        puts("读取成功");
    }

    fclose(fp2);
    fclose(fp1);
    return 0;

}
