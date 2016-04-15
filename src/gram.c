/* A Bison parser, made by GNU Bison 3.0.2.  */

/* Bison implementation for Yacc-like parsers in C

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.0.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* Copy the first part of user declarations.  */
#line 1 "gram.y" /* yacc.c:339  */

#include <curses.h>
#include <stdlib.h>
#include "sc.h"
#include "cmds.h"
#include "color.h"
#include "interp.h"
#include "macros.h"
#include "sort.h"
#include "filter.h"
#include "maps.h"
#include "marks.h"
#include "xmalloc.h" // for scxfree
#include "hide_show.h"
#include "cmds_normal.h"
#include "conf.h"
#include "pipe.h"
#include "main.h"

void yyerror(char *err);               // error routine for yacc (gram.y)
int yylex();


#ifdef USELOCALE
#include <locale.h>
#endif

#ifndef MSDOS
#include <unistd.h>
#endif

#define ENULL (struct enode *) 0

#line 100 "y.tab.c" /* yacc.c:339  */

# ifndef YY_NULLPTR
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULLPTR nullptr
#  else
#   define YY_NULLPTR 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* In a future release of Bison, this section will be replaced
   by #include "y.tab.h".  */
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
#line 35 "gram.y" /* yacc.c:355  */

    int ival;
    double fval;
    struct ent_ptr ent;
    struct enode * enode;
    char * sval;
    struct range_s rval; // no debiera usarse en futuro

#line 475 "y.tab.c" /* yacc.c:355  */
};
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_Y_TAB_H_INCLUDED  */

/* Copy the second part of user declarations.  */

#line 490 "y.tab.c" /* yacc.c:358  */

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef YY_ATTRIBUTE
# if (defined __GNUC__                                               \
      && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)))  \
     || defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

#if !defined _Noreturn \
     && (!defined __STDC_VERSION__ || __STDC_VERSION__ < 201112)
# if defined _MSC_VER && 1200 <= _MSC_VER
#  define _Noreturn __declspec (noreturn)
# else
#  define _Noreturn YY_ATTRIBUTE ((__noreturn__))
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif


#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYSIZE_T yynewbytes;                                            \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                          \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  120
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   2554

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  187
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  10
/* YYNRULES -- Number of rules.  */
#define YYNRULES  203
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  628

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   418

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   172,     2,   175,   186,   178,   168,     2,
     181,   182,   176,   173,   183,   174,   184,   177,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   166,   164,
     169,   170,   171,   165,   180,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,   179,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,   167,     2,   185,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   147,   148,   149,   150,   151,   152,   153,   154,
     155,   156,   157,   158,   159,   160,   161,   162,   163
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   321,   321,   322,   335,   336,   337,   338,   339,   340,
     341,   342,   346,   348,   351,   352,   353,   354,   357,   359,
     361,   363,   365,   367,   369,   381,   394,   396,   402,   405,
     422,   423,   425,   426,   427,   428,   429,   430,   431,   434,
     435,   436,   442,   448,   454,   461,   466,   471,   478,   486,
     490,   491,   492,   506,   507,   509,   511,   514,   520,   525,
     529,   531,   533,   535,   576,   577,   578,   579,   580,   582,
     585,   588,   591,   594,   597,   600,   603,   606,   609,   612,
     615,   618,   620,   623,   626,   628,   630,   632,   633,   634,
     635,   636,   637,   638,   639,   640,   641,   642,   643,   644,
     645,   646,   647,   648,   649,   650,   651,   652,   653,   655,
     656,   657,   659,   660,   661,   662,   663,   664,   665,   666,
     668,   672,   674,   675,   676,   677,   678,   679,   680,   681,
     682,   683,   685,   687,   690,   692,   694,   697,   700,   703,
     706,   708,   710,   713,   714,   715,   716,   718,   719,   720,
     721,   722,   723,   724,   725,   726,   727,   728,   729,   730,
     731,   732,   733,   734,   735,   736,   737,   738,   752,   753,
     754,   755,   756,   757,   758,   759,   760,   761,   762,   763,
     764,   765,   766,   767,   768,   769,   770,   773,   774,   777,
     778,   781,   784,   787,   790,   794,   799,   800,   803,   804,
     805,   806,   808,   809
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "STRING", "COL", "NUMBER", "FNUMBER",
  "RANGE", "VAR", "WORD", "MAPWORD", "PLUGIN", "S_SHOW", "S_HIDE",
  "S_SHOWROW", "S_HIDEROW", "S_SHOWCOL", "S_HIDECOL", "S_MARK",
  "S_AUTOJUS", "S_PAD", "S_DATEFMT", "S_FORMAT", "S_FMT", "S_LET",
  "S_LABEL", "S_LEFTSTRING", "S_RIGHTSTRING", "S_LEFTJUSTIFY",
  "S_RIGHTJUSTIFY", "S_CENTER", "S_SORT", "S_FILTERON", "S_GOTO", "S_SET",
  "S_LOCK", "S_UNLOCK", "S_DEFINE", "S_UNDEFINE", "S_EVAL", "S_SEVAL",
  "S_GETNUM", "S_GETSTRING", "S_GETEXP", "S_GETFMT", "S_GETFORMAT",
  "S_RECALC", "S_QUIT", "S_IMAP", "S_NMAP", "S_INOREMAP", "S_NNOREMAP",
  "S_NUNMAP", "S_IUNMAP", "S_COLOR", "S_CELLCOLOR", "S_REDEFINE_COLOR",
  "K_ERROR", "K_INVALID", "K_FIXED", "K_SUM", "K_PROD", "K_AVG",
  "K_STDDEV", "K_COUNT", "K_ROWS", "K_COLS", "K_ABS", "K_ACOS", "K_ASIN",
  "K_ATAN", "K_ATAN2", "K_CEIL", "K_COS", "K_EXP", "K_FABS", "K_FLOOR",
  "K_HYPOT", "K_LN", "K_LOG", "K_PI", "K_POW", "K_SIN", "K_SQRT", "K_TAN",
  "K_DTR", "K_RTD", "K_MAX", "K_MIN", "K_RND", "K_ROUND", "K_IF", "K_PV",
  "K_FV", "K_PMT", "K_HOUR", "K_MINUTE", "K_SECOND", "K_MONTH", "K_DAY",
  "K_YEAR", "K_NOW", "K_DATE", "K_DTS", "K_TTS", "K_FMT", "K_SUBSTR",
  "K_UPPER", "K_LOWER", "K_CAPITAL", "K_STON", "K_SLEN", "K_EQS", "K_EXT",
  "K_NVAL", "K_SVAL", "K_LOOKUP", "K_HLOOKUP", "K_VLOOKUP", "K_INDEX",
  "K_STINDEX", "K_AUTO", "K_AUTOCALC", "K_AUTOINSERT", "K_AUTOWRAP",
  "K_CSLOP", "K_BYROWS", "K_BYCOLS", "K_OPTIMIZE", "K_ITERATIONS",
  "K_NUMERIC", "K_PRESCALE", "K_EXTFUN", "K_CELLCUR", "K_TOPROW",
  "K_COLOR", "K_COLORNEG", "K_COLORERR", "K_BRAILLE", "K_TBLSTYLE",
  "K_TBL", "K_LATEX", "K_SLATEX", "K_TEX", "K_FRAME", "K_RNDTOEVEN",
  "K_FILENAME", "K_MYROW", "K_MYCOL", "K_LASTROW", "K_LASTCOL", "K_COLTOA",
  "K_CRACTION", "K_CRROW", "K_CRCOL", "K_ROWLIMIT", "K_COLLIMIT",
  "K_PAGESIZE", "K_NUMITER", "K_ERR", "K_LOCALE", "K_SET8BIT", "K_ASCII",
  "K_CHR", "';'", "'?'", "':'", "'|'", "'&'", "'<'", "'='", "'>'", "'!'",
  "'+'", "'-'", "'#'", "'*'", "'/'", "'%'", "'^'", "'@'", "'('", "')'",
  "','", "'.'", "'~'", "'$'", "$accept", "command", "term", "e",
  "expr_list", "range", "var", "var_or_range", "num", "strarg", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
     365,   366,   367,   368,   369,   370,   371,   372,   373,   374,
     375,   376,   377,   378,   379,   380,   381,   382,   383,   384,
     385,   386,   387,   388,   389,   390,   391,   392,   393,   394,
     395,   396,   397,   398,   399,   400,   401,   402,   403,   404,
     405,   406,   407,   408,   409,   410,   411,   412,   413,   414,
     415,   416,   417,   418,    59,    63,    58,   124,    38,    60,
      61,    62,    33,    43,    45,    35,    42,    47,    37,    94,
      64,    40,    41,    44,    46,   126,    36
};
# endif

#define YYPACT_NINF -171

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-171)))

#define YYTABLE_NINF -198

