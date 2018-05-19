/* A Bison parser, made by GNU Bison 1.875a.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     ARITH_NUM = 258,
     ARITH_LPAREN = 259,
     ARITH_RPAREN = 260,
     ARITH_OR = 261,
     ARITH_AND = 262,
     ARITH_BOR = 263,
     ARITH_BXOR = 264,
     ARITH_BAND = 265,
     ARITH_NE = 266,
     ARITH_EQ = 267,
     ARITH_LE = 268,
     ARITH_GE = 269,
     ARITH_GT = 270,
     ARITH_LT = 271,
     ARITH_RSHIFT = 272,
     ARITH_LSHIFT = 273,
     ARITH_SUB = 274,
     ARITH_ADD = 275,
     ARITH_REM = 276,
     ARITH_DIV = 277,
     ARITH_MUL = 278,
     ARITH_BNOT = 279,
     ARITH_NOT = 280,
     ARITH_UNARYPLUS = 281,
     ARITH_UNARYMINUS = 282
   };
#endif
#define ARITH_NUM 258
#define ARITH_LPAREN 259
#define ARITH_RPAREN 260
#define ARITH_OR 261
#define ARITH_AND 262
#define ARITH_BOR 263
#define ARITH_BXOR 264
#define ARITH_BAND 265
#define ARITH_NE 266
#define ARITH_EQ 267
#define ARITH_LE 268
#define ARITH_GE 269
#define ARITH_GT 270
#define ARITH_LT 271
#define ARITH_RSHIFT 272
#define ARITH_LSHIFT 273
#define ARITH_SUB 274
#define ARITH_ADD 275
#define ARITH_REM 276
#define ARITH_DIV 277
#define ARITH_MUL 278
#define ARITH_BNOT 279
#define ARITH_NOT 280
#define ARITH_UNARYPLUS 281
#define ARITH_UNARYMINUS 282




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
typedef int YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;



