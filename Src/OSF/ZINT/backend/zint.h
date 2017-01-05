/*  zint.h - definitions for libzint

    libzint - the open source barcode library
    Copyright (C) 2009-2016 Robin Stuart <rstuart114@gmail.com>

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.
    3. Neither the name of the project nor the names of its contributors
       may be used to endorse or promote products derived from this software
       without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
    OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
    HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
    OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
    SUCH DAMAGE.
 */

#ifndef ZINT_H
#define ZINT_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct ZintRenderLine : public FPoint {
	float length;
	float width;
	ZintRenderLine * next; /* Pointer to next line */
};

struct ZintRenderString : public FPoint {
	float fsize;
	float width; // Suggested string width, may be 0 if none recommended
	int   length;
	uchar * text;
	ZintRenderString * next; // Pointer to next character
};

struct ZintRenderRing : public FPoint {
	float  radius;
	float  line_width;
	ZintRenderRing * next; // Pointer to next ring
};

struct ZintRenderHexagon : public FPoint {
	ZintRenderHexagon * next; // Pointer to next hexagon
};

struct ZintRender : public FPoint {
	// X - width
	// Y - height
	ZintRenderLine    * lines;    // Pointer to first line
	ZintRenderString  * strings;  // Pointer to first string
	ZintRenderRing    * rings;    // Pointer to first ring
	ZintRenderHexagon * hexagons; // Pointer to first hexagon
};

struct ZintSymbol {
	int    Std;               // BARCODE_XXX
	int    height;
	int    whitespace_width;
	int    border_width;
	int    output_options;
	char   fgcolour[10];
	char   bgcolour[10];
	char   outfile[256];
	float  scale;
	int    option_1;
	int    option_2;
	int    option_3;
	int    show_hrt;
	int    input_mode;
	uchar  text[128];
	int    rows;
	int    width;
	char   primary[128];
	uchar  encoded_data[178][143];
	int    row_height[178];   // Largest symbol is 177x177 QR Code
	char   errtxt[100];
	char * P_Bitmap;
	int    bitmap_width;
	int    bitmap_height;
	ZintRender * rendered;
};
//
// Tbarcode 7 codes
//
#define BARCODE_CODE11          1
#define BARCODE_C25MATRIX       2
#define BARCODE_C25INTER        3
#define BARCODE_C25IATA         4
#define BARCODE_C25LOGIC        6
#define BARCODE_C25IND          7
#define BARCODE_CODE39          8
#define BARCODE_EXCODE39        9
#define BARCODE_EANX            13
#define BARCODE_EAN128          16
#define BARCODE_CODABAR         18
#define BARCODE_CODE128         20
#define BARCODE_DPLEIT          21
#define BARCODE_DPIDENT         22
#define BARCODE_CODE16K         23
#define BARCODE_CODE49          24
#define BARCODE_CODE93          25
#define BARCODE_FLAT            28
#define BARCODE_RSS14           29
#define BARCODE_RSS_LTD         30
#define BARCODE_RSS_EXP         31
#define BARCODE_TELEPEN         32
#define BARCODE_UPCA            34
#define BARCODE_UPCE            37
#define BARCODE_POSTNET         40
#define BARCODE_MSI_PLESSEY     47
#define BARCODE_FIM             49
#define BARCODE_LOGMARS         50
#define BARCODE_PHARMA          51
#define BARCODE_PZN             52
#define BARCODE_PHARMA_TWO      53
#define BARCODE_PDF417          55
#define BARCODE_PDF417TRUNC     56
#define BARCODE_MAXICODE        57
#define BARCODE_QRCODE          58
#define BARCODE_CODE128B        60
#define BARCODE_AUSPOST         63
#define BARCODE_AUSREPLY        66
#define BARCODE_AUSROUTE        67
#define BARCODE_AUSREDIRECT     68
#define BARCODE_ISBNX           69
#define BARCODE_RM4SCC          70
#define BARCODE_DATAMATRIX      71
#define BARCODE_EAN14           72
#define BARCODE_CODABLOCKF      74
#define BARCODE_NVE18           75
#define BARCODE_JAPANPOST       76
#define BARCODE_KOREAPOST       77
#define BARCODE_RSS14STACK      79
#define BARCODE_RSS14STACK_OMNI 80
#define BARCODE_RSS_EXPSTACK    81
#define BARCODE_PLANET          82
#define BARCODE_MICROPDF417     84
#define BARCODE_ONECODE         85
#define BARCODE_PLESSEY         86
//
// Tbarcode 8 codes
//
#define BARCODE_TELEPEN_NUM     87
#define BARCODE_ITF14           89
#define BARCODE_KIX             90
#define BARCODE_AZTEC           92
#define BARCODE_DAFT            93
#define BARCODE_MICROQR         97
//
// Tbarcode 9 codes
//
#define BARCODE_HIBC_128        98
#define BARCODE_HIBC_39         99
#define BARCODE_HIBC_DM         102
#define BARCODE_HIBC_QR         104
#define BARCODE_HIBC_PDF        106
#define BARCODE_HIBC_MICPDF     108
#define BARCODE_HIBC_BLOCKF     110
#define BARCODE_HIBC_AZTEC      112
//
// Zint specific
//
#define BARCODE_AZRUNE          128
#define BARCODE_CODE32          129
#define BARCODE_EANX_CC         130
#define BARCODE_EAN128_CC       131
#define BARCODE_RSS14_CC        132
#define BARCODE_RSS_LTD_CC      133
#define BARCODE_RSS_EXP_CC      134
#define BARCODE_UPCA_CC         135
#define BARCODE_UPCE_CC         136
#define BARCODE_RSS14STACK_CC   137
#define BARCODE_RSS14_OMNI_CC   138
#define BARCODE_RSS_EXPSTACK_CC 139
#define BARCODE_CHANNEL         140
#define BARCODE_CODEONE         141
#define BARCODE_GRIDMATRIX      142

