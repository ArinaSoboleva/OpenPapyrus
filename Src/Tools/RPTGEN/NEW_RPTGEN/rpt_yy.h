// RPT_YY.H
// Copyright (c) A.Sobolev 2002
//
// Support for bison and flex parsing

typedef struct {
#ifdef FLEX_SCANNER
	YY_BUFFER_STATE yyin_buf;
#else
	void * yyin_buf;
#endif
	char fname[_MAX_PATH];
	long yyin_line;
} YYIN_STR;

extern FILE * yyin;

void yyerror(char * str);
int  yylex(void);