#define yytable_value_is_error(Yytable_value) \
  (!!((Yytable_value) == (-198)))

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     824,  -171,    26,    28,    -1,    14,    23,    32,    37,    43,
      50,    10,    59,    21,    21,    21,    21,    21,    21,    21,
      21,    21,    21,    45,    21,    21,    12,    21,   422,    21,
      21,    21,    21,    61,  -171,  -171,    64,    66,    69,    72,
      88,    95,    96,    21,    97,    68,  -171,  -171,  -171,  -171,
    -100,   -65,   -64,   -63,    21,   -62,    30,  -171,     0,  -171,
    -171,   101,  -171,   -59,   109,   110,   111,   -57,   -54,   -52,
     -50,  -171,  -171,  -171,   126,   -59,  -171,  -171,  -171,  -171,
    -171,    65,    65,   127,   131,    21,  -171,  -171,  -171,  -171,
    -171,    21,  -171,  -171,   -48,  -171,  -171,   422,   422,   422,
     567,   439,   422,  -171,  2340,  -171,  -171,  -171,  -171,  -171,
    -171,   138,   139,   140,   141,  -171,  -171,  -171,   142,   144,
    -171,   145,   147,   143,   149,    21,   151,     6,  -171,  -171,
     152,    35,    31,  -171,   153,  -171,   422,   422,   422,   422,
    -171,  -171,  -171,  -171,  -171,  -171,  -171,   -59,   154,  -171,
    -171,  -171,   422,   -35,   -25,   -21,   -20,   -16,   -14,   -13,
     -12,   -11,   -10,    -8,    -2,     1,     2,    13,    16,    18,
      41,    44,    46,  -171,    51,    53,    62,   122,   123,   124,
     125,   130,   133,   135,   136,   137,   156,   157,   158,   160,
     170,   172,   173,   174,  -171,   177,   178,   179,   180,   181,
     182,   183,   184,   185,   186,   189,   190,   191,   192,   193,
     201,   202,   208,   209,   210,  -171,  -171,  -171,  -171,   211,
    -171,  -171,   212,   213,   214,   672,  1281,  -171,   422,   422,
     422,   422,     4,   422,    56,    11,   422,   422,   422,   422,
     422,   422,   422,  -171,  -171,  -171,  -171,  -171,   175,  -171,
    -171,  -171,  -171,  -171,  -171,   187,  -171,  -171,   219,  -171,
     235,  2340,  2340,  2340,  2340,     3,  -171,    21,    21,    21,
      21,    21,    21,    21,   422,   422,   422,   422,   422,   422,
     422,   422,   422,   422,   422,   422,   422,   422,   422,   422,
     422,   422,   422,   321,   321,   422,   422,   422,   422,   422,
     422,   422,   422,   422,   422,   422,   422,   422,   422,   422,
     422,   422,   422,   422,   422,   422,   422,   422,   422,   422,
     422,   321,   321,   321,   321,   321,   422,   422,   422,   422,
     422,   344,  -171,  2340,  2324,   363,  2364,   422,   422,  2375,
    2375,   422,  2375,   422,  -155,  -155,  -155,   128,   128,   128,
    -171,   239,  -171,  -171,  -171,   240,   129,   221,   222,   224,
     225,   228,   229,   231,   232,   237,   241,   247,  1297,  1313,
    1329,  1345,   737,  1361,  1377,  1393,  1409,  1425,   754,  1441,
    1457,   771,  1473,  1489,  1505,  1521,  1537,   788,   234,  -140,
     249,   805,   238,   251,  1553,   822,   839,   856,   873,   890,
    1569,  1585,  1601,  1617,  1633,  1649,   524,   907,   924,   941,
     958,  1665,  1681,  1697,  1713,  1729,   975,   992,  1009,  1026,
    1043,   252,   -59,  1060,   253,  1077,   254,  1094,   256,  1111,
     257,  1745,  1761,  1777,  1793,  1809,   422,   422,  2375,  2375,
    2375,  2375,  -171,  -171,   422,  -171,   422,  -171,   422,  -171,
     422,  -171,   422,  -171,  -171,  -171,  -171,  -171,  -171,  -171,
     422,  -171,  -171,  -171,  -171,  -171,   422,  -171,  -171,   422,
    -171,  -171,  -171,  -171,  -171,   422,   422,  -171,   422,   422,
    -171,  -171,   422,   422,   422,   422,   422,  -171,  -171,  -171,
    -171,  -171,  -171,  -171,   422,   422,   422,   422,   422,  -171,
    -171,  -171,  -171,  -171,   422,   422,   422,   422,    21,   422,
      21,   422,    21,   422,    21,   422,    21,   422,  -171,  -171,
    -171,  -171,  -171,  -171,  2353,  1825,  1841,  1857,  1873,  1889,
    1905,  1921,  1937,  2340,  -139,  1953,  -137,  1969,  1985,  1128,
    1145,  1162,  1179,  2001,  1196,  1213,  2017,  1230,  2033,  2049,
    2065,  2081,   264,  2097,   266,  1247,   267,  1264,   270,   629,
     271,   717,  -171,  -171,  -171,  -171,  -171,  -171,  -171,  -171,
    -171,   422,  -171,  -171,  -171,  -171,   422,   422,   422,   422,
    -171,   422,   422,  -171,   422,  -171,  -171,  -171,  -171,  -171,
    -171,   422,   422,   422,   422,  -171,  -171,   422,  -171,  -171,
     422,  2340,  2113,  2129,  2145,  2161,  2177,  2193,  2209,  2225,
    2241,  2257,  2273,  2289,  2305,  -171,  -171,  -171,  -171,  -171,
    -171,  -171,  -171,  -171,  -171,  -171,  -171,  -171
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,    65,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    58,    57,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    16,    17,    14,    15,
      21,    20,    19,    18,     0,    31,     0,    13,     0,   190,
     195,     0,   196,   197,     0,     0,     0,     0,     0,     0,
       0,     7,     8,     9,     0,     0,    29,    35,   198,   199,
      38,     0,     0,     0,     0,    33,    34,    39,    40,   202,
     203,     0,    55,   153,   150,   151,   167,     0,     0,     0,
       0,     0,     0,   174,    56,    66,    59,    60,    61,    63,
      62,     0,     0,     0,     0,    45,    46,    47,     0,     0,
       1,     0,     0,     0,     0,    26,     0,    51,    52,   191,
       0,     0,     0,    12,     0,    11,     3,     0,     0,     0,
      28,   201,   200,    36,    37,    32,    53,    54,     0,   155,
     148,   149,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   152,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   118,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   157,   158,   159,   160,     0,
     165,   166,     0,     0,     0,     0,     0,   154,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    42,    41,    44,    43,    48,     0,    23,
      25,    22,    24,    27,    30,     0,   193,   192,     0,   189,
       0,     2,     4,     5,     6,     0,    67,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   147,   176,     0,   181,   180,     0,     0,   177,
     178,     0,   179,     0,   168,   169,   186,   170,   171,   172,
     173,     0,    50,   194,    10,     0,   196,     0,   196,     0,
     196,     0,   196,     0,   196,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   196,    66,
       0,     0,   196,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    66,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   182,   184,
     185,   183,    49,   120,     0,    69,     0,    71,     0,    73,
       0,    75,     0,    77,    85,    86,    87,    88,    89,    90,
       0,    92,    93,    94,    95,    96,     0,    98,    99,     0,
     101,   102,   103,   104,   105,     0,     0,    79,     0,     0,
      82,   106,     0,     0,     0,     0,     0,   112,   113,   114,
     115,   116,   117,   125,     0,     0,     0,     0,     0,   128,
     129,   130,   122,   123,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   156,   161,
     163,   162,   164,    68,   175,     0,     0,     0,     0,     0,
       0,     0,     0,   187,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    70,    72,    74,    76,    78,    91,    97,   100,
      81,     0,    80,    84,    83,   107,     0,     0,     0,     0,
     126,     0,     0,   127,     0,   124,   143,   144,   145,   135,
     134,     0,     0,     0,     0,   132,   131,     0,   141,   140,
       0,   188,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   108,   109,   110,   111,   119,
     121,   146,   137,   136,   139,   138,   133,   142
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -171,  -171,   -96,   -28,  -170,    75,    63,   108,   -24,  -171
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    45,   103,   533,   534,    62,   105,    64,    86,    91
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
     104,   149,   150,   151,    50,   129,   227,    93,    58,    94,
      95,   129,    60,    57,    58,    89,    58,    59,    60,    51,
      60,   239,   240,   241,   242,    58,   132,    52,    59,    60,
      46,    47,    48,    49,   127,    58,    53,    59,    60,    60,
     257,    54,  -197,   570,   571,   573,   571,    55,    77,    58,
      78,    79,    59,    60,    80,    56,   266,   141,   142,    93,
      58,    94,    95,    65,    60,   110,   121,   111,   120,   112,
      78,    79,   113,   226,    63,   114,    63,    63,    63,    63,
      63,    63,    63,    63,    75,    75,    63,    63,    63,    90,
      63,   115,    63,    63,    63,    63,    74,    76,   116,   117,
     119,   122,   123,   124,   126,   131,    63,   132,   261,   262,
     263,   264,   133,   136,   135,   134,   137,    63,   138,    63,
     139,    66,    67,    68,    69,    70,    71,    72,    73,   140,
     143,    85,    87,    88,   144,    92,   148,   106,   107,   108,
     109,   243,   244,   245,   246,   247,   267,   251,    63,   248,
     249,   118,   250,   252,   147,   254,   268,   256,   260,   265,
     269,   270,   125,    96,   128,   271,   146,   272,   273,   274,
     275,   276,   255,   277,   337,   338,    97,    98,    99,   278,
     351,   343,   279,   280,   100,   101,   130,   355,    63,   102,
      61,   352,   130,   145,   281,   259,    61,   282,    61,   283,
     333,   334,   335,   336,   339,   340,   342,    61,   344,   345,
     346,   347,   348,   349,   350,    96,    61,    61,    81,    82,
      83,   258,   284,    84,   353,   285,   341,   286,    97,    98,
      99,    61,   287,   253,   288,   266,   100,   101,    81,    82,
     354,   102,    61,   289,   442,   443,   368,   369,   370,   371,
     372,   373,   374,   375,   376,   377,   378,   379,   380,   381,
     382,   383,   384,   385,   386,   387,   391,   394,   395,   396,
     397,   398,   399,   400,   401,   402,   403,   404,   405,   406,
     407,   408,   409,   410,   411,   412,   413,   414,   415,   416,
     417,   418,   419,   420,   423,   425,   427,   429,   431,   432,
     433,   434,   435,   290,   291,   292,   293,   242,   536,   438,
     439,   294,   444,   440,   295,   441,   296,   297,   298,     0,
       0,     0,     0,     0,    93,    58,    94,    95,    59,    60,
      63,    63,    63,    63,    63,    63,    63,   299,   300,   301,
     523,   302,   356,   358,   360,   362,   364,    93,    58,    94,
      95,   303,    60,   304,   305,   306,   389,   389,   307,   308,
     309,   310,   311,   312,   313,   314,   315,   316,   388,   392,
     317,   318,   319,   320,   321,   357,   359,   361,   363,   365,
     366,   367,   322,   323,   422,   422,   422,   422,   422,   324,
     325,   326,   327,   328,   329,   330,   421,   424,   426,   428,
     430,   390,   393,   445,     0,   446,   447,     0,   448,   524,
     449,     0,   450,   451,     0,   452,   525,   476,   526,   453,
     527,   479,   528,   454,   529,    93,    58,    94,    95,   455,
      60,   477,   530,   480,     0,   509,   511,   513,   531,   515,
     517,   532,    93,    58,    94,    95,   589,    60,   535,   591,
     593,   537,   595,   598,   538,   539,   540,   541,   542,     0,
       0,     0,     0,     0,     0,     0,   543,   544,   545,   546,
     547,     0,     0,     0,     0,     0,   548,   549,   550,   551,
      96,   553,     0,   555,     0,   557,     0,   559,     0,   561,
       0,     0,     0,    97,    98,    99,     0,     0,     0,     0,
       0,   100,   101,    96,     0,     0,   102,    61,     0,     0,
       0,     0,     0,     0,     0,     0,    97,    98,    99,     0,
       0,     0,     0,     0,   100,   101,   436,     0,     0,   102,
      61,   231,   232,   233,   234,   235,   236,   237,   238,   239,
     240,   241,   242,   601,     0,     0,     0,     0,   602,   603,
     604,   605,     0,   606,   607,     0,   608,     0,     0,     0,
       0,     0,     0,   609,   610,   611,   612,     0,     0,   613,
       0,    75,   614,    75,     0,    75,     0,    75,     0,    75,
       0,    96,     0,   552,     0,   554,     0,   556,     0,   558,
       0,   560,     0,     0,    97,    98,    99,     0,    96,     0,
       0,     0,   100,   101,     0,     0,     0,   102,    61,     0,
       0,    97,    98,    99,     0,     0,     0,     0,     0,   225,
     101,     0,     0,     0,   102,    61,   152,   153,   154,   155,
     156,   157,   158,   159,   160,   161,   162,   163,   164,   165,
     166,   167,   168,   169,   170,   171,   172,   173,   174,   175,
     176,   177,   178,   179,   180,   181,   182,   183,   184,   185,
     186,   187,   188,   189,   190,   191,   192,   193,   194,   195,
     196,   197,   198,   199,   200,   201,   202,   203,   204,   205,
     206,   207,   208,   209,   210,   211,   212,   213,   228,   229,
       0,   230,   231,   232,   233,   234,   235,   236,   237,   238,
     239,   240,   241,   242,     0,     0,   493,   494,     0,     0,
       0,     0,     0,   214,   215,   216,   217,   218,   219,     0,
       0,     0,     0,     0,     0,   220,   221,     0,   222,   223,
     224,   331,   153,   154,   155,   156,   157,   158,   159,   160,
     161,   162,   163,   164,   165,   166,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,   177,   178,   179,   180,
     181,   182,   183,   184,   185,   186,   187,   188,   189,   190,
     191,   192,   193,   194,   195,   196,   197,   198,   199,   200,
     201,   202,   203,   204,   205,   206,   207,   208,   209,   210,
     211,   212,   213,   228,   229,     0,   230,   231,   232,   233,
     234,   235,   236,   237,   238,   239,   240,   241,   242,     0,
       0,   596,   597,     0,     0,     0,     0,     0,   214,   215,
     216,   217,   218,   219,   -64,     1,     0,     0,     0,     0,
     220,   221,     0,   222,   223,   224,     2,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,     0,    24,
      25,    26,    27,    28,     0,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,   228,   229,     0,   230,   231,   232,   233,   234,   235,
     236,   237,   238,   239,   240,   241,   242,     0,     0,   599,
     600,   228,   229,     0,   230,   231,   232,   233,   234,   235,
     236,   237,   238,   239,   240,   241,   242,     0,   228,   229,
     460,   230,   231,   232,   233,   234,   235,   236,   237,   238,
     239,   240,   241,   242,     0,   228,   229,   466,   230,   231,
     232,   233,   234,   235,   236,   237,   238,   239,   240,   241,
     242,     0,   228,   229,   469,   230,   231,   232,   233,   234,
     235,   236,   237,   238,   239,   240,   241,   242,     0,   228,
     229,   475,   230,   231,   232,   233,   234,   235,   236,   237,
     238,   239,   240,   241,   242,     0,   228,   229,   478,   230,
     231,   232,   233,   234,   235,   236,   237,   238,   239,   240,
     241,   242,     0,   228,   229,   482,   230,   231,   232,   233,
     234,   235,   236,   237,   238,   239,   240,   241,   242,     0,
     228,   229,   483,   230,   231,   232,   233,   234,   235,   236,
     237,   238,   239,   240,   241,   242,     0,   228,   229,   484,
     230,   231,   232,   233,   234,   235,   236,   237,   238,   239,
     240,   241,   242,     0,   228,   229,   485,   230,   231,   232,
     233,   234,   235,   236,   237,   238,   239,   240,   241,   242,
       0,   228,   229,   486,   230,   231,   232,   233,   234,   235,
     236,   237,   238,   239,   240,   241,   242,     0,   228,   229,
     495,   230,   231,   232,   233,   234,   235,   236,   237,   238,
     239,   240,   241,   242,     0,   228,   229,   496,   230,   231,
     232,   233,   234,   235,   236,   237,   238,   239,   240,   241,
     242,     0,   228,   229,   497,   230,   231,   232,   233,   234,
     235,   236,   237,   238,   239,   240,   241,   242,     0,   228,
     229,   498,   230,   231,   232,   233,   234,   235,   236,   237,
     238,   239,   240,   241,   242,     0,   228,   229,   504,   230,
     231,   232,   233,   234,   235,   236,   237,   238,   239,   240,
     241,   242,     0,   228,   229,   505,   230,   231,   232,   233,
     234,   235,   236,   237,   238,   239,   240,   241,   242,     0,
     228,   229,   506,   230,   231,   232,   233,   234,   235,   236,
     237,   238,   239,   240,   241,   242,     0,   228,   229,   507,
     230,   231,   232,   233,   234,   235,   236,   237,   238,   239,
     240,   241,   242,     0,   228,   229,   508,   230,   231,   232,
     233,   234,   235,   236,   237,   238,   239,   240,   241,   242,
       0,   228,   229,   510,   230,   231,   232,   233,   234,   235,
     236,   237,   238,   239,   240,   241,   242,     0,   228,   229,
     512,   230,   231,   232,   233,   234,   235,   236,   237,   238,
     239,   240,   241,   242,     0,   228,   229,   514,   230,   231,
     232,   233,   234,   235,   236,   237,   238,   239,   240,   241,
     242,     0,   228,   229,   516,   230,   231,   232,   233,   234,
     235,   236,   237,   238,   239,   240,   241,   242,     0,   228,
     229,   576,   230,   231,   232,   233,   234,   235,   236,   237,
     238,   239,   240,   241,   242,     0,   228,   229,   577,   230,
     231,   232,   233,   234,   235,   236,   237,   238,   239,   240,
     241,   242,     0,   228,   229,   578,   230,   231,   232,   233,
     234,   235,   236,   237,   238,   239,   240,   241,   242,     0,
     228,   229,   579,   230,   231,   232,   233,   234,   235,   236,
     237,   238,   239,   240,   241,   242,     0,   228,   229,   581,
     230,   231,   232,   233,   234,   235,   236,   237,   238,   239,
     240,   241,   242,     0,   228,   229,   582,   230,   231,   232,
     233,   234,   235,   236,   237,   238,   239,   240,   241,   242,
       0,   228,   229,   584,   230,   231,   232,   233,   234,   235,
     236,   237,   238,   239,   240,   241,   242,     0,   228,   229,
     592,   230,   231,   232,   233,   234,   235,   236,   237,   238,
     239,   240,   241,   242,     0,   228,   229,   594,   230,   231,
     232,   233,   234,   235,   236,   237,   238,   239,   240,   241,
     242,   228,   229,   332,   230,   231,   232,   233,   234,   235,
     236,   237,   238,   239,   240,   241,   242,   228,   229,   456,
     230,   231,   232,   233,   234,   235,   236,   237,   238,   239,
     240,   241,   242,   228,   229,   457,   230,   231,   232,   233,
     234,   235,   236,   237,   238,   239,   240,   241,   242,   228,
     229,   458,   230,   231,   232,   233,   234,   235,   236,   237,
     238,   239,   240,   241,   242,   228,   229,   459,   230,   231,
     232,   233,   234,   235,   236,   237,   238,   239,   240,   241,
     242,   228,   229,   461,   230,   231,   232,   233,   234,   235,
     236,   237,   238,   239,   240,   241,   242,   228,   229,   462,
     230,   231,   232,   233,   234,   235,   236,   237,   238,   239,
     240,   241,   242,   228,   229,   463,   230,   231,   232,   233,
     234,   235,   236,   237,   238,   239,   240,   241,   242,   228,
     229,   464,   230,   231,   232,   233,   234,   235,   236,   237,
     238,   239,   240,   241,   242,   228,   229,   465,   230,   231,
     232,   233,   234,   235,   236,   237,   238,   239,   240,   241,
     242,   228,   229,   467,   230,   231,   232,   233,   234,   235,
     236,   237,   238,   239,   240,   241,   242,   228,   229,   468,
     230,   231,   232,   233,   234,   235,   236,   237,   238,   239,
     240,   241,   242,   228,   229,   470,   230,   231,   232,   233,
     234,   235,   236,   237,   238,   239,   240,   241,   242,   228,
     229,   471,   230,   231,   232,   233,   234,   235,   236,   237,
     238,   239,   240,   241,   242,   228,   229,   472,   230,   231,
     232,   233,   234,   235,   236,   237,   238,   239,   240,   241,
     242,   228,   229,   473,   230,   231,   232,   233,   234,   235,
     236,   237,   238,   239,   240,   241,   242,   228,   229,   474,
     230,   231,   232,   233,   234,   235,   236,   237,   238,   239,
     240,   241,   242,   228,   229,   481,   230,   231,   232,   233,
     234,   235,   236,   237,   238,   239,   240,   241,   242,   228,
     229,   487,   230,   231,   232,   233,   234,   235,   236,   237,
     238,   239,   240,   241,   242,   228,   229,   488,   230,   231,
     232,   233,   234,   235,   236,   237,   238,   239,   240,   241,
     242,   228,   229,   489,   230,   231,   232,   233,   234,   235,
     236,   237,   238,   239,   240,   241,   242,   228,   229,   490,
     230,   231,   232,   233,   234,   235,   236,   237,   238,   239,
     240,   241,   242,   228,   229,   491,   230,   231,   232,   233,
     234,   235,   236,   237,   238,   239,   240,   241,   242,   228,
     229,   492,   230,   231,   232,   233,   234,   235,   236,   237,
     238,   239,   240,   241,   242,   228,   229,   499,   230,   231,
     232,   233,   234,   235,   236,   237,   238,   239,   240,   241,
     242,   228,   229,   500,   230,   231,   232,   233,   234,   235,
     236,   237,   238,   239,   240,   241,   242,   228,   229,   501,
     230,   231,   232,   233,   234,   235,   236,   237,   238,   239,
     240,   241,   242,   228,   229,   502,   230,   231,   232,   233,
     234,   235,   236,   237,   238,   239,   240,   241,   242,   228,
     229,   503,   230,   231,   232,   233,   234,   235,   236,   237,
     238,   239,   240,   241,   242,   228,   229,   518,   230,   231,
     232,   233,   234,   235,   236,   237,   238,   239,   240,   241,
     242,   228,   229,   519,   230,   231,   232,   233,   234,   235,
     236,   237,   238,   239,   240,   241,   242,   228,   229,   520,
     230,   231,   232,   233,   234,   235,   236,   237,   238,   239,
     240,   241,   242,   228,   229,   521,   230,   231,   232,   233,
     234,   235,   236,   237,   238,   239,   240,   241,   242,   228,
     229,   522,   230,   231,   232,   233,   234,   235,   236,   237,
     238,   239,   240,   241,   242,   228,   229,   562,   230,   231,
     232,   233,   234,   235,   236,   237,   238,   239,   240,   241,
     242,   228,   229,   563,   230,   231,   232,   233,   234,   235,
     236,   237,   238,   239,   240,   241,   242,   228,   229,   564,
     230,   231,   232,   233,   234,   235,   236,   237,   238,   239,
     240,   241,   242,   228,   229,   565,   230,   231,   232,   233,
     234,   235,   236,   237,   238,   239,   240,   241,   242,   228,
     229,   566,   230,   231,   232,   233,   234,   235,   236,   237,
     238,   239,   240,   241,   242,   228,   229,   567,   230,   231,
     232,   233,   234,   235,   236,   237,   238,   239,   240,   241,
     242,   228,   229,   568,   230,   231,   232,   233,   234,   235,
     236,   237,   238,   239,   240,   241,   242,   228,   229,   569,
     230,   231,   232,   233,   234,   235,   236,   237,   238,   239,
     240,   241,   242,   228,   229,   572,   230,   231,   232,   233,
     234,   235,   236,   237,   238,   239,   240,   241,   242,   228,
     229,   574,   230,   231,   232,   233,   234,   235,   236,   237,
     238,   239,   240,   241,   242,   228,   229,   575,   230,   231,
     232,   233,   234,   235,   236,   237,   238,   239,   240,   241,
     242,   228,   229,   580,   230,   231,   232,   233,   234,   235,
     236,   237,   238,   239,   240,   241,   242,   228,   229,   583,
     230,   231,   232,   233,   234,   235,   236,   237,   238,   239,
     240,   241,   242,   228,   229,   585,   230,   231,   232,   233,
     234,   235,   236,   237,   238,   239,   240,   241,   242,   228,
     229,   586,   230,   231,   232,   233,   234,   235,   236,   237,
     238,   239,   240,   241,   242,   228,   229,   587,   230,   231,
     232,   233,   234,   235,   236,   237,   238,   239,   240,   241,
     242,   228,   229,   588,   230,   231,   232,   233,   234,   235,
     236,   237,   238,   239,   240,   241,   242,   228,   229,   590,
     230,   231,   232,   233,   234,   235,   236,   237,   238,   239,
     240,   241,   242,   228,   229,   615,   230,   231,   232,   233,
     234,   235,   236,   237,   238,   239,   240,   241,   242,   228,
     229,   616,   230,   231,   232,   233,   234,   235,   236,   237,
     238,   239,   240,   241,   242,   228,   229,   617,   230,   231,
     232,   233,   234,   235,   236,   237,   238,   239,   240,   241,
     242,   228,   229,   618,   230,   231,   232,   233,   234,   235,
     236,   237,   238,   239,   240,   241,   242,   228,   229,   619,
     230,   231,   232,   233,   234,   235,   236,   237,   238,   239,
     240,   241,   242,   228,   229,   620,   230,   231,   232,   233,
     234,   235,   236,   237,   238,   239,   240,   241,   242,   228,
     229,   621,   230,   231,   232,   233,   234,   235,   236,   237,
     238,   239,   240,   241,   242,   228,   229,   622,   230,   231,
     232,   233,   234,   235,   236,   237,   238,   239,   240,   241,
     242,   228,   229,   623,   230,   231,   232,   233,   234,   235,
     236,   237,   238,   239,   240,   241,   242,   228,   229,   624,
     230,   231,   232,   233,   234,   235,   236,   237,   238,   239,
     240,   241,   242,   228,   229,   625,   230,   231,   232,   233,
     234,   235,   236,   237,   238,   239,   240,   241,   242,   228,
     229,   626,   230,   231,   232,   233,   234,   235,   236,   237,
     238,   239,   240,   241,   242,     0,     0,   627,   228,   229,
     437,   230,   231,   232,   233,   234,   235,   236,   237,   238,
     239,   240,   241,   242,   228,   229,     0,   230,   231,   232,
     233,   234,   235,   236,   237,   238,   239,   240,   241,   242,
     230,   231,   232,   233,   234,   235,   236,   237,   238,   239,
     240,   241,   242,   232,   233,   234,   235,   236,   237,   238,
     239,   240,   241,   242,  -198,  -198,  -198,  -198,   236,   237,
     238,   239,   240,   241,   242
};