#define BARCODE_NO_ASCII        1
#define BARCODE_BIND            2
#define BARCODE_BOX             4
#define BARCODE_STDOUT          8
#define READER_INIT             16
#define SMALL_TEXT              32

#define DATA_MODE       0
#define UNICODE_MODE    1
#define GS1_MODE        2
#define KANJI_MODE      3
#define SJIS_MODE       4

#define DM_SQUARE       100
#define DM_DMRE 101

#define ZINT_WARN_INVALID_OPTION        2
#define ZINT_ERROR_TOO_LONG             5
#define ZINT_ERROR_INVALID_DATA 6
#define ZINT_ERROR_INVALID_CHECK        7
#define ZINT_ERROR_INVALID_OPTION       8
#define ZINT_ERROR_ENCODING_PROBLEM     9
#define ZINT_ERROR_FILE_ACCESS  10
#define ZINT_ERROR_MEMORY               11

ZintSymbol * ZBarcode_Create();
void ZBarcode_Clear(ZintSymbol * symbol);
void ZBarcode_Delete(ZintSymbol * symbol);
int ZBarcode_Encode(ZintSymbol * symbol, const uchar * input, int length);
int ZBarcode_Encode_File(ZintSymbol * symbol, const char * filename);
int ZBarcode_Print(ZintSymbol * symbol, int rotate_angle);
int ZBarcode_Encode_and_Print(ZintSymbol * symbol, const uchar * input, int length, int rotate_angle);
int ZBarcode_Encode_File_and_Print(ZintSymbol * symbol, char * filename, int rotate_angle);
int ZBarcode_Render(ZintSymbol * symbol, const float width, const float height);
int ZBarcode_Buffer(ZintSymbol * symbol, int rotate_angle);
int ZBarcode_Encode_and_Buffer(ZintSymbol * symbol, const uchar * input, int length, int rotate_angle);
int ZBarcode_Encode_File_and_Buffer(ZintSymbol * symbol, char * filename, int rotate_angle);
int ZBarcode_ValidID(int symbol_id);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZINT_H */