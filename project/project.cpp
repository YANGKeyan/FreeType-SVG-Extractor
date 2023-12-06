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
#include <iostream>
#include <sstream>
#include <vector>

using namespace std;
#define _CRT_SECURE_NO_DEPRECATE 

#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H
#include FT_BBOX_H
#include FT_ADVANCES_H
#include FT_TRUETYPE_TABLES_H
#include FT_SFNT_NAMES_H
#include FT_TRUETYPE_IDS_H
#include FT_TYPES_H

#define WIDTH   80
#define HEIGHT  80
/* origin is the upper left corner */

// initial FreeTypeLibrary
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
inline FreeTypeLibrary::FreeTypeLibrary()
{
    FT_Error error = FT_Init_FreeType(&m_ftLibrary);

    if (error)
        throw std::runtime_error::runtime_error("Couldn't initialize the library:"
            " FT_Init_FreeType() failed");
}
inline FreeTypeLibrary::~FreeTypeLibrary()
{
    FT_Done_FreeType(m_ftLibrary);
}
inline FreeTypeLibrary::operator FT_Library() const
{
    return m_ftLibrary;
}


//initial FreeTypeFace
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
inline FreeTypeFace::FreeTypeFace(const FreeTypeLibrary& library, const char* filename)
{
    // For simplicity, always use the first face index.
    FT_Error error = FT_New_Face(library, filename, 0, &m_ftFace);

    if (error)
        throw runtime_error("Couldn't load the font file:"
            " FT_New_Face() failed");
}
inline FreeTypeFace::~FreeTypeFace()
{
    FT_Done_Face(m_ftFace);
}
inline FreeTypeFace::operator FT_Face() const
{
    return m_ftFace;
}


class OutlinePrinter{
public:
    OutlinePrinter(const char* filename);//字体文件
    int Run(const wchar_t* symbol, const wchar_t* symbol1);//字符
    void PrintfontSVG() const;
    void PrintSVG() const;
    void PrinthkernSVG() const;
    void Print() const;
private:
    OutlinePrinter(const OutlinePrinter&);
    OutlinePrinter& operator =(const OutlinePrinter&);

private:
    void LoadGlyph(const wchar_t* symbol) const; //load字符
    //void getGlyphName(const wchar_t* symbol) const;
    bool OutlineExists() const;
    void FlipOutline() const;
    void clear();
    void getPanose();
    void ExtractOutline();

    void FontDetails();

    void getAdvance(const wchar_t* symbol);
    void getHAdvance(const wchar_t* symbol);
    void getVAdvance(const wchar_t* symbol);
    void gethkerning(const wchar_t* symbol, const wchar_t* symbol1);
    

    void ComputeViewBox();

    static int MoveToFunction(const FT_Vector* to,
        void* user);
    static int LineToFunction(const FT_Vector* to,
        void* user);
    static int ConicToFunction(const FT_Vector* control,
        const FT_Vector* to,
        void* user);
    static int CubicToFunction(const FT_Vector* controlOne,
        const FT_Vector* controlTwo,
        const FT_Vector* to,
        void* user);

private:
    // These two lines initialize the library and the face;
    // the order is important!
    FreeTypeLibrary m_library;
    FreeTypeFace m_face;
   
    ostringstream m_path;
    ostringstream fontfamily;
    ostringstream fontstyle;
    ostringstream fontpanose;
    vector<int> panose1;
    ostringstream UnitsPerEM;
    ostringstream ascender1;
    ostringstream descender1;
    ostringstream alphabetic;
    //ostringstream unicode;

    wchar_t *unicode1;


    ostringstream adv1;
    ostringstream Hadv;
    ostringstream Vadv;

    char kern_g1[100];
    char kern_g2[100];
    int k;

    // These four variables are for the `viewBox' attribute.
    FT_Pos m_xMin;
    FT_Pos m_yMin;
    FT_Pos m_width;
    FT_Pos m_height;
};

inline OutlinePrinter::OutlinePrinter(const char* filename)
    : m_face(m_library, filename),
    m_xMin(0),
    m_yMin(0),
    m_width(0),
    m_height(0)
{
    // Empty body.
}


