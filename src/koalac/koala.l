/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2018-2022 James <zhuguangxiang@gmail.com>
 */

/* prologue */

%{
#include "lexer.inc"
%}

%option bison-locations
%option bison-bridge
%option reentrant
%option nounput
%option noinput
%option never-interactive

%x STR TXT COMMENT

/* rules */

%%

"package"   { YY_RETURN(PACKAGE);  }
"import"    { YY_RETURN(IMPORT);   }
"type"      { YY_RETURN(TYPE);     }
"let"       { YY_RETURN(LET);      }
"var"       { YY_RETURN(VAR);      }
"func"      { YY_RETURN(FUNC);     }
"class"     { YY_RETURN(CLASS);    }
"interface" { YY_RETURN(INTERFACE);}
"enum"      { YY_RETURN(ENUM);     }
"struct"    { YY_RETURN(STRUCT);   }
"if"        { YY_RETURN(IF);       }
"else"      { YY_RETURN(ELSE);     }
"while"     { YY_RETURN(WHILE);    }
"for"       { YY_RETURN(FOR);      }
"switch"    { YY_RETURN(SWITCH);   }
"case"      { YY_RETURN(CASE);     }
"break"     { YY_RETURN(BREAK);    }
"continue"  { YY_RETURN(CONTINUE); }
"return"    { YY_RETURN(RETURN);   }
"in"        { YY_RETURN(IN);       }
"as"        { YY_RETURN(AS);       }
"is"        { YY_RETURN(IS);       }
"public"    { YY_RETURN(PUBLIC);   }
"extern \"C\""  { YY_RETURN(EXTERNC); }

"self"      { YY_RETURN(SELF);  }
"super"     { YY_RETURN(SUPER); }
"true"      { YY_RETURN(TRUE);  }
"false"     { YY_RETURN(FALSE); }
"null"      { YY_RETURN(NULL_TK); }

("UInt8"|"uint8"|"u8")    {  printf("u8\n"); YY_RETURN(UINT8); }
"UInt16"    { YY_RETURN(UINT16);  }
"UInt32"    { YY_RETURN(UINT32);  }
"UInt64"    { YY_RETURN(UINT64);  }
"Int8"      { YY_RETURN(INT8);    }
"Int16"     { YY_RETURN(INT16);   }
"Int32"     { YY_RETURN(INT32);   }
"Int64"     { YY_RETURN(INT64);   }
"Int"       { YY_RETURN_INT();    }
"UInt"      { YY_RETURN_UINT();   }
"Float32"   { YY_RETURN(FLOAT32); }
"Float64"   { YY_RETURN(FLOAT64); }
"Bool"      { YY_RETURN(BOOL);    }
"Char"      { YY_RETURN(CHAR);    }
"String"    { YY_RETURN(STRING);  }
"Any"       { YY_RETURN(ANY);     }
"Pointer"   { YY_RETURN(POINTER); }
"uint16"    { YY_RETURN(UINT16);  }
"uint32"    { YY_RETURN(UINT32);  }
"uint64"    { YY_RETURN(UINT64);  }
"int8"      { YY_RETURN(INT8);    }
"int16"     { YY_RETURN(INT16);   }
"int32"     { YY_RETURN(INT32);   }
"int64"     { YY_RETURN(INT64);   }
"int"       { YY_RETURN_INT();    }
"uint"      { YY_RETURN_UINT();   }
"float32"   { YY_RETURN(FLOAT32); }
"float64"   { YY_RETURN(FLOAT64); }
"bool"      { YY_RETURN(BOOL);    }
"char"      { YY_RETURN(CHAR);    }
"string"    { YY_RETURN(STRING);  }

"sizeof"    { YY_RETURN(SIZEOF); }
"inout"     { YY_RETURN(INOUT); }

"&&"        { YY_RETURN(AND); }
"||"        { YY_RETURN(OR);  }
"!"         { YY_RETURN(NOT); }

"=="        { YY_RETURN(EQ);  }
"!="        { YY_RETURN(NE);  }
">="        { YY_RETURN(GE);  }
"<="        { YY_RETURN(LE);  }

"+="        { YY_RETURN(PLUS_ASSIGN);  }
"-="        { YY_RETURN(MINUS_ASSIGN); }
"*="        { YY_RETURN(MULT_ASSIGN);  }
"/="        { YY_RETURN(DIV_ASSIGN);   }
"%="        { YY_RETURN(MOD_ASSIGN);   }
"&="        { YY_RETURN(AND_ASSIGN);   }
"|="        { YY_RETURN(OR_ASSIGN);    }
"^="        { YY_RETURN(XOR_ASSIGN);   }
"<<="       { YY_RETURN(SHL_ASSIGN);   }
">>="       { YY_RETURN(SHR_ASSIGN);   }

"..."       { YY_RETURN(DOTDOTDOT);  }
"..<"       { YY_RETURN(DOTDOTLESS); }