static const yytype_int16 yycheck[] =
{
      28,    97,    98,    99,     5,     5,   102,     3,     4,     5,
       6,     5,     8,     3,     4,     3,     4,     7,     8,     5,
       8,   176,   177,   178,   179,     4,   166,     4,     7,     8,
       4,     5,     4,     5,     4,     4,     4,     7,     8,     8,
       5,     4,   182,   182,   183,   182,   183,     4,     3,     4,
       5,     6,     7,     8,     9,     5,   152,    81,    82,     3,
       4,     5,     6,     4,     8,     4,   166,     3,     0,     3,
       5,     6,     3,   101,    11,     3,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,     3,    29,    30,    31,    32,    21,    22,     3,     3,
       3,   166,   166,   166,   166,     4,    43,   166,   136,   137,
     138,   139,     3,   170,     3,     5,   170,    54,   170,    56,
     170,    13,    14,    15,    16,    17,    18,    19,    20,     3,
       3,    23,    24,    25,     3,    27,   184,    29,    30,    31,
      32,     3,     3,     3,     3,     3,   181,     4,    85,     5,
       5,    43,     5,     4,    91,     4,   181,     5,     5,     5,
     181,   181,    54,   159,    56,   181,    91,   181,   181,   181,
     181,   181,   166,   181,   170,   171,   172,   173,   174,   181,
       5,   170,   181,   181,   180,   181,   186,   184,   125,   185,
     186,     4,   186,    85,   181,   132,   186,   181,   186,   181,
     228,   229,   230,   231,   232,   233,   234,   186,   236,   237,
     238,   239,   240,   241,   242,   159,   186,   186,   173,   174,
     175,   186,   181,   178,     5,   181,   170,   181,   172,   173,
     174,   186,   181,   125,   181,   331,   180,   181,   173,   174,
       5,   185,   186,   181,     5,     5,   274,   275,   276,   277,
     278,   279,   280,   281,   282,   283,   284,   285,   286,   287,
     288,   289,   290,   291,   292,   293,   294,   295,   296,   297,
     298,   299,   300,   301,   302,   303,   304,   305,   306,   307,
     308,   309,   310,   311,   312,   313,   314,   315,   316,   317,
     318,   319,   320,   321,   322,   323,   324,   325,   326,   327,
     328,   329,   330,   181,   181,   181,   181,   179,   478,   337,
     338,   181,   183,   341,   181,   343,   181,   181,   181,    -1,
      -1,    -1,    -1,    -1,     3,     4,     5,     6,     7,     8,
     267,   268,   269,   270,   271,   272,   273,   181,   181,   181,
     436,   181,   267,   268,   269,   270,   271,     3,     4,     5,
       6,   181,     8,   181,   181,   181,   293,   294,   181,   181,
     181,   181,   181,   181,   181,   181,   181,   181,   293,   294,
     181,   181,   181,   181,   181,   267,   268,   269,   270,   271,
     272,   273,   181,   181,   321,   322,   323,   324,   325,   181,
     181,   181,   181,   181,   181,   181,   321,   322,   323,   324,
     325,   293,   294,   182,    -1,   183,   182,    -1,   183,   437,
     182,    -1,   183,   182,    -1,   183,   444,   183,   446,   182,
     448,   183,   450,   182,   452,     3,     4,     5,     6,   182,
       8,   182,   460,   182,    -1,   183,   183,   183,   466,   183,
     183,   469,     3,     4,     5,     6,   182,     8,   476,   183,
     183,   479,   182,   182,   482,   483,   484,   485,   486,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   494,   495,   496,   497,
     498,    -1,    -1,    -1,    -1,    -1,   504,   505,   506,   507,
     159,   509,    -1,   511,    -1,   513,    -1,   515,    -1,   517,
      -1,    -1,    -1,   172,   173,   174,    -1,    -1,    -1,    -1,
      -1,   180,   181,   159,    -1,    -1,   185,   186,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   172,   173,   174,    -1,
      -1,    -1,    -1,    -1,   180,   181,   182,    -1,    -1,   185,
     186,   168,   169,   170,   171,   172,   173,   174,   175,   176,
     177,   178,   179,   571,    -1,    -1,    -1,    -1,   576,   577,
     578,   579,    -1,   581,   582,    -1,   584,    -1,    -1,    -1,
      -1,    -1,    -1,   591,   592,   593,   594,    -1,    -1,   597,
      -1,   508,   600,   510,    -1,   512,    -1,   514,    -1,   516,
      -1,   159,    -1,   508,    -1,   510,    -1,   512,    -1,   514,
      -1,   516,    -1,    -1,   172,   173,   174,    -1,   159,    -1,
      -1,    -1,   180,   181,    -1,    -1,    -1,   185,   186,    -1,
      -1,   172,   173,   174,    -1,    -1,    -1,    -1,    -1,   180,
     181,    -1,    -1,    -1,   185,   186,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
     113,   114,   115,   116,   117,   118,   119,   120,   164,   165,
      -1,   167,   168,   169,   170,   171,   172,   173,   174,   175,
     176,   177,   178,   179,    -1,    -1,   182,   183,    -1,    -1,
      -1,    -1,    -1,   146,   147,   148,   149,   150,   151,    -1,
      -1,    -1,    -1,    -1,    -1,   158,   159,    -1,   161,   162,
     163,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,   120,   164,   165,    -1,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,   177,   178,   179,    -1,
      -1,   182,   183,    -1,    -1,    -1,    -1,    -1,   146,   147,
     148,   149,   150,   151,     0,     1,    -1,    -1,    -1,    -1,
     158,   159,    -1,   161,   162,   163,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    -1,    35,
      36,    37,    38,    39,    -1,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,   164,   165,    -1,   167,   168,   169,   170,   171,   172,
     173,   174,   175,   176,   177,   178,   179,    -1,    -1,   182,
     183,   164,   165,    -1,   167,   168,   169,   170,   171,   172,
     173,   174,   175,   176,   177,   178,   179,    -1,   164,   165,
     183,   167,   168,   169,   170,   171,   172,   173,   174,   175,
     176,   177,   178,   179,    -1,   164,   165,   183,   167,   168,
     169,   170,   171,   172,   173,   174,   175,   176,   177,   178,
     179,    -1,   164,   165,   183,   167,   168,   169,   170,   171,
     172,   173,   174,   175,   176,   177,   178,   179,    -1,   164,
     165,   183,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,   177,   178,   179,    -1,   164,   165,   183,   167,
     168,   169,   170,   171,   172,   173,   174,   175,   176,   177,
     178,   179,    -1,   164,   165,   183,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,   177,   178,   179,    -1,
     164,   165,   183,   167,   168,   169,   170,   171,   172,   173,
     174,   175,   176,   177,   178,   179,    -1,   164,   165,   183,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
     177,   178,   179,    -1,   164,   165,   183,   167,   168,   169,
     170,   171,   172,   173,   174,   175,   176,   177,   178,   179,
      -1,   164,   165,   183,   167,   168,   169,   170,   171,   172,
     173,   174,   175,   176,   177,   178,   179,    -1,   164,   165,
     183,   167,   168,   169,   170,   171,   172,   173,   174,   175,
     176,   177,   178,   179,    -1,   164,   165,   183,   167,   168,
     169,   170,   171,   172,   173,   174,   175,   176,   177,   178,
     179,    -1,   164,   165,   183,   167,   168,   169,   170,   171,
     172,   173,   174,   175,   176,   177,   178,   179,    -1,   164,
     165,   183,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,   177,   178,   179,    -1,   164,   165,   183,   167,
     168,   169,   170,   171,   172,   173,   174,   175,   176,   177,
     178,   179,    -1,   164,   165,   183,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,   177,   178,   179,    -1,
     164,   165,   183,   167,   168,   169,   170,   171,   172,   173,
     174,   175,   176,   177,   178,   179,    -1,   164,   165,   183,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
     177,   178,   179,    -1,   164,   165,   183,   167,   168,   169,
     170,   171,   172,   173,   174,   175,   176,   177,   178,   179,
      -1,   164,   165,   183,   167,   168,   169,   170,   171,   172,
     173,   174,   175,   176,   177,   178,   179,    -1,   164,   165,
     183,   167,   168,   169,   170,   171,   172,   173,   174,   175,
     176,   177,   178,   179,    -1,   164,   165,   183,   167,   168,
     169,   170,   171,   172,   173,   174,   175,   176,   177,   178,
     179,    -1,   164,   165,   183,   167,   168,   169,   170,   171,
     172,   173,   174,   175,   176,   177,   178,   179,    -1,   164,
     165,   183,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,   177,   178,   179,    -1,   164,   165,   183,   167,
     168,   169,   170,   171,   172,   173,   174,   175,   176,   177,
     178,   179,    -1,   164,   165,   183,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,   177,   178,   179,    -1,
     164,   165,   183,   167,   168,   169,   170,   171,   172,   173,
     174,   175,   176,   177,   178,   179,    -1,   164,   165,   183,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
     177,   178,   179,    -1,   164,   165,   183,   167,   168,   169,
     170,   171,   172,   173,   174,   175,   176,   177,   178,   179,
      -1,   164,   165,   183,   167,   168,   169,   170,   171,   172,
     173,   174,   175,   176,   177,   178,   179,    -1,   164,   165,
     183,   167,   168,   169,   170,   171,   172,   173,   174,   175,
     176,   177,   178,   179,    -1,   164,   165,   183,   167,   168,
     169,   170,   171,   172,   173,   174,   175,   176,   177,   178,
     179,   164,   165,   182,   167,   168,   169,   170,   171,   172,
     173,   174,   175,   176,   177,   178,   179,   164,   165,   182,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
     177,   178,   179,   164,   165,   182,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,   177,   178,   179,   164,
     165,   182,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,   177,   178,   179,   164,   165,   182,   167,   168,
     169,   170,   171,   172,   173,   174,   175,   176,   177,   178,
     179,   164,   165,   182,   167,   168,   169,   170,   171,   172,
     173,   174,   175,   176,   177,   178,   179,   164,   165,   182,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
     177,   178,   179,   164,   165,   182,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,   177,   178,   179,   164,
     165,   182,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,   177,   178,   179,   164,   165,   182,   167,   168,
     169,   170,   171,   172,   173,   174,   175,   176,   177,   178,
     179,   164,   165,   182,   167,   168,   169,   170,   171,   172,
     173,   174,   175,   176,   177,   178,   179,   164,   165,   182,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
     177,   178,   179,   164,   165,   182,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,   177,   178,   179,   164,
     165,   182,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,   177,   178,   179,   164,   165,   182,   167,   168,
     169,   170,   171,   172,   173,   174,   175,   176,   177,   178,
     179,   164,   165,   182,   167,   168,   169,   170,   171,   172,
     173,   174,   175,   176,   177,   178,   179,   164,   165,   182,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
     177,   178,   179,   164,   165,   182,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,   177,   178,   179,   164,
     165,   182,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,   177,   178,   179,   164,   165,   182,   167,   168,
     169,   170,   171,   172,   173,   174,   175,   176,   177,   178,
     179,   164,   165,   182,   167,   168,   169,   170,   171,   172,
     173,   174,   175,   176,   177,   178,   179,   164,   165,   182,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
     177,   178,   179,   164,   165,   182,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,   177,   178,   179,   164,
     165,   182,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,   177,   178,   179,   164,   165,   182,   167,   168,
     169,   170,   171,   172,   173,   174,   175,   176,   177,   178,
     179,   164,   165,   182,   167,   168,   169,   170,   171,   172,
     173,   174,   175,   176,   177,   178,   179,   164,   165,   182,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
     177,   178,   179,   164,   165,   182,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,   177,   178,   179,   164,
     165,   182,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,   177,   178,   179,   164,   165,   182,   167,   168,
     169,   170,   171,   172,   173,   174,   175,   176,   177,   178,
     179,   164,   165,   182,   167,   168,   169,   170,   171,   172,
     173,   174,   175,   176,   177,   178,   179,   164,   165,   182,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
     177,   178,   179,   164,   165,   182,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,   177,   178,   179,   164,
     165,   182,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,   177,   178,   179,   164,   165,   182,   167,   168,
     169,   170,   171,   172,   173,   174,   175,   176,   177,   178,
     179,   164,   165,   182,   167,   168,   169,   170,   171,   172,
     173,   174,   175,   176,   177,   178,   179,   164,   165,   182,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
     177,   178,   179,   164,   165,   182,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,   177,   178,   179,   164,
     165,   182,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,   177,   178,   179,   164,   165,   182,   167,   168,
     169,   170,   171,   172,   173,   174,   175,   176,   177,   178,
     179,   164,   165,   182,   167,   168,   169,   170,   171,   172,
     173,   174,   175,   176,   177,   178,   179,   164,   165,   182,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
     177,   178,   179,   164,   165,   182,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,   177,   178,   179,   164,
     165,   182,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,   177,   178,   179,   164,   165,   182,   167,   168,
     169,   170,   171,   172,   173,   174,   175,   176,   177,   178,
     179,   164,   165,   182,   167,   168,   169,   170,   171,   172,
     173,   174,   175,   176,   177,   178,   179,   164,   165,   182,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
     177,   178,   179,   164,   165,   182,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,   177,   178,   179,   164,
     165,   182,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,   177,   178,   179,   164,   165,   182,   167,   168,
     169,   170,   171,   172,   173,   174,   175,   176,   177,   178,
     179,   164,   165,   182,   167,   168,   169,   170,   171,   172,
     173,   174,   175,   176,   177,   178,   179,   164,   165,   182,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
     177,   178,   179,   164,   165,   182,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,   177,   178,   179,   164,
     165,   182,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,   177,   178,   179,   164,   165,   182,   167,   168,
     169,   170,   171,   172,   173,   174,   175,   176,   177,   178,
     179,   164,   165,   182,   167,   168,   169,   170,   171,   172,
     173,   174,   175,   176,   177,   178,   179,   164,   165,   182,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
     177,   178,   179,   164,   165,   182,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,   177,   178,   179,   164,
     165,   182,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,   177,   178,   179,   164,   165,   182,   167,   168,
     169,   170,   171,   172,   173,   174,   175,   176,   177,   178,
     179,   164,   165,   182,   167,   168,   169,   170,   171,   172,
     173,   174,   175,   176,   177,   178,   179,   164,   165,   182,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
     177,   178,   179,   164,   165,   182,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,   177,   178,   179,   164,
     165,   182,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,   177,   178,   179,    -1,    -1,   182,   164,   165,
     166,   167,   168,   169,   170,   171,   172,   173,   174,   175,
     176,   177,   178,   179,   164,   165,    -1,   167,   168,   169,
     170,   171,   172,   173,   174,   175,   176,   177,   178,   179,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
     177,   178,   179,   169,   170,   171,   172,   173,   174,   175,
     176,   177,   178,   179,   169,   170,   171,   172,   173,   174,
     175,   176,   177,   178,   179
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     1,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    35,    36,    37,    38,    39,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,   188,     4,     5,     4,     5,
       5,     5,     4,     4,     4,     4,     5,     3,     4,     7,
       8,   186,   192,   193,   194,     4,   194,   194,   194,   194,
     194,   194,   194,   194,   192,   193,   192,     3,     5,     6,
       9,   173,   174,   175,   178,   194,   195,   194,   194,     3,
     193,   196,   194,     3,     5,     6,   159,   172,   173,   174,
     180,   181,   185,   189,   190,   193,   194,   194,   194,   194,
       4,     3,     3,     3,     3,     3,     3,     3,   194,     3,
       0,   166,   166,   166,   166,   194,   166,     4,   194,     5,
     186,     4,   166,     3,     5,     3,   170,   170,   170,   170,
       3,   195,   195,     3,     3,   194,   192,   193,   184,   189,
     189,   189,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,   119,   120,   146,   147,   148,   149,   150,   151,
     158,   159,   161,   162,   163,   180,   190,   189,   164,   165,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
     177,   178,   179,     3,     3,     3,     3,     3,     5,     5,
       5,     4,     4,   194,     4,   166,     5,     5,   186,   193,
       5,   190,   190,   190,   190,     5,   189,   181,   181,   181,
     181,   181,   181,   181,   181,   181,   181,   181,   181,   181,
     181,   181,   181,   181,   181,   181,   181,   181,   181,   181,
     181,   181,   181,   181,   181,   181,   181,   181,   181,   181,
     181,   181,   181,   181,   181,   181,   181,   181,   181,   181,
     181,   181,   181,   181,   181,   181,   181,   181,   181,   181,
     181,   181,   181,   181,   181,   181,   181,   181,   181,   181,
     181,    59,   182,   190,   190,   190,   190,   170,   171,   190,
     190,   170,   190,   170,   190,   190,   190,   190,   190,   190,
     190,     5,     4,     5,     5,   184,   192,   194,   192,   194,
     192,   194,   192,   194,   192,   194,   194,   194,   190,   190,
     190,   190,   190,   190,   190,   190,   190,   190,   190,   190,
     190,   190,   190,   190,   190,   190,   190,   190,   192,   193,
     194,   190,   192,   194,   190,   190,   190,   190,   190,   190,
     190,   190,   190,   190,   190,   190,   190,   190,   190,   190,
     190,   190,   190,   190,   190,   190,   190,   190,   190,   190,
     190,   192,   193,   190,   192,   190,   192,   190,   192,   190,
     192,   190,   190,   190,   190,   190,   182,   166,   190,   190,
     190,   190,     5,     5,   183,   182,   183,   182,   183,   182,
     183,   182,   183,   182,   182,   182,   182,   182,   182,   182,
     183,   182,   182,   182,   182,   182,   183,   182,   182,   183,
     182,   182,   182,   182,   182,   183,   183,   182,   183,   183,
     182,   182,   183,   183,   183,   183,   183,   182,   182,   182,
     182,   182,   182,   182,   183,   183,   183,   183,   183,   182,
     182,   182,   182,   182,   183,   183,   183,   183,   183,   183,
     183,   183,   183,   183,   183,   183,   183,   183,   182,   182,
     182,   182,   182,   189,   190,   190,   190,   190,   190,   190,
     190,   190,   190,   190,   191,   190,   191,   190,   190,   190,
     190,   190,   190,   190,   190,   190,   190,   190,   190,   190,
     190,   190,   192,   190,   192,   190,   192,   190,   192,   190,
     192,   190,   182,   182,   182,   182,   182,   182,   182,   182,
     182,   183,   182,   182,   182,   182,   183,   183,   183,   183,
     182,   183,   183,   182,   183,   182,   182,   182,   182,   182,
     182,   183,   183,   183,   183,   182,   182,   183,   182,   182,
     183,   190,   190,   190,   190,   190,   190,   190,   190,   190,
     190,   190,   190,   190,   190,   182,   182,   182,   182,   182,
     182,   182,   182,   182,   182,   182,   182,   182
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,   187,   188,   188,   188,   188,   188,   188,   188,   188,
     188,   188,   188,   188,   188,   188,   188,   188,   188,   188,
     188,   188,   188,   188,   188,   188,   188,   188,   188,   188,
     188,   188,   188,   188,   188,   188,   188,   188,   188,   188,
     188,   188,   188,   188,   188,   188,   188,   188,   188,   188,
     188,   188,   188,   188,   188,   188,   188,   188,   188,   188,
     188,   188,   188,   188,   188,   188,   189,   189,   189,   189,
     189,   189,   189,   189,   189,   189,   189,   189,   189,   189,
     189,   189,   189,   189,   189,   189,   189,   189,   189,   189,
     189,   189,   189,   189,   189,   189,   189,   189,   189,   189,
     189,   189,   189,   189,   189,   189,   189,   189,   189,   189,
     189,   189,   189,   189,   189,   189,   189,   189,   189,   189,
     189,   189,   189,   189,   189,   189,   189,   189,   189,   189,
     189,   189,   189,   189,   189,   189,   189,   189,   189,   189,
     189,   189,   189,   189,   189,   189,   189,   189,   189,   189,
     189,   189,   189,   189,   189,   189,   189,   189,   189,   189,
     189,   189,   189,   189,   189,   189,   189,   189,   190,   190,
     190,   190,   190,   190,   190,   190,   190,   190,   190,   190,
     190,   190,   190,   190,   190,   190,   190,   191,   191,   192,
     192,   193,   193,   193,   193,   193,   194,   194,   195,   195,
     195,   195,   196,   196
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     4,     3,     4,     4,     4,     2,     2,     2,
       5,     3,     3,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     4,     4,     4,     4,     3,     4,     3,     2,
       4,     2,     3,     2,     2,     2,     3,     3,     2,     2,
       2,     3,     3,     3,     3,     2,     2,     2,     3,     5,
       5,     3,     3,     3,     3,     2,     2,     1,     1,     2,
       2,     2,     2,     2,     0,     1,     1,     3,     5,     5,
       7,     5,     7,     5,     7,     5,     7,     5,     7,     5,
       7,     7,     5,     7,     7,     5,     5,     5,     5,     5,
       5,     7,     5,     5,     5,     5,     5,     7,     5,     5,
       7,     5,     5,     5,     5,     5,     5,     7,     9,     9,
       9,     9,     5,     5,     5,     5,     5,     5,     2,     9,
       5,     9,     5,     5,     7,     5,     7,     7,     5,     5,
       5,     7,     7,     9,     7,     7,     9,     9,     9,     9,
       7,     7,     9,     7,     7,     7,     9,     3,     2,     2,
       1,     1,     2,     1,     2,     2,     5,     2,     2,     2,
       2,     5,     5,     5,     5,     2,     2,     1,     3,     3,
       3,     3,     3,     3,     1,     5,     3,     3,     3,     3,
       3,     3,     4,     4,     4,     4,     3,     1,     3,     3,
       1,     2,     3,     3,     4,     1,     1,     1,     1,     1,
       2,     2,     1,     1
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
do                                                              \
  if (yychar == YYEMPTY)                                        \
    {                                                           \
      yychar = (Token);                                         \
      yylval = (Value);                                         \
      YYPOPSTACK (yylen);                                       \
      yystate = *yyssp;                                         \
      goto yybackup;                                            \
    }                                                           \
  else                                                          \
    {                                                           \
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;                                                  \
    }                                                           \
while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256



/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  YYUSE (yytype);
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyoutput, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, int yyrule)
{
  unsigned long int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[yyssp[yyi + 1 - yynrhs]],
                       &(yyvsp[(yyi + 1) - (yynrhs)])
                                              );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