int OutlinePrinter::Run(const wchar_t* symbol, const wchar_t* symbol1)
{
    clear();
    LoadGlyph(symbol);

    // Check whether outline exists.
    bool outlineExists = OutlineExists();

    if (!outlineExists) // Outline doesn't exist.
        throw runtime_error("Outline check failed.\n"
            "Please, inspect your font file or try another one,"
            " for example LiberationSerif-Bold.ttf");
    
    FlipOutline();

    getPanose();

    ExtractOutline();

    FontDetails();

    getAdvance(symbol);
    getHAdvance(symbol);
    getVAdvance(symbol);
    gethkerning(symbol, symbol1);

    ComputeViewBox();

    //PrintfontSVG();
    //PrintSVG();
    //PrinthkernSVG();
    return 0;
}
void OutlinePrinter::clear() {
    fontfamily.str("");
    fontstyle.str("");
    fontpanose.str("");
    UnitsPerEM.str("");
    ascender1.str("");
    descender1.str("");
    alphabetic.str("");

    adv1.str("");
    Hadv.str("");
    Vadv.str("");
    m_path.str("");
}
void OutlinePrinter::LoadGlyph(const wchar_t* symbol) const
{
    FT_ULong code = symbol[0];
    // For simplicity, use the charmap FreeType provides by default;
    // in most cases this means Unicode.
    FT_UInt index = FT_Get_Char_Index(m_face, code);

    FT_Error error = FT_Load_Glyph(m_face, index, FT_LOAD_NO_SCALE | FT_LOAD_NO_BITMAP);
    if (error)
        throw runtime_error("Couldn't load the glyph: FT_Load_Glyph() failed");
    
}

bool OutlinePrinter::OutlineExists() const
{
    FT_Face face = m_face;
    FT_GlyphSlot slot = face->glyph;
    FT_Outline& outline = slot->outline;
   
    if (slot->format != FT_GLYPH_FORMAT_OUTLINE)
        return false; // Should never happen.  Just an extra check.

    if (outline.n_contours <= 0 || outline.n_points <= 0)
        return false; // Can happen for some font files.

    FT_Error error = FT_Outline_Check(&outline);

    return error == 0;
}
void OutlinePrinter::FlipOutline() const//围绕x轴翻转轮廓。
{
    const FT_Fixed multiplier = 65536L;

    FT_Matrix matrix;

    matrix.xx = 1L * multiplier;
    matrix.xy = 0L * multiplier;
    matrix.yx = 0L * multiplier;
    matrix.yy = -1L * multiplier;

    FT_Face face = m_face;
    FT_GlyphSlot slot = face->glyph;
    FT_Outline& outline = slot->outline;

    FT_Outline_Transform(&outline, &matrix);
}
void OutlinePrinter::getPanose() { 
    
   
    FT_Face face = m_face;
    FT_GlyphSlot slot = face->glyph;
    FT_Outline& outline = slot->outline; 

    vector<int> panose(10);
    auto table = static_cast<TT_OS2*>(FT_Get_Sfnt_Table(m_face, ft_sfnt_os2));
        if (table)
        for (int i = 0; i < 10; i++) {
            panose[i] = table->panose[i];
        }
        panose1 = panose;
}
void OutlinePrinter::ExtractOutline()
{
    m_path << "d='\n";

    FT_Outline_Funcs callbacks;

    callbacks.move_to = MoveToFunction;
    callbacks.line_to = LineToFunction;
    callbacks.conic_to = ConicToFunction;
    callbacks.cubic_to = CubicToFunction;

    callbacks.shift = 0;
    callbacks.delta = 0;

    FT_Face face = m_face;
    FT_GlyphSlot slot = face->glyph;
    FT_Outline& outline = slot->outline;

    FT_Error error = FT_Outline_Decompose(&outline, &callbacks, this);

    if (error)
        throw runtime_error("Couldn't extract the outline:"
            " FT_Outline_Decompose() failed");
    m_path <<"'";

   
}
void OutlinePrinter::FontDetails()
{
    FT_Face face = m_face;
    FT_GlyphSlot slot = face->glyph;
    FT_Outline& outline = slot->outline;

    //printf("family name: %s\n,style name : %s\n",face->family_name,face->style_name);

    fontfamily << face->family_name;
    fontstyle << face->style_name;
    UnitsPerEM << face->units_per_EM;
    ascender1 << face->ascender;
    descender1 << face->descender;
    alphabetic << face->underline_position;
}
void OutlinePrinter::getAdvance(const wchar_t* symbol){
    FT_Face face = m_face;
    FT_ULong code = symbol[0];
    FT_UInt index = FT_Get_Char_Index(m_face, code);
    unicode1 = (wchar_t *)symbol;
    FT_Fixed adv = 0;
    FT_Get_Advance(m_face, index, FT_LOAD_NO_SCALE, &adv);
    adv1 << adv;
}
void OutlinePrinter::getHAdvance(const wchar_t* symbol) {
    FT_Face face = m_face;
    FT_ULong code = symbol[0];
    FT_UInt index = FT_Get_Char_Index(m_face, code);

    auto table = static_cast<TT_OS2*>(FT_Get_Sfnt_Table(m_face, ft_sfnt_os2));
    Hadv << table->xAvgCharWidth;
}
void OutlinePrinter::getVAdvance(const wchar_t* symbol) {
    FT_Face face = m_face;
    FT_ULong code = symbol[0];
    FT_UInt index = FT_Get_Char_Index(m_face, code);

    FT_Load_Glyph(m_face, index, FT_LOAD_NO_SCALE);
    if (FT_HAS_VERTICAL(face))
        //return face->glyph->metrics.vertAdvance;
    //return _currentFace->glyph->metrics.horiAdvance;
        Vadv << face->glyph->metrics.vertAdvance;
}
void OutlinePrinter::gethkerning(const wchar_t* symbol, const wchar_t* symbol1) {
    FT_Face face = m_face;
    FT_ULong code = symbol[0];
    FT_ULong code1 = symbol1[0];

    FT_UInt glyph_index = FT_Get_Char_Index(face, code);
    FT_UInt prev_index = FT_Get_Char_Index(face, code1);

    FT_Get_Glyph_Name(face, glyph_index, (FT_Pointer)kern_g1, 100);
    FT_Get_Glyph_Name(face, prev_index, (FT_Pointer)kern_g2, 100);
    unicode1 = (wchar_t*)symbol;
        FT_Vector delta;
        FT_Get_Kerning(face, glyph_index, prev_index, FT_KERNING_DEFAULT, &delta);
        k = delta.x;
    }

