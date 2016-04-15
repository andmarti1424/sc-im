/* A Bison parser, made by GNU Bison 3.0.2.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2013 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

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

#ifndef YY_YY_Y_TAB_H_INCLUDED
# define YY_YY_Y_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    STRING = 258,
    COL = 259,
    NUMBER = 260,
    FNUMBER = 261,
    RANGE = 262,
    VAR = 263,
    WORD = 264,
    MAPWORD = 265,
    PLUGIN = 266,
    S_SHOW = 267,
    S_HIDE = 268,
    S_SHOWROW = 269,
    S_HIDEROW = 270,
    S_SHOWCOL = 271,
    S_HIDECOL = 272,
    S_MARK = 273,
    S_AUTOJUS = 274,
    S_PAD = 275,
    S_DATEFMT = 276,
    S_FORMAT = 277,
    S_FMT = 278,
    S_LET = 279,
    S_LABEL = 280,
    S_LEFTSTRING = 281,
    S_RIGHTSTRING = 282,
    S_LEFTJUSTIFY = 283,
    S_RIGHTJUSTIFY = 284,
    S_CENTER = 285,
    S_SORT = 286,
    S_FILTERON = 287,
    S_GOTO = 288,
    S_SET = 289,
    S_LOCK = 290,
    S_UNLOCK = 291,
    S_DEFINE = 292,
    S_UNDEFINE = 293,
    S_EVAL = 294,
    S_SEVAL = 295,
    S_GETNUM = 296,
    S_GETSTRING = 297,
    S_GETEXP = 298,
    S_GETFMT = 299,
    S_GETFORMAT = 300,
    S_RECALC = 301,
    S_QUIT = 302,
    S_IMAP = 303,
    S_NMAP = 304,
    S_INOREMAP = 305,
    S_NNOREMAP = 306,
    S_NUNMAP = 307,
    S_IUNMAP = 308,
    S_COLOR = 309,
    S_CELLCOLOR = 310,
    S_REDEFINE_COLOR = 311,
    K_ERROR = 312,
    K_INVALID = 313,
    K_FIXED = 314,
    K_SUM = 315,
    K_PROD = 316,
    K_AVG = 317,
    K_STDDEV = 318,
    K_COUNT = 319,
    K_ROWS = 320,
    K_COLS = 321,
    K_ABS = 322,
    K_ACOS = 323,
    K_ASIN = 324,
    K_ATAN = 325,
    K_ATAN2 = 326,
    K_CEIL = 327,
    K_COS = 328,
    K_EXP = 329,
    K_FABS = 330,
    K_FLOOR = 331,
    K_HYPOT = 332,
    K_LN = 333,
    K_LOG = 334,
    K_PI = 335,
    K_POW = 336,
    K_SIN = 337,
    K_SQRT = 338,
    K_TAN = 339,
    K_DTR = 340,
    K_RTD = 341,
    K_MAX = 342,
    K_MIN = 343,
    K_RND = 344,
    K_ROUND = 345,
    K_IF = 346,
    K_PV = 347,
    K_FV = 348,
    K_PMT = 349,
    K_HOUR = 350,
    K_MINUTE = 351,
    K_SECOND = 352,
    K_MONTH = 353,
    K_DAY = 354,
    K_YEAR = 355,
    K_NOW = 356,
    K_DATE = 357,
    K_DTS = 358,
    K_TTS = 359,
    K_FMT = 360,
    K_SUBSTR = 361,
    K_UPPER = 362,
    K_LOWER = 363,
    K_CAPITAL = 364,
    K_STON = 365,
    K_SLEN = 366,
    K_EQS = 367,
    K_EXT = 368,
    K_NVAL = 369,
    K_SVAL = 370,
    K_LOOKUP = 371,
    K_HLOOKUP = 372,
    K_VLOOKUP = 373,
    K_INDEX = 374,
    K_STINDEX = 375,
    K_AUTO = 376,
    K_AUTOCALC = 377,
    K_AUTOINSERT = 378,
    K_AUTOWRAP = 379,
    K_CSLOP = 380,
    K_BYROWS = 381,
    K_BYCOLS = 382,
    K_OPTIMIZE = 383,
    K_ITERATIONS = 384,
    K_NUMERIC = 385,
    K_PRESCALE = 386,
    K_EXTFUN = 387,
    K_CELLCUR = 388,
    K_TOPROW = 389,
    K_COLOR = 390,
    K_COLORNEG = 391,
    K_COLORERR = 392,
    K_BRAILLE = 393,
    K_TBLSTYLE = 394,
    K_TBL = 395,
    K_LATEX = 396,
    K_SLATEX = 397,
    K_TEX = 398,
    K_FRAME = 399,
    K_RNDTOEVEN = 400,
    K_FILENAME = 401,
    K_MYROW = 402,
    K_MYCOL = 403,
    K_LASTROW = 404,
    K_LASTCOL = 405,
    K_COLTOA = 406,
    K_CRACTION = 407,
    K_CRROW = 408,
    K_CRCOL = 409,
    K_ROWLIMIT = 410,
    K_COLLIMIT = 411,
    K_PAGESIZE = 412,
    K_NUMITER = 413,
    K_ERR = 414,
    K_LOCALE = 415,
    K_SET8BIT = 416,
    K_ASCII = 417,
    K_CHR = 418
  };
#endif
/* Tokens.  */
#define STRING 258
#define COL 259
#define NUMBER 260
#define FNUMBER 261
#define RANGE 262
#define VAR 263
#define WORD 264
#define MAPWORD 265
#define PLUGIN 266
#define S_SHOW 267
#define S_HIDE 268
#define S_SHOWROW 269
#define S_HIDEROW 270
#define S_SHOWCOL 271
#define S_HIDECOL 272
#define S_MARK 273
#define S_AUTOJUS 274
#define S_PAD 275
#define S_DATEFMT 276
#define S_FORMAT 277
#define S_FMT 278
#define S_LET 279
#define S_LABEL 280
#define S_LEFTSTRING 281
#define S_RIGHTSTRING 282
#define S_LEFTJUSTIFY 283
#define S_RIGHTJUSTIFY 284
#define S_CENTER 285
#define S_SORT 286
#define S_FILTERON 287
#define S_GOTO 288
#define S_SET 289
#define S_LOCK 290
#define S_UNLOCK 291
#define S_DEFINE 292
#define S_UNDEFINE 293
#define S_EVAL 294
#define S_SEVAL 295
#define S_GETNUM 296
#define S_GETSTRING 297
#define S_GETEXP 298
#define S_GETFMT 299
#define S_GETFORMAT 300
#define S_RECALC 301
#define S_QUIT 302
#define S_IMAP 303
#define S_NMAP 304
#define S_INOREMAP 305
#define S_NNOREMAP 306
#define S_NUNMAP 307
#define S_IUNMAP 308
#define S_COLOR 309
#define S_CELLCOLOR 310
#define S_REDEFINE_COLOR 311
#define K_ERROR 312
#define K_INVALID 313
#define K_FIXED 314
#define K_SUM 315
#define K_PROD 316
#define K_AVG 317
#define K_STDDEV 318
#define K_COUNT 319
#define K_ROWS 320
#define K_COLS 321
#define K_ABS 322
#define K_ACOS 323
#define K_ASIN 324
#define K_ATAN 325
#define K_ATAN2 326
#define K_CEIL 327
#define K_COS 328
#define K_EXP 329
#define K_FABS 330
#define K_FLOOR 331
#define K_HYPOT 332
#define K_LN 333
#define K_LOG 334
#define K_PI 335
#define K_POW 336
#define K_SIN 337
#define K_SQRT 338
#define K_TAN 339
#define K_DTR 340
#define K_RTD 341
#define K_MAX 342
#define K_MIN 343
#define K_RND 344
#define K_ROUND 345
#define K_IF 346
#define K_PV 347
#define K_FV 348
#define K_PMT 349
#define K_HOUR 350
#define K_MINUTE 351
#define K_SECOND 352
#define K_MONTH 353
#define K_DAY 354
#define K_YEAR 355
#define K_NOW 356
#define K_DATE 357
#define K_DTS 358
#define K_TTS 359
#define K_FMT 360
#define K_SUBSTR 361
#define K_UPPER 362
#define K_LOWER 363
#define K_CAPITAL 364
#define K_STON 365
#define K_SLEN 366
#define K_EQS 367
#define K_EXT 368
#define K_NVAL 369
#define K_SVAL 370
#define K_LOOKUP 371
#define K_HLOOKUP 372
#define K_VLOOKUP 373
#define K_INDEX 374
#define K_STINDEX 375
#define K_AUTO 376
#define K_AUTOCALC 377
#define K_AUTOINSERT 378
#define K_AUTOWRAP 379
#define K_CSLOP 380
#define K_BYROWS 381
#define K_BYCOLS 382
#define K_OPTIMIZE 383
#define K_ITERATIONS 384
#define K_NUMERIC 385
#define K_PRESCALE 386
#define K_EXTFUN 387
#define K_CELLCUR 388
#define K_TOPROW 389
#define K_COLOR 390
#define K_COLORNEG 391
#define K_COLORERR 392
#define K_BRAILLE 393
#define K_TBLSTYLE 394
#define K_TBL 395
#define K_LATEX 396
#define K_SLATEX 397
#define K_TEX 398
#define K_FRAME 399
#define K_RNDTOEVEN 400
#define K_FILENAME 401
#define K_MYROW 402
#define K_MYCOL 403
#define K_LASTROW 404
#define K_LASTCOL 405
#define K_COLTOA 406
#define K_CRACTION 407
#define K_CRROW 408
#define K_CRCOL 409
#define K_ROWLIMIT 410
#define K_COLLIMIT 411
#define K_PAGESIZE 412
#define K_NUMITER 413
#define K_ERR 414
#define K_LOCALE 415
#define K_SET8BIT 416
#define K_ASCII 417
#define K_CHR 418

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE YYSTYPE;
union YYSTYPE
{
#line 35 "gram.y" /* yacc.c:1909  */

    int ival;
    double fval;
    struct ent_ptr ent;
    struct enode * enode;
    char * sval;
    struct range_s rval; // no debiera usarse en futuro

#line 389 "y.tab.h" /* yacc.c:1909  */
};
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_Y_TAB_H_INCLUDED  */