yystrlen (const char *yystr)
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
        switch (*++yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
            /* Fall through.  */
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
{
  YYUSE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        YYSTYPE *yyvs1 = yyvs;
        yytype_int16 *yyss1 = yyss;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * sizeof (*yyssp),
                    &yyvs1, yysize * sizeof (*yyvsp),
                    &yystacksize);

        yyss = yyss1;
        yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yytype_int16 *yyss1 = yyss;
        union yyalloc *yyptr =
          (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
                  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
#line 321 "gram.y" /* yacc.c:1646  */
    { let((yyvsp[-2].rval).left.vp, (yyvsp[0].enode)); }
#line 2374 "y.tab.c" /* yacc.c:1646  */
    break;

  case 3:
#line 323 "gram.y" /* yacc.c:1646  */
    { (yyvsp[-1].rval).left.vp->v = (double) 0.0;
                                  if ((yyvsp[-1].rval).left.vp->expr && !((yyvsp[-1].rval).left.vp->flags & is_strexpr)) {
                                    efree((yyvsp[-1].rval).left.vp->expr);
                                    (yyvsp[-1].rval).left.vp->expr = NULL;
                                  }
                                  (yyvsp[-1].rval).left.vp->cellerror = CELLOK;
                                  (yyvsp[-1].rval).left.vp->flags &= ~is_valid;
                                  (yyvsp[-1].rval).left.vp->flags |= is_changed;
                                  changed++;
                                  modflg++;
                                  }
#line 2390 "y.tab.c" /* yacc.c:1646  */
    break;

  case 4:
#line 335 "gram.y" /* yacc.c:1646  */
    { slet((yyvsp[-2].rval).left.vp, (yyvsp[0].enode), 0); }
#line 2396 "y.tab.c" /* yacc.c:1646  */
    break;

  case 5:
#line 336 "gram.y" /* yacc.c:1646  */
    { slet((yyvsp[-2].rval).left.vp, (yyvsp[0].enode), -1); }
#line 2402 "y.tab.c" /* yacc.c:1646  */
    break;

  case 6:
#line 337 "gram.y" /* yacc.c:1646  */
    { slet((yyvsp[-2].rval).left.vp, (yyvsp[0].enode), 1); }
#line 2408 "y.tab.c" /* yacc.c:1646  */
    break;

  case 7:
#line 338 "gram.y" /* yacc.c:1646  */
    { ljustify((yyvsp[0].rval).left.vp->row, (yyvsp[0].rval).left.vp->col, (yyvsp[0].rval).right.vp->row, (yyvsp[0].rval).right.vp->col); }
#line 2414 "y.tab.c" /* yacc.c:1646  */
    break;

  case 8:
#line 339 "gram.y" /* yacc.c:1646  */
    { rjustify((yyvsp[0].rval).left.vp->row, (yyvsp[0].rval).left.vp->col, (yyvsp[0].rval).right.vp->row, (yyvsp[0].rval).right.vp->col); }
#line 2420 "y.tab.c" /* yacc.c:1646  */
    break;

  case 9:
#line 340 "gram.y" /* yacc.c:1646  */
    { center((yyvsp[0].rval).left.vp->row, (yyvsp[0].rval).left.vp->col, (yyvsp[0].rval).right.vp->row, (yyvsp[0].rval).right.vp->col); }
#line 2426 "y.tab.c" /* yacc.c:1646  */
    break;

  case 10:
#line 341 "gram.y" /* yacc.c:1646  */
    { doformat((yyvsp[-3].ival),(yyvsp[-3].ival),(yyvsp[-2].ival),(yyvsp[-1].ival),(yyvsp[0].ival)); }
#line 2432 "y.tab.c" /* yacc.c:1646  */
    break;

  case 11:
#line 342 "gram.y" /* yacc.c:1646  */
    { format_cell((yyvsp[-1].rval).left.vp, (yyvsp[-1].rval).right.vp, (yyvsp[0].sval));
                                       scxfree((yyvsp[0].sval)); 
                                     }
#line 2440 "y.tab.c" /* yacc.c:1646  */
    break;

  case 12:
#line 346 "gram.y" /* yacc.c:1646  */
    { dateformat((yyvsp[-1].rval).left.vp, (yyvsp[-1].rval).right.vp, (yyvsp[0].sval));
                                       scxfree((yyvsp[0].sval)); }
#line 2447 "y.tab.c" /* yacc.c:1646  */
    break;

  case 13:
#line 348 "gram.y" /* yacc.c:1646  */
    { dateformat(lookat(currow, curcol), lookat(currow, curcol), (yyvsp[0].sval));
                                       scxfree((yyvsp[0].sval)); }
#line 2454 "y.tab.c" /* yacc.c:1646  */
    break;

  case 14:
#line 351 "gram.y" /* yacc.c:1646  */
    { hide_col((yyvsp[0].ival), 1); }
#line 2460 "y.tab.c" /* yacc.c:1646  */
    break;

  case 15:
#line 352 "gram.y" /* yacc.c:1646  */
    { hide_row((yyvsp[0].ival), 1); }
#line 2466 "y.tab.c" /* yacc.c:1646  */
    break;

  case 16:
#line 353 "gram.y" /* yacc.c:1646  */
    { show_col((yyvsp[0].ival), 1); }
#line 2472 "y.tab.c" /* yacc.c:1646  */
    break;

  case 17:
#line 354 "gram.y" /* yacc.c:1646  */
    { show_row((yyvsp[0].ival), 1); }
#line 2478 "y.tab.c" /* yacc.c:1646  */
    break;

  case 18:
#line 357 "gram.y" /* yacc.c:1646  */
    {
                                       hide_col((yyvsp[0].ival), 1); }
#line 2485 "y.tab.c" /* yacc.c:1646  */
    break;

  case 19:
#line 359 "gram.y" /* yacc.c:1646  */
    {
                                       show_col((yyvsp[0].ival), 1); }
#line 2492 "y.tab.c" /* yacc.c:1646  */
    break;

  case 20:
#line 361 "gram.y" /* yacc.c:1646  */
    {
                                       hide_row((yyvsp[0].ival), 1); }
#line 2499 "y.tab.c" /* yacc.c:1646  */
    break;

  case 21:
#line 363 "gram.y" /* yacc.c:1646  */
    {
                                       show_row((yyvsp[0].ival), 1); }
#line 2506 "y.tab.c" /* yacc.c:1646  */
    break;

  case 22:
#line 365 "gram.y" /* yacc.c:1646  */
    {
                                       show_col((yyvsp[-2].ival), (yyvsp[0].ival)-(yyvsp[-2].ival)+1); }
#line 2513 "y.tab.c" /* yacc.c:1646  */
    break;

  case 23:
#line 367 "gram.y" /* yacc.c:1646  */
    {
                                       show_row((yyvsp[-2].ival), (yyvsp[0].ival)-(yyvsp[-2].ival)+1); }
#line 2520 "y.tab.c" /* yacc.c:1646  */
    break;

  case 24:
#line 369 "gram.y" /* yacc.c:1646  */
    {
                                       int c = curcol, arg;
                                       if ((yyvsp[-2].ival) < (yyvsp[0].ival)) {
                                            curcol = (yyvsp[-2].ival);
                                            arg = (yyvsp[0].ival) - (yyvsp[-2].ival) + 1;
                                       } else {
                                            curcol = (yyvsp[0].ival);
                                            arg = (yyvsp[-2].ival) - (yyvsp[0].ival) + 1;
                                       }
                                       hide_col((yyvsp[-2].ival), arg);        // hide de un rango de columnas
                                       curcol = c < curcol ? c : c < curcol + arg ? curcol : c - arg;
                                     }
#line 2537 "y.tab.c" /* yacc.c:1646  */
    break;

  case 25:
#line 381 "gram.y" /* yacc.c:1646  */
    {
                                       int r = currow, arg;      // hide de un rango de filas
                                       if ((yyvsp[-2].ival) < (yyvsp[0].ival)) {
                                           currow = (yyvsp[-2].ival);
                                           arg = (yyvsp[0].ival) - (yyvsp[-2].ival) + 1;
                                       } else {
                                           currow = (yyvsp[0].ival);
                                           arg = (yyvsp[-2].ival) - (yyvsp[0].ival) + 1;
                                       }
                                       hide_row((yyvsp[-2].ival), arg);
                                       currow = r < currow ? r : r < currow + arg ? currow : r - arg;
                                     }
#line 2554 "y.tab.c" /* yacc.c:1646  */
    break;

  case 26:
#line 394 "gram.y" /* yacc.c:1646  */
    { set_cell_mark((yyvsp[-1].ival) + 97, (yyvsp[0].rval).left.vp->row, (yyvsp[0].rval).left.vp->col); }
#line 2560 "y.tab.c" /* yacc.c:1646  */
    break;

  case 27:
#line 396 "gram.y" /* yacc.c:1646  */
    { ;
                                          srange * sr = create_range('\0', '\0', (yyvsp[-1].rval).left.vp, (yyvsp[0].rval).left.vp);
                                          unselect_ranges();
                                          set_range_mark((yyvsp[-2].ival) + 97, sr);
                                          }
#line 2570 "y.tab.c" /* yacc.c:1646  */
    break;

  case 28:
#line 402 "gram.y" /* yacc.c:1646  */
    { sortrange((yyvsp[-1].rval).left.vp, (yyvsp[-1].rval).right.vp, (yyvsp[0].sval));
                                          //scxfree($3);
                                     }
#line 2578 "y.tab.c" /* yacc.c:1646  */
    break;

  case 29:
#line 405 "gram.y" /* yacc.c:1646  */
    { enable_filters((yyvsp[0].rval).left.vp, (yyvsp[0].rval).right.vp);
                                     }
#line 2585 "y.tab.c" /* yacc.c:1646  */
    break;

  case 30:
#line 422 "gram.y" /* yacc.c:1646  */
    { auto_justify((yyvsp[-2].ival), (yyvsp[0].ival), DEFWIDTH); }
#line 2591 "y.tab.c" /* yacc.c:1646  */
    break;

  case 31:
#line 423 "gram.y" /* yacc.c:1646  */
    { auto_justify((yyvsp[0].ival), (yyvsp[0].ival), DEFWIDTH); }
#line 2597 "y.tab.c" /* yacc.c:1646  */
    break;

  case 32:
#line 425 "gram.y" /* yacc.c:1646  */
    { moveto((yyvsp[-1].rval).left.vp->row, (yyvsp[-1].rval).left.vp->col, (yyvsp[-1].rval).right.vp->row, (yyvsp[-1].rval).right.vp->col, (yyvsp[0].rval).left.vp->row, (yyvsp[0].rval).left.vp->col); }
#line 2603 "y.tab.c" /* yacc.c:1646  */
    break;

  case 33:
#line 426 "gram.y" /* yacc.c:1646  */
    { moveto((yyvsp[0].rval).left.vp->row, (yyvsp[0].rval).left.vp->col, (yyvsp[0].rval).right.vp->row, (yyvsp[0].rval).right.vp->col, -1, -1); }
#line 2609 "y.tab.c" /* yacc.c:1646  */
    break;

  case 34:
#line 427 "gram.y" /* yacc.c:1646  */
    { num_search((yyvsp[0].fval), 0, 0, maxrow, maxcol, 0, 1); }
#line 2615 "y.tab.c" /* yacc.c:1646  */
    break;

  case 35:
#line 428 "gram.y" /* yacc.c:1646  */
    { str_search((yyvsp[0].sval), 0, 0, maxrow, maxcol, 0, 1); }
#line 2621 "y.tab.c" /* yacc.c:1646  */
    break;

  case 36:
#line 429 "gram.y" /* yacc.c:1646  */
    { str_search((yyvsp[0].sval), 0, 0, maxrow, maxcol, 1, 1); }
#line 2627 "y.tab.c" /* yacc.c:1646  */
    break;

  case 37:
#line 430 "gram.y" /* yacc.c:1646  */
    { str_search((yyvsp[0].sval), 0, 0, maxrow, maxcol, 2, 1); }
#line 2633 "y.tab.c" /* yacc.c:1646  */
    break;

  case 38:
#line 431 "gram.y" /* yacc.c:1646  */
    { /* don't repeat last goto on "unintelligible word" */ ; }
#line 2639 "y.tab.c" /* yacc.c:1646  */
    break;

  case 39:
#line 434 "gram.y" /* yacc.c:1646  */
    { lock_cells((yyvsp[0].rval).left.vp, (yyvsp[0].rval).right.vp); }
#line 2645 "y.tab.c" /* yacc.c:1646  */
    break;

  case 40:
#line 435 "gram.y" /* yacc.c:1646  */
    { unlock_cells((yyvsp[0].rval).left.vp, (yyvsp[0].rval).right.vp); }
#line 2651 "y.tab.c" /* yacc.c:1646  */
    break;

  case 41:
#line 436 "gram.y" /* yacc.c:1646  */
    {
                                          add_map((yyvsp[-1].sval), (yyvsp[0].sval), NORMAL_MODE, 1);
                                          scxfree((yyvsp[-1].sval));
                                          scxfree((yyvsp[0].sval));
                                        }
#line 2661 "y.tab.c" /* yacc.c:1646  */
    break;

  case 42:
#line 443 "gram.y" /* yacc.c:1646  */
    {
                                          add_map((yyvsp[-1].sval), (yyvsp[0].sval), INSERT_MODE, 1);
                                          scxfree((yyvsp[-1].sval));
                                          scxfree((yyvsp[0].sval));
                                        }
#line 2671 "y.tab.c" /* yacc.c:1646  */
    break;

  case 43:
#line 448 "gram.y" /* yacc.c:1646  */
    {
                                          add_map((yyvsp[-1].sval), (yyvsp[0].sval), NORMAL_MODE, 0);
                                          scxfree((yyvsp[-1].sval));
                                          scxfree((yyvsp[0].sval));
                                        }
#line 2681 "y.tab.c" /* yacc.c:1646  */
    break;

  case 44:
#line 454 "gram.y" /* yacc.c:1646  */
    {
                                          add_map((yyvsp[-1].sval), (yyvsp[0].sval), INSERT_MODE, 0);
                                          scxfree((yyvsp[-1].sval));
                                          scxfree((yyvsp[0].sval));
                                        }
#line 2691 "y.tab.c" /* yacc.c:1646  */
    break;

  case 45:
#line 461 "gram.y" /* yacc.c:1646  */
    {
                                          del_map((yyvsp[0].sval), NORMAL_MODE);
                                          scxfree((yyvsp[0].sval));
                                        }
#line 2700 "y.tab.c" /* yacc.c:1646  */
    break;

  case 46:
#line 466 "gram.y" /* yacc.c:1646  */
    {
                                          del_map((yyvsp[0].sval), INSERT_MODE);
                                          scxfree((yyvsp[0].sval));
                                        }
#line 2709 "y.tab.c" /* yacc.c:1646  */
    break;

  case 47:
#line 471 "gram.y" /* yacc.c:1646  */
    {
#ifdef USECOLORS
                                          chg_color((yyvsp[0].sval));
#endif
                                          scxfree((yyvsp[0].sval));
                                        }
#line 2720 "y.tab.c" /* yacc.c:1646  */
    break;

  case 48:
#line 478 "gram.y" /* yacc.c:1646  */
    {
#ifdef USECOLORS
                                          if ( ! atoi(get_conf_value("nocurses")))
                                              color_cell((yyvsp[-1].rval).left.vp->row, (yyvsp[-1].rval).left.vp->col, (yyvsp[-1].rval).right.vp->row, (yyvsp[-1].rval).right.vp->col, (yyvsp[0].sval));
#endif
                                          scxfree((yyvsp[0].sval));
                                        }
#line 2732 "y.tab.c" /* yacc.c:1646  */
    break;

  case 49:
#line 486 "gram.y" /* yacc.c:1646  */
    {
                                         redefine_color((yyvsp[-3].sval), (yyvsp[-2].ival), (yyvsp[-1].ival), (yyvsp[0].ival));
                                         scxfree((yyvsp[-3].sval)); }
#line 2740 "y.tab.c" /* yacc.c:1646  */
    break;

  case 50:
#line 490 "gram.y" /* yacc.c:1646  */
    { pad((yyvsp[-3].ival), 0, (yyvsp[-2].ival), maxrow, (yyvsp[0].ival)); }
#line 2746 "y.tab.c" /* yacc.c:1646  */
    break;

  case 51:
#line 491 "gram.y" /* yacc.c:1646  */
    { pad((yyvsp[-1].ival), 0, (yyvsp[0].ival), maxrow, (yyvsp[0].ival)); }
#line 2752 "y.tab.c" /* yacc.c:1646  */
    break;

  case 52:
#line 492 "gram.y" /* yacc.c:1646  */
    { pad((yyvsp[-1].ival), (yyvsp[0].rval).left.vp->row, (yyvsp[0].rval).left.vp->col, (yyvsp[0].rval).right.vp->row, (yyvsp[0].rval).right.vp->col); }
#line 2758 "y.tab.c" /* yacc.c:1646  */
    break;

  case 53:
#line 506 "gram.y" /* yacc.c:1646  */
    { add_range((yyvsp[-1].sval), (yyvsp[0].rval).left, (yyvsp[0].rval).right, 1); }
#line 2764 "y.tab.c" /* yacc.c:1646  */
    break;

  case 54:
#line 507 "gram.y" /* yacc.c:1646  */
    { add_range((yyvsp[-1].sval), (yyvsp[0].ent), (yyvsp[0].ent), 0); }
#line 2770 "y.tab.c" /* yacc.c:1646  */
    break;

  case 55:
#line 509 "gram.y" /* yacc.c:1646  */
    { del_range((yyvsp[0].rval).left.vp, (yyvsp[0].rval).right.vp); }
#line 2776 "y.tab.c" /* yacc.c:1646  */
    break;

  case 56:
#line 511 "gram.y" /* yacc.c:1646  */
    { eval_result = eval((yyvsp[0].enode));
                                      efree((yyvsp[0].enode));
                                    }
#line 2784 "y.tab.c" /* yacc.c:1646  */
    break;

  case 57:
#line 514 "gram.y" /* yacc.c:1646  */
    { printf("quitting. unsaved changes will be lost.\n");
                                      shall_quit = 2; // unsaved changes are lost!
                                    }
#line 2792 "y.tab.c" /* yacc.c:1646  */
    break;

  case 58:
#line 520 "gram.y" /* yacc.c:1646  */
    {
                                      EvalAll();
                                      //update(1);
                                      //changed = 0;
                                    }
#line 2802 "y.tab.c" /* yacc.c:1646  */
    break;

  case 59:
#line 525 "gram.y" /* yacc.c:1646  */
    {
                                      getnum((yyvsp[0].rval).left.vp->row, (yyvsp[0].rval).left.vp->col, (yyvsp[0].rval).right.vp->row, (yyvsp[0].rval).right.vp->col, fdoutput);
                                    }
#line 2810 "y.tab.c" /* yacc.c:1646  */
    break;

  case 60:
#line 529 "gram.y" /* yacc.c:1646  */
    { getstring((yyvsp[0].rval).left.vp->row, (yyvsp[0].rval).left.vp->col, (yyvsp[0].rval).right.vp->row, (yyvsp[0].rval).right.vp->col, fdoutput); }
#line 2816 "y.tab.c" /* yacc.c:1646  */
    break;

  case 61:
#line 531 "gram.y" /* yacc.c:1646  */
    { getexp((yyvsp[0].rval).left.vp->row, (yyvsp[0].rval).left.vp->col, (yyvsp[0].rval).right.vp->row, (yyvsp[0].rval).right.vp->col, fdoutput); }
#line 2822 "y.tab.c" /* yacc.c:1646  */
    break;

  case 62:
#line 533 "gram.y" /* yacc.c:1646  */
    { getformat((yyvsp[0].ival), fdoutput); }
#line 2828 "y.tab.c" /* yacc.c:1646  */
    break;

  case 63:
#line 535 "gram.y" /* yacc.c:1646  */
    { getfmt((yyvsp[0].rval).left.vp->row, (yyvsp[0].rval).left.vp->col, (yyvsp[0].rval).right.vp->row, (yyvsp[0].rval).right.vp->col, fdoutput); }
#line 2834 "y.tab.c" /* yacc.c:1646  */
    break;

  case 66:
#line 578 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new_var(O_VAR, (yyvsp[0].ent)); }
#line 2840 "y.tab.c" /* yacc.c:1646  */
    break;

  case 67:
#line 579 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new('f', (yyvsp[0].enode), ENULL); }
#line 2846 "y.tab.c" /* yacc.c:1646  */
    break;

  case 68:
#line 581 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new('F', (yyvsp[0].enode), ENULL); }
#line 2852 "y.tab.c" /* yacc.c:1646  */
    break;

  case 69:
#line 583 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(SUM,
                                        new_range(REDUCE | SUM, (yyvsp[-1].rval)), ENULL); }