void OutlinePrinter::ComputeViewBox()
{
    FT_Face face = m_face;
    FT_GlyphSlot slot = face->glyph;
    FT_Outline& outline = slot->outline;

    FT_BBox boundingBox;

    FT_Outline_Get_BBox(&outline, &boundingBox);  

    FT_Pos xMin = boundingBox.xMin;
    FT_Pos yMin = boundingBox.yMin;
    FT_Pos xMax = boundingBox.xMax;
    FT_Pos yMax = boundingBox.yMax;

    m_xMin = xMin;
    m_yMin = yMin;
    m_width = xMax - xMin;
    m_height = yMax - yMin;
}
void OutlinePrinter::PrintfontSVG() const
{
    cout << "<?xml version='1.0'    standalone='no'?>\n"
        "<svg wide='100%' height='100%' xmlns='http://www.w3.org/2000/svg' xmlns:xlink='http://www.w3.org/1999/xlink'\n"
        "<defs >\n"

        "<font horiz-adv-x='" << adv1.str()
        << "' id='" << "ascii" << "'>\n"
        "   <font-face\n"
        "       <font-family='" << fontfamily.str() << "'\n"
        "       <font-style='" << fontstyle.str() << "'\n"
        "       <units-per-em='" << UnitsPerEM.str() << "'\n"
        "       <panose-1='" << panose1[0] << " " << panose1[1] << " " << panose1[2] << " " << panose1[3] << " "
        << panose1[4] << " " << panose1[5] << " " << panose1[6] << " " << panose1[7] << " "
        << panose1[8] << " " << panose1[9] << "'\n"
        "       <ascent='" << ascender1.str() << "'\n"
        "       <descent='" << descender1.str() << "'\n"

        "       <alphabetic='" << alphabetic.str() << "'  />"
        << endl;
}
void OutlinePrinter::PrintSVG() const
{
    wcout << "<glyph unicode='" << unicode1; //
       cout << "' glyph-name='" << kern_g1   //
        << "' horiz-adv-x='" << Hadv.str()
        << "' vert-adv-y='" << Vadv.str()
        << "' viewBox='"
        << m_xMin << ' ' << m_yMin << ' ' << m_width << ' ' << m_height<<"'"
        << m_path.str()
        << endl;
}
void OutlinePrinter::PrinthkernSVG() const
{

    cout << "<hkern g1='" << kern_g1   //
        << "' g2='" << kern_g2
        << "' k='" << k
        << "' />\n";
        
}
void OutlinePrinter::Print() const
{
    cout<< "</svg>"
        << endl;
}
int OutlinePrinter::MoveToFunction(const FT_Vector* to, void* user)
{
    OutlinePrinter* self = static_cast<OutlinePrinter*>(user);

    FT_Pos x = to->x;
    FT_Pos y = to->y;

    self->m_path << "           "
        "M " << x << ' ' << y << '\n';

    return 0;
}
int OutlinePrinter::LineToFunction(const FT_Vector* to, void* user)
{
    OutlinePrinter* self = static_cast<OutlinePrinter*>(user);

    FT_Pos x = to->x;
    FT_Pos y = to->y;

    self->m_path << "           "
        "L " << x << ' ' << y << '\n';

    return 0;
}
int OutlinePrinter::ConicToFunction(const FT_Vector* control, const FT_Vector* to, void* user)
{
    OutlinePrinter* self = static_cast<OutlinePrinter*>(user);

    FT_Pos controlX = control->x;
    FT_Pos controlY = control->y;

    FT_Pos x = to->x;
    FT_Pos y = to->y;

    self->m_path << "           "
        "Q " << controlX << ' ' << controlY << ", "
        << x << ' ' << y << '\n';

    return 0;
}
int OutlinePrinter::CubicToFunction(const FT_Vector* controlOne, const FT_Vector* controlTwo, const FT_Vector* to, void* user)
{
    OutlinePrinter* self = static_cast<OutlinePrinter*>(user);

    FT_Pos controlOneX = controlOne->x;
    FT_Pos controlOneY = controlOne->y;

    FT_Pos controlTwoX = controlTwo->x;
    FT_Pos controlTwoY = controlTwo->y;

    FT_Pos x = to->x;
    FT_Pos y = to->y;

    self->m_path << "           "
        "C " << controlOneX << ' ' << controlOneY << ", "
        << controlTwoX << ' ' << controlTwoY << ", "
        << x << ' ' << y << '\n';

    return 0;
}