"@"         { YY_RETURN('@'); }
"+"         { YY_RETURN('+'); }
"-"         { YY_RETURN('-'); }
"*"         { YY_RETURN('*'); }
"/"         { YY_RETURN('/'); }
"%"         { YY_RETURN('%'); }
"&"         { YY_RETURN('&'); }
"|"         { YY_RETURN('|'); }
"~"         { YY_RETURN('~'); }
"^"         { YY_RETURN('^'); }

":"         { YY_RETURN(':'); }
","         { YY_RETURN(','); }
";"         { YY_RETURN(';'); }
"."         { YY_RETURN('.'); }
"="         { YY_RETURN('='); }
"_"         { YY_RETURN('_'); }

"("         { YY_RETURN('('); }
")"         { YY_RETURN(')'); }
"["         { YY_RETURN('['); }
"]"         { YY_RETURN(']'); }
"{"         { YY_RETURN('{'); }
"}"         { YY_RETURN('}'); }

">"         { YY_RETURN('>'); }
"<" {
    if (yyps->in_angle) {
        yyps->in_angle = 0;
        printf("L_ANGLE_ARGS\n");
        YY_RETURN(L_ANGLE_ARGS);
    } else {
        YY_RETURN('<');
    }
}

"<<"        { YY_RETURN(L_SHIFT); }
">"/">"     { YY_RETURN(R_ANGLE_SHIFT); }

\"          { BEGIN(STR); YY_STR_BEGIN;     }
<STR>\"     { BEGIN(INITIAL); YY_STR_END;   }
<STR>\\\"   { YY_STRING_WRITE('\"');        }
<STR>\\\\   { YY_STRING_WRITE('\\');        }
<STR>\n     { /* omit newline */            }
<STR>\r     { /* omit return */             }
<STR>.      { YY_STRING_WRITE(yytext[0]);   }

\"\"\"      { BEGIN(TXT); YY_TEXT_BEGIN;    }
<TXT>\"\"\" { BEGIN(INITIAL); YY_TEXT_END;  }
<TXT>\n     {
                /* text with newline */
                YY_TEXT_WRITE('\n');
            }
<TXT>\r     {
                /* text with return */
                YY_TEXT_WRITE('\r');
            }
<TXT>.      { YY_TEXT_WRITE(yytext[0]);     }

[A-Za-z_][A-Za-z0-9_]*  {
    YY_IDENT_VALUE(yytext, yyleng);
    check_type_param(yyps, yytext);
    YY_RETURN(ID);
}

0[xX][0-9a-fA-F_]+  { YY_INT_VALUE(yytext + 2, 16); YY_RETURN(INT_LITERAL); }

0[dD][0-9_]+    { YY_INT_VALUE(yytext + 2, 10); YY_RETURN(INT_LITERAL); }
0[oO][0-7_]+    { YY_INT_VALUE(yytext + 2, 8); YY_RETURN(INT_LITERAL);  }
0[bB][01_]+     { YY_INT_VALUE(yytext + 2, 2); YY_RETURN(INT_LITERAL);  }
[0-9][0-9_]*    { YY_INT_VALUE(yytext, 10); YY_RETURN(INT_LITERAL);     }

([0-9]+\.[0-9]+|[0-9]+)([eE][-+]?[0-9]+)? {
    YY_FLOAT_VALUE(yytext); YY_RETURN(FLOAT_LITERAL);
}

[ \t\r]  { }

"\n" {
    // printf("new_line:%d\n", yyps->row);
    #if 0
    if (yyps->doc_line > 0 && yyps->doc_line != yyps->row) {
        /* not consecutive document */
        printf("doc\n");
        printf("%s", BUF_STR(yyps->doc_buf));
        printf("doc is taken\n");
        yyps->doc_line = 0;
        RESET_BUF(yyps->doc_buf);
    }
    #endif
    YY_NEWLINE;
}

"///"[^\n]* {
    // yyps->doc_line = yyps->row;
    // do_doc(yyps, yytext + 3, yyleng - 3);
    printf("doc comment\n");
}

"//"[^\n]*  { /* single line comment */ }

"/*"            { BEGIN(COMMENT); yyps->in_str = 1; }
<COMMENT>"*/"   { BEGIN(INITIAL); yyps->in_str = 0; }
<COMMENT>\n     { /* omit new line */ }
<COMMENT>.      { /* omit other characters */ }

. {  }

%%

/* epilogue */

/*
When the scanner receives an end-of-file indication from YY_INPUT, it then
checks the yywrap() function.

If yywrap() returns false (zero), then it is assumed that the function has gone
ahead and set up yyin to point to another input file, and scanning continues.

If it returns true (non-zero), then the scanner terminates.
*/
int yywrap(yyscan_t yyscanner)
{
    UNUSED(yyscanner);
    return -1;
}