#line 2859 "y.tab.c" /* yacc.c:1646  */
    break;

  case 70:
#line 586 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(SUM,
                                        new_range(REDUCE | SUM, (yyvsp[-3].rval)), (yyvsp[-1].enode)); }
#line 2866 "y.tab.c" /* yacc.c:1646  */
    break;

  case 71:
#line 589 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(PROD,
                                        new_range(REDUCE | PROD, (yyvsp[-1].rval)), ENULL); }
#line 2873 "y.tab.c" /* yacc.c:1646  */
    break;

  case 72:
#line 592 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(PROD,
                                        new_range(REDUCE | PROD, (yyvsp[-3].rval)), (yyvsp[-1].enode)); }
#line 2880 "y.tab.c" /* yacc.c:1646  */
    break;

  case 73:
#line 595 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(AVG,
                                        new_range(REDUCE | AVG, (yyvsp[-1].rval)), ENULL); }
#line 2887 "y.tab.c" /* yacc.c:1646  */
    break;

  case 74:
#line 598 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(AVG,
                                        new_range(REDUCE | AVG, (yyvsp[-3].rval)), (yyvsp[-1].enode)); }
#line 2894 "y.tab.c" /* yacc.c:1646  */
    break;

  case 75:
#line 601 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(STDDEV,
                                        new_range(REDUCE | STDDEV, (yyvsp[-1].rval)), ENULL); }