void usage() {
    printf("Usage:\n");
    printf("\tProject [-f font] [-i a.txt] [> out.svg]");
}

//控制台命令
void testFun2(int argc, char* argv[]) {
    int o;
    const char* optstring = "hf::i::";
    while ((o = xgetopt(argc, argv, optstring)) != -1) {
        switch (o) {
        case 'h':
            printf("Usage:\nProject [-f font] [-i a.txt] [> out.svg]\n");
            break;
        case 'i':
            //printf("inputfile is %s\n", xoptarg);
            break;
        case 'f':
            //printf("fontfile is %s\n", xoptarg);
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

    //指引
    fprintf(stderr, "This is a freetype based tool for extracting glyphs.\n \tin the SVG format to stdout.\n");
    fprintf(stderr, "Try 'Project [-h]' to get help.\n");
    //控制台输入
    testFun2(argc, argv);

    char* f1 = argv[4];// 后台参数 打开文本文件
    FILE* fp1; // 创建文件指针及打开文本文件
    errno_t err;

    if ((err = fopen_s(&fp1, f1, "r")) != 0)
    {
        printf("文件 %s 打开时发生错误", f1);
        return -1;
    }

    char* str = setlocale(LC_ALL, "zh_CN");//zh_CN.utf8
    if (str == NULL) {
        fprintf(stderr, "Your system doesn't support Chinese!\n");
        return 4;
    }

    rewind(fp1);

    wchar_t str1[101];  //wchar_t *str1 = new wchar_t[100];
    fwscanf(fp1, L"%s", str1);
    fclose(fp1);

    const char* filename = argv[2];
    OutlinePrinter printer(filename);
    

    for (int i = 0; i < wcslen(str1); i++) {      //for (i = 0; str1[i]!= '\0'; i++) {

        //printf("输出第%d个汉字：", (i + 1));
        //wprintf(L"%lc\n", str1[i]);
        
        wchar_t symbol[51];   // const ֵsymbol
        wchar_t symbol1[51];
        wcscpy(symbol, &str1[i]);
        wcscpy(symbol1, &str1[i+1]);

        printer.Run(symbol, symbol1);

        if (i == 0) {
            printer.PrintfontSVG();
        }
       
        printer.PrintSVG();
        printer.PrinthkernSVG();
        printer.Print();

    }
    //if (ferror(fp1)) {puts("读取出错");}
    //else {puts("读取成功");}
}
