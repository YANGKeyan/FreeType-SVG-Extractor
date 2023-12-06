# FreeTypeSVGExtractor

This code is a program that uses the FreeType library. Its main function is to extract Bezier curves from TrueType font files, output the outline data as SVG paths, and support some console commands to specify the input and output files.

## FreeTypeSVGExtractor.c

The main program procedure is as follows:

Defined a series of variables and constants, including the font file path, canvas width and height, global variables face, scale, etc..

Defined a series of functions, such as moveto, lineto, conicto, cubicto, for processing different commands in the font outline, converting them to SVG paths and storing them in the string array x.

The ExtractOutline function is defined to parse the font outline by calling the FreeType library function FT_Outline_Decompose.

Defines the FreeTypeLibrary and FreeTypeFace classes for initializing and releasing the FreeType library and fonts.

The usage function is defined to display instructions for using the program.

The testFun2 function is defined to parse console command arguments using the xgetopt function.

In the main function, the FreeType library is initialized, the font file is loaded, the font size and transformation matrix are set, and then the font outline is parsed by the ExtractOutline function and output to the string array x.

Open the input file, read the characters in it, and then output to the output file.

Finally, the associated resources are released, including the FreeType library, fonts, and file pointers.

Note that the program requires console parameters such as input file (-i) and output file (-o) to be supplied when used. You can type Project -h in the console to see instructions for use.