#line 2901 "y.tab.c" /* yacc.c:1646  */
    break;

  case 76:
#line 604 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(STDDEV,
                                        new_range(REDUCE | STDDEV, (yyvsp[-3].rval)), (yyvsp[-1].enode)); }
#line 2908 "y.tab.c" /* yacc.c:1646  */
    break;

  case 77:
#line 607 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(COUNT,
                                        new_range(REDUCE | COUNT, (yyvsp[-1].rval)), ENULL); }
#line 2915 "y.tab.c" /* yacc.c:1646  */
    break;

  case 78:
#line 610 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(COUNT,
                                        new_range(REDUCE | COUNT, (yyvsp[-3].rval)), (yyvsp[-1].enode)); }
#line 2922 "y.tab.c" /* yacc.c:1646  */
    break;

  case 79:
#line 613 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(MAX,
                                        new_range(REDUCE | MAX, (yyvsp[-1].rval)), ENULL); }
#line 2929 "y.tab.c" /* yacc.c:1646  */
    break;

  case 80:
#line 616 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(MAX,
                                        new_range(REDUCE | MAX, (yyvsp[-3].rval)), (yyvsp[-1].enode)); }
#line 2936 "y.tab.c" /* yacc.c:1646  */
    break;

  case 81:
#line 619 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(LMAX, (yyvsp[-1].enode), (yyvsp[-3].enode)); }
#line 2942 "y.tab.c" /* yacc.c:1646  */
    break;

  case 82:
