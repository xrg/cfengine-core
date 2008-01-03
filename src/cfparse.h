/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     LVALUE = 258,
     ID = 259,
     VAROBJ = 260,
     LBRACK = 261,
     RBRACK = 262,
     CONTROL = 263,
     GROUPS = 264,
     ARROW = 265,
     EQUALS = 266,
     EDITFILES = 267,
     QSTRING = 268,
     RVALUE = 269,
     BCLASS = 270,
     LBRACE = 271,
     RBRACE = 272,
     PARSECLASS = 273,
     LARROW = 274,
     OPTION = 275,
     FUNCTION = 276,
     ACL = 277,
     ADMIT = 278,
     DENY = 279,
     FILTERS = 280,
     STRATEGIES = 281,
     ACTIONTYPE = 282,
     ACCESSOBJ = 283
   };
#endif
/* Tokens.  */
#define LVALUE 258
#define ID 259
#define VAROBJ 260
#define LBRACK 261
#define RBRACK 262
#define CONTROL 263
#define GROUPS 264
#define ARROW 265
#define EQUALS 266
#define EDITFILES 267
#define QSTRING 268
#define RVALUE 269
#define BCLASS 270
#define LBRACE 271
#define RBRACE 272
#define PARSECLASS 273
#define LARROW 274
#define OPTION 275
#define FUNCTION 276
#define ACL 277
#define ADMIT 278
#define DENY 279
#define FILTERS 280
#define STRATEGIES 281
#define ACTIONTYPE 282
#define ACCESSOBJ 283




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef int YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;