#line 621 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(MIN,
                                        new_range(REDUCE | MIN, (yyvsp[-1].rval)), ENULL); }
#line 2949 "y.tab.c" /* yacc.c:1646  */
    break;

  case 83:
#line 624 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(MIN,
                                        new_range(REDUCE | MIN, (yyvsp[-3].rval)), (yyvsp[-1].enode)); }
#line 2956 "y.tab.c" /* yacc.c:1646  */
    break;

  case 84:
#line 627 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(LMIN, (yyvsp[-1].enode), (yyvsp[-3].enode)); }
#line 2962 "y.tab.c" /* yacc.c:1646  */
    break;

  case 85:
#line 629 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new_range(REDUCE | 'R', (yyvsp[-1].rval)); }
#line 2968 "y.tab.c" /* yacc.c:1646  */
    break;

  case 86:
#line 631 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new_range(REDUCE | 'C', (yyvsp[-1].rval)); }
#line 2974 "y.tab.c" /* yacc.c:1646  */
    break;

  case 87:
#line 632 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(ABS, (yyvsp[-1].enode), ENULL); }
#line 2980 "y.tab.c" /* yacc.c:1646  */
    break;

  case 88:
#line 633 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(ACOS, (yyvsp[-1].enode), ENULL); }
#line 2986 "y.tab.c" /* yacc.c:1646  */
    break;

  case 89:
#line 634 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(ASIN, (yyvsp[-1].enode), ENULL); }
#line 2992 "y.tab.c" /* yacc.c:1646  */
    break;

  case 90:
#line 635 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(ATAN, (yyvsp[-1].enode), ENULL); }
#line 2998 "y.tab.c" /* yacc.c:1646  */
    break;

  case 91:
#line 636 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(ATAN2, (yyvsp[-3].enode), (yyvsp[-1].enode)); }
#line 3004 "y.tab.c" /* yacc.c:1646  */
    break;

  case 92:
#line 637 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(CEIL, (yyvsp[-1].enode), ENULL); }
#line 3010 "y.tab.c" /* yacc.c:1646  */
    break;

  case 93:
#line 638 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(COS, (yyvsp[-1].enode), ENULL); }
#line 3016 "y.tab.c" /* yacc.c:1646  */
    break;

  case 94:
#line 639 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(EXP, (yyvsp[-1].enode), ENULL); }
#line 3022 "y.tab.c" /* yacc.c:1646  */
    break;

  case 95:
#line 640 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(FABS, (yyvsp[-1].enode), ENULL); }
#line 3028 "y.tab.c" /* yacc.c:1646  */
    break;

  case 96:
#line 641 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(FLOOR, (yyvsp[-1].enode), ENULL); }
#line 3034 "y.tab.c" /* yacc.c:1646  */
    break;

  case 97:
#line 642 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(HYPOT, (yyvsp[-3].enode), (yyvsp[-1].enode)); }
#line 3040 "y.tab.c" /* yacc.c:1646  */
    break;

  case 98:
#line 643 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(LOG, (yyvsp[-1].enode), ENULL); }
#line 3046 "y.tab.c" /* yacc.c:1646  */
    break;

  case 99:
#line 644 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(LOG10, (yyvsp[-1].enode), ENULL); }
#line 3052 "y.tab.c" /* yacc.c:1646  */
    break;

  case 100:
#line 645 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(POW, (yyvsp[-3].enode), (yyvsp[-1].enode)); }
#line 3058 "y.tab.c" /* yacc.c:1646  */
    break;

  case 101:
#line 646 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(SIN, (yyvsp[-1].enode), ENULL); }
#line 3064 "y.tab.c" /* yacc.c:1646  */
    break;

  case 102:
#line 647 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(SQRT, (yyvsp[-1].enode), ENULL); }
#line 3070 "y.tab.c" /* yacc.c:1646  */
    break;

  case 103:
#line 648 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(TAN, (yyvsp[-1].enode), ENULL); }
#line 3076 "y.tab.c" /* yacc.c:1646  */
    break;

  case 104:
#line 649 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(DTR, (yyvsp[-1].enode), ENULL); }
#line 3082 "y.tab.c" /* yacc.c:1646  */
    break;

  case 105:
#line 650 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(RTD, (yyvsp[-1].enode), ENULL); }
#line 3088 "y.tab.c" /* yacc.c:1646  */
    break;

  case 106:
#line 651 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(RND, (yyvsp[-1].enode), ENULL); }
#line 3094 "y.tab.c" /* yacc.c:1646  */
    break;

  case 107:
#line 652 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(ROUND, (yyvsp[-3].enode), (yyvsp[-1].enode)); }
#line 3100 "y.tab.c" /* yacc.c:1646  */
    break;

  case 108:
#line 653 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(IF,  (yyvsp[-5].enode),new(',',(yyvsp[-3].enode),(yyvsp[-1].enode))); }
#line 3106 "y.tab.c" /* yacc.c:1646  */
    break;

  case 109:
#line 655 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(PV,  (yyvsp[-5].enode),new(':',(yyvsp[-3].enode),(yyvsp[-1].enode))); }
#line 3112 "y.tab.c" /* yacc.c:1646  */
    break;

  case 110:
#line 656 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(FV,  (yyvsp[-5].enode),new(':',(yyvsp[-3].enode),(yyvsp[-1].enode))); }
#line 3118 "y.tab.c" /* yacc.c:1646  */
    break;

  case 111:
#line 657 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(PMT, (yyvsp[-5].enode),new(':',(yyvsp[-3].enode),(yyvsp[-1].enode))); }
#line 3124 "y.tab.c" /* yacc.c:1646  */
    break;

  case 112:
#line 659 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(HOUR, (yyvsp[-1].enode), ENULL); }
#line 3130 "y.tab.c" /* yacc.c:1646  */
    break;

  case 113:
#line 660 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(MINUTE, (yyvsp[-1].enode), ENULL); }
#line 3136 "y.tab.c" /* yacc.c:1646  */
    break;

  case 114:
#line 661 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(SECOND, (yyvsp[-1].enode), ENULL); }
#line 3142 "y.tab.c" /* yacc.c:1646  */
    break;

  case 115:
#line 662 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(MONTH, (yyvsp[-1].enode), ENULL); }
#line 3148 "y.tab.c" /* yacc.c:1646  */
    break;

  case 116:
#line 663 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(DAY, (yyvsp[-1].enode), ENULL); }
#line 3154 "y.tab.c" /* yacc.c:1646  */
    break;

  case 117:
#line 664 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(YEAR, (yyvsp[-1].enode), ENULL); }
#line 3160 "y.tab.c" /* yacc.c:1646  */
    break;

  case 118:
#line 665 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(NOW, ENULL, ENULL);}
#line 3166 "y.tab.c" /* yacc.c:1646  */
    break;

  case 119:
#line 667 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(DTS, (yyvsp[-5].enode), new(',', (yyvsp[-3].enode), (yyvsp[-1].enode)));}
#line 3172 "y.tab.c" /* yacc.c:1646  */
    break;

  case 120:
#line 668 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(DTS,
                                new_const(O_CONST, (double) (yyvsp[-4].ival)),
                                new(',', new_const(O_CONST, (double) (yyvsp[-2].ival)),
                                new_const(O_CONST, (double) (yyvsp[0].ival))));}
#line 3181 "y.tab.c" /* yacc.c:1646  */
    break;

  case 121:
#line 673 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(TTS, (yyvsp[-5].enode), new(',', (yyvsp[-3].enode), (yyvsp[-1].enode)));}
#line 3187 "y.tab.c" /* yacc.c:1646  */
    break;

  case 122:
#line 674 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(STON, (yyvsp[-1].enode), ENULL); }
#line 3193 "y.tab.c" /* yacc.c:1646  */
    break;

  case 123:
#line 675 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(SLEN, (yyvsp[-1].enode), ENULL); }
#line 3199 "y.tab.c" /* yacc.c:1646  */
    break;

  case 124:
#line 676 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(EQS, (yyvsp[-3].enode), (yyvsp[-1].enode)); }
#line 3205 "y.tab.c" /* yacc.c:1646  */
    break;

  case 125:
#line 677 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(DATE, (yyvsp[-1].enode), ENULL); }
#line 3211 "y.tab.c" /* yacc.c:1646  */
    break;

  case 126:
#line 678 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(DATE, (yyvsp[-3].enode), (yyvsp[-1].enode)); }
#line 3217 "y.tab.c" /* yacc.c:1646  */
    break;

  case 127:
#line 679 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(FMT, (yyvsp[-3].enode), (yyvsp[-1].enode)); }
#line 3223 "y.tab.c" /* yacc.c:1646  */
    break;

  case 128:
#line 680 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(UPPER, (yyvsp[-1].enode), ENULL); }
#line 3229 "y.tab.c" /* yacc.c:1646  */
    break;

  case 129:
#line 681 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(LOWER, (yyvsp[-1].enode), ENULL); }
#line 3235 "y.tab.c" /* yacc.c:1646  */
    break;

  case 130:
#line 682 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(CAPITAL, (yyvsp[-1].enode), ENULL); }
#line 3241 "y.tab.c" /* yacc.c:1646  */
    break;

  case 131:
#line 684 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(INDEX, new_range(REDUCE | INDEX, (yyvsp[-3].rval)), (yyvsp[-1].enode)); }
#line 3247 "y.tab.c" /* yacc.c:1646  */
    break;

  case 132:
#line 686 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(INDEX, new_range(REDUCE | INDEX, (yyvsp[-1].rval)), (yyvsp[-3].enode)); }
#line 3253 "y.tab.c" /* yacc.c:1646  */
    break;

  case 133:
#line 688 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(INDEX, new_range(REDUCE | INDEX, (yyvsp[-5].rval)),
                    new(',', (yyvsp[-3].enode), (yyvsp[-1].enode))); }
#line 3260 "y.tab.c" /* yacc.c:1646  */
    break;

  case 134:
#line 691 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(LOOKUP, new_range(REDUCE | LOOKUP, (yyvsp[-3].rval)), (yyvsp[-1].enode)); }
#line 3266 "y.tab.c" /* yacc.c:1646  */
    break;

  case 135:
#line 693 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(LOOKUP, new_range(REDUCE | LOOKUP, (yyvsp[-1].rval)), (yyvsp[-3].enode)); }
#line 3272 "y.tab.c" /* yacc.c:1646  */
    break;

  case 136:
#line 695 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(HLOOKUP, new_range(REDUCE | HLOOKUP, (yyvsp[-5].rval)),
                    new(',', (yyvsp[-3].enode), (yyvsp[-1].enode))); }
#line 3279 "y.tab.c" /* yacc.c:1646  */
    break;

  case 137:
#line 698 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(HLOOKUP, new_range(REDUCE | HLOOKUP, (yyvsp[-3].rval)),
                    new(',', (yyvsp[-5].enode), (yyvsp[-1].enode))); }
#line 3286 "y.tab.c" /* yacc.c:1646  */
    break;

  case 138:
#line 701 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(VLOOKUP, new_range(REDUCE | VLOOKUP, (yyvsp[-5].rval)),
                    new(',', (yyvsp[-3].enode), (yyvsp[-1].enode))); }
#line 3293 "y.tab.c" /* yacc.c:1646  */
    break;

  case 139:
#line 704 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(VLOOKUP, new_range(REDUCE | VLOOKUP, (yyvsp[-3].rval)),
                    new(',', (yyvsp[-5].enode), (yyvsp[-1].enode))); }
#line 3300 "y.tab.c" /* yacc.c:1646  */
    break;

  case 140:
#line 707 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(STINDEX, new_range(REDUCE | STINDEX, (yyvsp[-3].rval)), (yyvsp[-1].enode)); }
#line 3306 "y.tab.c" /* yacc.c:1646  */
    break;

  case 141:
#line 709 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(STINDEX, new_range(REDUCE | STINDEX, (yyvsp[-1].rval)), (yyvsp[-3].enode)); }
#line 3312 "y.tab.c" /* yacc.c:1646  */
    break;

  case 142:
#line 711 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(STINDEX, new_range(REDUCE | STINDEX, (yyvsp[-5].rval)),
                    new(',', (yyvsp[-3].enode), (yyvsp[-1].enode))); }
#line 3319 "y.tab.c" /* yacc.c:1646  */
    break;

  case 143:
#line 713 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(EXT, (yyvsp[-3].enode), (yyvsp[-1].enode)); }
#line 3325 "y.tab.c" /* yacc.c:1646  */
    break;

  case 144:
#line 714 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(NVAL, (yyvsp[-3].enode), (yyvsp[-1].enode)); }
#line 3331 "y.tab.c" /* yacc.c:1646  */
    break;

  case 145:
#line 715 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(SVAL, (yyvsp[-3].enode), (yyvsp[-1].enode)); }
#line 3337 "y.tab.c" /* yacc.c:1646  */
    break;

  case 146:
#line 717 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(SUBSTR, (yyvsp[-5].enode), new(',', (yyvsp[-3].enode), (yyvsp[-1].enode))); }
#line 3343 "y.tab.c" /* yacc.c:1646  */
    break;

  case 147:
#line 718 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = (yyvsp[-1].enode); }
#line 3349 "y.tab.c" /* yacc.c:1646  */
    break;

  case 148:
#line 719 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = (yyvsp[0].enode); }
#line 3355 "y.tab.c" /* yacc.c:1646  */
    break;

  case 149:
#line 720 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new('m', (yyvsp[0].enode), ENULL); }
#line 3361 "y.tab.c" /* yacc.c:1646  */
    break;

  case 150:
#line 721 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new_const(O_CONST, (double) (yyvsp[0].ival)); }
#line 3367 "y.tab.c" /* yacc.c:1646  */
    break;

  case 151:
#line 722 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new_const(O_CONST, (yyvsp[0].fval)); }
#line 3373 "y.tab.c" /* yacc.c:1646  */
    break;

  case 152:
#line 723 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(PI_, ENULL, ENULL); }
#line 3379 "y.tab.c" /* yacc.c:1646  */
    break;

  case 153:
#line 724 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new_str((yyvsp[0].sval)); }
#line 3385 "y.tab.c" /* yacc.c:1646  */
    break;

  case 154:
#line 725 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new('!', (yyvsp[0].enode), ENULL); }
#line 3391 "y.tab.c" /* yacc.c:1646  */
    break;

  case 155:
#line 726 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new('!', (yyvsp[0].enode), ENULL); }
#line 3397 "y.tab.c" /* yacc.c:1646  */
    break;

  case 156:
#line 727 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(FILENAME, (yyvsp[-1].enode), ENULL); }
#line 3403 "y.tab.c" /* yacc.c:1646  */
    break;

  case 157:
#line 728 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(MYROW, ENULL, ENULL);}
#line 3409 "y.tab.c" /* yacc.c:1646  */
    break;

  case 158:
#line 729 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(MYCOL, ENULL, ENULL);}
#line 3415 "y.tab.c" /* yacc.c:1646  */
    break;

  case 159:
#line 730 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(LASTROW, ENULL, ENULL);}
#line 3421 "y.tab.c" /* yacc.c:1646  */
    break;

  case 160:
#line 731 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(LASTCOL, ENULL, ENULL);}
#line 3427 "y.tab.c" /* yacc.c:1646  */
    break;

  case 161:
#line 732 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(COLTOA, (yyvsp[-1].enode), ENULL);}
#line 3433 "y.tab.c" /* yacc.c:1646  */
    break;

  case 162:
#line 733 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(ASCII, (yyvsp[-1].enode), ENULL); }
#line 3439 "y.tab.c" /* yacc.c:1646  */
    break;

  case 163:
#line 734 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(SET8BIT, (yyvsp[-1].enode), ENULL); }
#line 3445 "y.tab.c" /* yacc.c:1646  */
    break;

  case 164:
#line 735 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(CHR, (yyvsp[-1].enode), ENULL);}
#line 3451 "y.tab.c" /* yacc.c:1646  */
    break;

  case 165:
#line 736 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(NUMITER, ENULL, ENULL);}
#line 3457 "y.tab.c" /* yacc.c:1646  */
    break;

  case 166:
#line 737 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(ERR_, ENULL, ENULL); }
#line 3463 "y.tab.c" /* yacc.c:1646  */
    break;

  case 167:
#line 738 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(ERR_, ENULL, ENULL); }
#line 3469 "y.tab.c" /* yacc.c:1646  */
    break;

  case 168:
#line 752 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new('+', (yyvsp[-2].enode), (yyvsp[0].enode)); }
#line 3475 "y.tab.c" /* yacc.c:1646  */
    break;

  case 169:
#line 753 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new('-', (yyvsp[-2].enode), (yyvsp[0].enode)); }
#line 3481 "y.tab.c" /* yacc.c:1646  */
    break;

  case 170:
#line 754 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new('*', (yyvsp[-2].enode), (yyvsp[0].enode)); }
#line 3487 "y.tab.c" /* yacc.c:1646  */
    break;

  case 171:
#line 755 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new('/', (yyvsp[-2].enode), (yyvsp[0].enode)); }
#line 3493 "y.tab.c" /* yacc.c:1646  */
    break;

  case 172:
#line 756 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new('%', (yyvsp[-2].enode), (yyvsp[0].enode)); }
#line 3499 "y.tab.c" /* yacc.c:1646  */
    break;

  case 173:
#line 757 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new('^', (yyvsp[-2].enode), (yyvsp[0].enode)); }
#line 3505 "y.tab.c" /* yacc.c:1646  */
    break;

  case 175:
#line 759 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new('?', (yyvsp[-4].enode), new(':', (yyvsp[-2].enode), (yyvsp[0].enode))); }
#line 3511 "y.tab.c" /* yacc.c:1646  */
    break;

  case 176:
#line 760 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(';', (yyvsp[-2].enode), (yyvsp[0].enode)); }
#line 3517 "y.tab.c" /* yacc.c:1646  */
    break;

  case 177:
#line 761 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new('<', (yyvsp[-2].enode), (yyvsp[0].enode)); }
#line 3523 "y.tab.c" /* yacc.c:1646  */
    break;

  case 178:
#line 762 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new('=', (yyvsp[-2].enode), (yyvsp[0].enode)); }
#line 3529 "y.tab.c" /* yacc.c:1646  */
    break;

  case 179:
#line 763 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new('>', (yyvsp[-2].enode), (yyvsp[0].enode)); }
#line 3535 "y.tab.c" /* yacc.c:1646  */
    break;

  case 180:
#line 764 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new('&', (yyvsp[-2].enode), (yyvsp[0].enode)); }
#line 3541 "y.tab.c" /* yacc.c:1646  */
    break;

  case 181:
#line 765 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new('|', (yyvsp[-2].enode), (yyvsp[0].enode)); }
#line 3547 "y.tab.c" /* yacc.c:1646  */
    break;

  case 182:
#line 766 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new('!', new('>', (yyvsp[-3].enode), (yyvsp[0].enode)), ENULL); }
#line 3553 "y.tab.c" /* yacc.c:1646  */
    break;

  case 183:
#line 767 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new('!', new('=', (yyvsp[-3].enode), (yyvsp[0].enode)), ENULL); }
#line 3559 "y.tab.c" /* yacc.c:1646  */
    break;

  case 184:
#line 768 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new('!', new('=', (yyvsp[-3].enode), (yyvsp[0].enode)), ENULL); }
#line 3565 "y.tab.c" /* yacc.c:1646  */
    break;

  case 185:
#line 769 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new('!', new('<', (yyvsp[-3].enode), (yyvsp[0].enode)), ENULL); }
#line 3571 "y.tab.c" /* yacc.c:1646  */
    break;

  case 186:
#line 770 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new('#', (yyvsp[-2].enode), (yyvsp[0].enode)); }
#line 3577 "y.tab.c" /* yacc.c:1646  */
    break;

  case 187:
#line 773 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(ELIST, ENULL, (yyvsp[0].enode)); }
#line 3583 "y.tab.c" /* yacc.c:1646  */
    break;

  case 188:
#line 774 "gram.y" /* yacc.c:1646  */
    { (yyval.enode) = new(ELIST, (yyvsp[-2].enode), (yyvsp[0].enode)); }
#line 3589 "y.tab.c" /* yacc.c:1646  */
    break;

  case 189:
#line 777 "gram.y" /* yacc.c:1646  */
    { (yyval.rval).left = (yyvsp[-2].ent); (yyval.rval).right = (yyvsp[0].ent); }
#line 3595 "y.tab.c" /* yacc.c:1646  */
    break;

  case 190:
#line 778 "gram.y" /* yacc.c:1646  */
    { (yyval.rval) = (yyvsp[0].rval); }
#line 3601 "y.tab.c" /* yacc.c:1646  */
    break;

  case 191:
#line 781 "gram.y" /* yacc.c:1646  */
    { (yyval.ent).vp = lookat((yyvsp[0].ival), (yyvsp[-1].ival));
                          (yyval.ent).vf = 0;
                        }
#line 3609 "y.tab.c" /* yacc.c:1646  */
    break;

  case 192:
#line 784 "gram.y" /* yacc.c:1646  */
    { (yyval.ent).vp = lookat((yyvsp[0].ival), (yyvsp[-1].ival));
                            (yyval.ent).vf = FIX_COL;
                        }
#line 3617 "y.tab.c" /* yacc.c:1646  */
    break;

  case 193:
#line 787 "gram.y" /* yacc.c:1646  */
    { (yyval.ent).vp = lookat((yyvsp[0].ival), (yyvsp[-2].ival));
                            (yyval.ent).vf = FIX_ROW;
                        }
#line 3625 "y.tab.c" /* yacc.c:1646  */
    break;

  case 194:
#line 790 "gram.y" /* yacc.c:1646  */
    {
                          (yyval.ent).vp = lookat((yyvsp[0].ival), (yyvsp[-2].ival));
                          (yyval.ent).vf = FIX_ROW | FIX_COL;
                        }
#line 3634 "y.tab.c" /* yacc.c:1646  */
    break;

  case 195:
#line 794 "gram.y" /* yacc.c:1646  */
    {
                          (yyval.ent) = (yyvsp[0].rval).left;
                        }
#line 3642 "y.tab.c" /* yacc.c:1646  */
    break;

  case 196:
#line 799 "gram.y" /* yacc.c:1646  */
    { (yyval.rval) = (yyvsp[0].rval); }
#line 3648 "y.tab.c" /* yacc.c:1646  */
    break;

  case 197:
#line 800 "gram.y" /* yacc.c:1646  */
    { (yyval.rval).left = (yyvsp[0].ent); (yyval.rval).right = (yyvsp[0].ent); }
#line 3654 "y.tab.c" /* yacc.c:1646  */
    break;

  case 198:
#line 803 "gram.y" /* yacc.c:1646  */
    { (yyval.fval) = (double) (yyvsp[0].ival); }
#line 3660 "y.tab.c" /* yacc.c:1646  */
    break;

  case 199:
#line 804 "gram.y" /* yacc.c:1646  */
    { (yyval.fval) = (yyvsp[0].fval); }
#line 3666 "y.tab.c" /* yacc.c:1646  */
    break;

  case 200:
#line 805 "gram.y" /* yacc.c:1646  */
    { (yyval.fval) = -(yyvsp[0].fval); }
#line 3672 "y.tab.c" /* yacc.c:1646  */
    break;

  case 201:
#line 806 "gram.y" /* yacc.c:1646  */
    { (yyval.fval) = (yyvsp[0].fval); }
#line 3678 "y.tab.c" /* yacc.c:1646  */
    break;

  case 202:
#line 808 "gram.y" /* yacc.c:1646  */
    { (yyval.sval) = (yyvsp[0].sval); }
#line 3684 "y.tab.c" /* yacc.c:1646  */
    break;

  case 203:
#line 809 "gram.y" /* yacc.c:1646  */
    {
                            char *s, *s1;
                            s1 = (yyvsp[0].ent).vp->label;
                            if (!s1)
                            s1 = "NULL_STRING";
                            s = scxmalloc((unsigned)strlen(s1)+1);
                            (void) strcpy(s, s1);
                            (yyval.sval) = s;
                          }
#line 3698 "y.tab.c" /* yacc.c:1646  */
    break;


#line 3702 "y.tab.c" /* yacc.c:1646  */
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYTERROR;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  return yyresult;
}
