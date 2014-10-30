/*****************************************************************************
 *
 * Mark Nagel <nagel@ics.uci.edu>
 * 20 July 1989
 *
 * $Revision: 7.16 $
 *
 * bool
 * format(fmt, precision, num, buf, buflen)
 *  char *fmt;
 *  double num;
 *  char buf[];
 *  int buflen;
 *
 * The format function will produce a string representation of a number
 * given a _format_ (described below) and a double value.  The result is
 * written into the passed buffer -- if the resulting string is too
 * long to fit into the passed buffer, the function returns false.
 * Otherwise the function returns true.
 *
 * The fmt parameter contains the format to use to convert the number.
 *
 *  #	Digit placeholder.  If the number has fewer digits on either
 *      side of the decimal point than  there are '#' characters in
 *      the format, the extra '#' characters are ignored.  The number
 *      is rounded to the number of digit placeholders as there are
 *      to the right of the decimal point.  If there are more digits
 *      in the number than there are digit placeholders on the left
 *      side of the decimal point, then those digits are displayed.
 *
 *  0	Digit placeholder.  Same as for '#' except that the number
 *      is padded with zeroes on either side of the decimal point.
 *      The number of zeroes used in padding is determined by the
 *      number of digit placeholders after the '0' for digits on
 *      the left side of the decimal point and by the number of
 *      digit placeholders before the '0' for digits on the right
 *      side of the decimal point.
 *
 *  .	Decimal point.  Determines how many digits are placed on
 *      the right and left sides of the decimal point in the number.
 *      Note that numbers smaller than 1 will begin with a decimal
 *      point if the left side of the decimal point contains only
 *      a '#' digit placeholder.  Use a '0' placeholder to get a
 *      leading zero in decimal formats.
 *
 *  %	Percentage.  For each '%' character in the format, the actual
 *      number gets multiplied by 100 (only for purposes of formatting
 *      -- the original number is left unmodified) and the '%' character
 *      is placed in the same position as it is in the format.
 *
 *  ,	Thousands separator.  The presence of a ',' in the format
 *      (multiple commas are treated as one) will cause the number
 *      to be formatted with a ',' separating each set of three digits
 *      in the integer part of the number with numbering beginning
 *      from the right end of the integer.
 *
 *  &   Precision.  When this character is present in the fractional
 *      part of the number, it is equavalent to a number of 0's equal
 *      to the precision specified in the column format command.  For
 *      example, if the precision is 3, "&" is equivalent to "000".
 *
 *  \	Quote.  This character causes the next character to be
 *      inserted into the formatted string directly with no
 *      special interpretation.
 *
 *  E- E+ e- e+
 *	Scientific format.  Causes the number to formatted in scientific
 *	notation.  The case of the 'E' or 'e' given is preserved.  If
 *      the format uses a '+', then the sign is always given for the
 *	exponent value.  If the format uses a '-', then the sign is
 *	only given when the exponent value is negative.  Note that if
 *	there is no digit placeholder following the '+' or '-', then
 *	that part of the formatted number is left out.  In general,
 *	there should be one or more digit placeholders after the '+'
 *	or '-'.
 *
 *  ;	Format selector.  Use this character to separate the format
 *	into two distinct formats.  The format to the left of the
 *	';' character will be used if the number given is zero or
 *	positive.  The format to the right of the ';' character is
 *      used if the number given is negative.
 *    
 *  Any
 *	Self insert.  Any other character will be inserted directly
 *	into the formatted number with no change made to the actual
 *      number.
 *
 *****************************************************************************/

/*****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include "sc.h"

#define bool	int
#define true	1
#define false	0
#define EOS	'\0'
#define MAXBUF	256

static char *fmt_int(char *val, char *fmt, bool comma, bool negative);
static char *fmt_frac(char *val, char *fmt, int lprecision);
static char *fmt_exp(int val, char *fmt);

static void reverse(register char *buf);

char *colformat[COLFORMATS];

/*****************************************************************************/

bool
format(char *fmt, int lprecision, double val, char *buf, int buflen)
{
    register char *cp;
    char *tmp, *tp;
    bool comma = false, negative = false;
    char *integer = NULL, *decimal = NULL;
    char *exponent = NULL;
    int exp_val = 0;
    int width;
    char prtfmt[32];
    static char		*mantissa = NULL;
    static char		*tmpfmt1 = NULL, *tmpfmt2 = NULL, *exptmp = NULL;
    static unsigned	mantlen = 0, fmtlen = 0;
    char *fraction = NULL;
    int zero_pad = 0;

    if (fmt == NULL)
	return(true);

    if (strlen(fmt) + 1 > fmtlen) {
	fmtlen = strlen(fmt) + 40;
	tmpfmt1 = scxrealloc(tmpfmt1, fmtlen);
	tmpfmt2 = scxrealloc(tmpfmt2, fmtlen);
	exptmp = scxrealloc(exptmp, fmtlen);
    }
    fmt = strcpy(tmpfmt1, fmt);
    if (buflen + 1 > mantlen) {
    	mantlen = buflen + 40;
	mantissa = scxrealloc(mantissa, mantlen);
    }

/*
 * select positive or negative format if necessary
 */
    for (cp = fmt; *cp != ';' && *cp != EOS; cp++) {
	if (*cp == '\\')
	    cp++;
    }
    if (*cp == ';') {
	if (val < 0.0) {
	    val = -val;     /* format should provide sign if desired */
	    fmt = cp + 1;
	} else
	    *cp = EOS;
    }
  
/*
 * extract other information from format and produce a
 * format string stored in tmpfmt2 also scxmalloc()'d above
 */
    tmp = tmpfmt2;
    for (cp = fmt, tp = tmp; *cp != EOS; cp++) {
	switch (*cp) {
	    case '\\':
		*tp++ = *cp++;
		*tp++ = *cp;
		break;

	    case ',':
		comma = true;
		break;

	    case '.':
		if (decimal == NULL)
		    decimal = tp;
		*tp++ = *cp;
		break;
	
	    case '%':
		val *= 100.0;
		*tp++ = *cp;
		break;
	
	    default:
		*tp++ = *cp;
		break;
	}
    }
    *tp = EOS;
    fmt = tmpfmt2;

/* The following line was necessary due to problems with the gcc
 * compiler and val being a negative zero.  Thanks to Mike Novack for
 * the suggestion. - CRM
 */
    val = (val + 1.0) - 1.0;
    if (val < 0.0) {
  	negative = true;
	val = -val;
    }
/*
 * extract the exponent from the format if present
 */
    for (cp = fmt; *cp != EOS; cp++) {
	if (*cp == '\\')
	    cp++;
	else if (*cp == 'e' || *cp == 'E') {
	    if (cp[1] == '+' || cp[1] == '-') {
		exponent = strcpy(exptmp, cp);
		*cp = EOS;
		if (val != 0.0) {
		    while (val < 1.0) {
			val *= 10.0;
			exp_val--;
		    }
		    while (val >= 10.0) {
			val /= 10.0;
			exp_val++;
		    }
		}
		break;
	    }
	}
    }

/*
 * determine maximum decimal places and use sprintf
 * to build initial character form of formatted value.
 */
    width = 0;
    if (decimal) {
	*decimal++ = EOS;
	for (cp = decimal; *cp != EOS; cp++) {
	    switch (*cp) {
		case '\\':
		    cp++;
		    break;

		case '#':
		    width++;
		    break;

		case '0':
		    zero_pad = ++width;
		    break;

		case '&':
		    width += lprecision;
		    zero_pad = width;
		    break;
	    }
	}
	zero_pad = strlen(decimal) - zero_pad;
    }
    (void) sprintf(prtfmt, "%%.%dlf", width);
    (void) sprintf(mantissa, prtfmt, val);
    for (cp = integer = mantissa; *cp != dpoint && *cp != EOS; cp++) {
	if (*integer == '0')
	    integer++;
    }
    if (*cp == dpoint) {
	fraction = cp + 1;
	*cp = EOS;
	cp = fraction + strlen(fraction) - 1;
	for (; zero_pad > 0; zero_pad--, cp--) {
	    if (*cp == '0')
		*cp = EOS;
	    else
		break;
	}
    } else
	fraction = "";

/*
 * format the puppy
 */
    {
    static	char *citmp = NULL, *cftmp = NULL;
    static	unsigned cilen = 0, cflen = 0;
    char *ci, *cf, *ce;
    int len_ci, len_cf, len_ce;
    bool ret = false;
    
    ci = fmt_int(integer, fmt, comma, negative);
    len_ci = strlen(ci);
    if (len_ci >= cilen) {
    	cilen = len_ci + 40;
	citmp = scxrealloc(citmp, cilen);
    }
    ci = strcpy(citmp, ci);

    cf = decimal ? fmt_frac(fraction, decimal, lprecision) : "";
    len_cf = strlen(cf);
    if (len_cf >= cflen) {
    	cflen = len_cf + 40;
	cftmp = scxrealloc(cftmp, cilen);
    }
    cf = strcpy(cftmp, cf);

    ce = (exponent) ? fmt_exp(exp_val, exponent) : "";
    len_ce = strlen(ce);
/*
 * Skip copy assuming sprintf doesn't call our format functions
 *   ce = strcpy(scxmalloc((unsigned)((len_ce = strlen(ce)) + 1)), ce);
 */
    if (len_ci + len_cf + len_ce < buflen) {
	(void) sprintf(buf, "%s%s%s", ci, cf, ce);
	ret = true;
    }

    return (ret);
    }
}

/*****************************************************************************/

static char *
fmt_int(char *val,	/* integer part of the value to be formatted */
    char *fmt,		/* integer part of the format */
    bool comma,		/* true if we should comma-ify the value */
    bool negative)	/* true if the value is actually negative */
{
    int digit, f, v;
    int thousands = 0;
    char *cp;
    static char buf[MAXBUF];
    char *bufptr = buf;

/*
 * locate the leftmost digit placeholder
 */
    for (cp = fmt; *cp != EOS; cp++) {
	if (*cp == '\\')
	    cp++;
	else if (*cp == '#' || *cp == '0')
	    break;
    }
    digit = (*cp == EOS) ? -1 : cp - fmt;

/*
 * format the value
 */
    f = strlen(fmt) - 1;
    v = (digit >= 0) ? strlen(val) - 1 : -1;
    while (f >= 0 || v >= 0) {
	if (f > 0 && fmt[f-1] == '\\') {
	    *bufptr++ = fmt[f--];
	} else if (f >= 0 && (fmt[f] == '#' || fmt[f] == '0')) {
	    if (v >= 0 || fmt[f] == '0') {
		*bufptr++ = v < 0 ? '0' : val[v];
		if (comma && (thousands = (thousands + 1) % 3) == 0 &&
			v > 0 && thsep != '\0')
		    *bufptr++ = thsep;
		v--;
	    }
	} else if (f >= 0) {
	    *bufptr++ = fmt[f];
	}
	if (v >= 0 && f == digit) {
	    continue;
	}
	f--;
    }
    
    if (negative && digit >= 0)
	*bufptr++ = '-';
    *bufptr = EOS;
    reverse(buf);

    return (buf);
}

/*****************************************************************************/

static char *
fmt_frac(char *val,	/* fractional part of the value to be formatted */
    char *fmt,		/* fractional portion of format */
    int lprecision)	/* precision, for interpreting the "&" */
{
    static char buf[MAXBUF];
    register char *bufptr = buf;
    register char *fmtptr = fmt, *valptr = val;

    *bufptr++ = dpoint;
    while (*fmtptr != EOS) {
	if (*fmtptr == '&') {
	    int i;
	    for (i = 0; i < lprecision; i++)
		*bufptr++ = (*valptr != EOS) ? *valptr++ : '0';
	} else if (*fmtptr == '\\')
	    *bufptr++ = *++fmtptr;
	else if (*fmtptr == '#' || *fmtptr == '0') {
	    if (*valptr != EOS || *fmtptr == '0')
		*bufptr++ = (*valptr != EOS) ? *valptr++ : '0';
	} else
	    *bufptr++ = *fmtptr;
	fmtptr++;
    }
    *bufptr = EOS;

    if (buf[1] < '0' || buf[1] > '9')
	return (buf + 1);
    else
	return (buf);
}

/*****************************************************************************/

static char *
fmt_exp(int val,	/* value of the exponent */
    char *fmt)		/* exponent part of the format */
{
    static char buf[MAXBUF];
    register char *bufptr = buf;
    char valbuf[64];
    bool negative = false;
  
    *bufptr++ = *fmt++;
    if (*fmt == '+')
	*bufptr++ = (val < 0) ? '-' : '+';
    else if (val < 0)
	*bufptr++ = '-';
    fmt++;
    *bufptr = EOS;

    if (val < 0) {
	val = -val;
	negative = false;
    }
    (void) sprintf(valbuf, "%d", val);
  
    (void) strcat(buf, fmt_int(valbuf, fmt, false, negative));
    return (buf);
}

/*****************************************************************************/

static void
reverse(register char *buf)
{
    register char *cp = buf + strlen(buf) - 1;
    register char tmp;

    while (buf < cp) {
	tmp = *cp;
	*cp-- = *buf;
	*buf++ = tmp;
    }
}

/*****************************************************************************/
/*  
 * Tom Anderson    <toma@hpsad.hp.com>
 * 10/14/90
 *
 * This routine takes a value and formats it using fixed, scientific,
 * or engineering notation.  The format command 'f' determines which
 * format is used.  The formats are:         example
 *    0:   Fixed point (default)             0.00010
 *    1:   Scientific                        1.00E-04
 *    2:   Engineering                       100.00e-06
 *
 * The format command 'f' now uses three values.  The first two are the
 * width and precision, and the last one is the format value 0, 1, or 2 as
 * described above.  The format value is passed in the variable fmt.
 *
 * This formatted value is written into the passed buffer.  if the
 * resulting string is too long to fit into the passed buffer, the
 * function returns false.  Otherwise the function returns true.
 *
 * When a number is formatted as engineering and is outside of the range,
 * the format reverts to scientific.
 *
 * To preserve compatability with old spreadsheet files, the third value
 * may be missing, and the default will be fixed point (format 0).
 *
 * When an old style sheet is saved, the third value will be stored.
 *
 */

/* defined in sc.h */
#ifndef REFMTFIX
#define REFMTFIX	0
#define REFMTFLT	1
#define REFMTENG	2
#define REFMTDATE	3
#define REFMTLDATE	4
#endif

bool
engformat(int fmt, int width, int lprecision, double val, char *buf, int buflen)
{

    static char *engmult[] = {
	"-18", "-15", "-12", "-09", "-06", "-03",
	"+00",
	"+03", "+06", "+09", "+12", "+15", "+18"
    };
    int engind = 0;
    double engmant, pow(), engabs, engexp;

    if (buflen < width) return (false);
    if (fmt >= 0 && fmt < COLFORMATS && colformat[fmt])
	return (format(colformat[fmt], lprecision, val, buf, buflen));
    if (fmt == REFMTFIX)
	(void) sprintf(buf,"%*.*f", width, lprecision, val);
    if (fmt == REFMTFLT)
	(void) sprintf(buf,"%*.*E", width, lprecision, val);
    if (fmt == REFMTENG) {
	if (val == 0e0) {	/* Hack to get zeroes to line up in engr fmt */
	    (void) sprintf((buf-1),"%*.*f ", width, lprecision, val);
	} else {
	    engabs = (val);
	    if ( engabs <  0e0)       engabs = -engabs;
	    if ((engabs >= 1e-18) && (engabs <  1e-15)) engind=0;
	    if ((engabs >= 1e-15) && (engabs <  1e-12)) engind=1;
	    if ((engabs >= 1e-12) && (engabs <  1e-9 )) engind=2;
	    if ((engabs >= 1e-9)  && (engabs <  1e-6 )) engind=3;
	    if ((engabs >= 1e-6)  && (engabs <  1e-3 )) engind=4;
	    if ((engabs >= 1e-3)  && (engabs <  1    )) engind=5;
	    if ((engabs >= 1)     && (engabs <  1e3  )) engind=6;
	    if ((engabs >= 1e3)   && (engabs <  1e6  )) engind=7;
	    if ((engabs >= 1e6)   && (engabs <  1e9  )) engind=8;
	    if ((engabs >= 1e9)   && (engabs <  1e12 )) engind=9;
	    if ((engabs >= 1e12)  && (engabs <  1e15 )) engind=10;
	    if ((engabs >= 1e15)  && (engabs <  1e18 )) engind=11;
	    if ((engabs >= 1e18)  && (engabs <  1e21 )) engind=12;
	    if ((engabs < 1e-18)  || (engabs >= 1e21 )) {
		/* Revert to floating point */
		(void) sprintf(buf,"%*.*E", width, lprecision, val);
	    } else {
		engexp = (double) (engind-6)*3;
		engmant = val/pow(10.0e0,engexp);
		(void) sprintf(buf,"%*.*fe%s", width-4,
			lprecision, engmant, engmult[engind]);
	    }
	}
    }
    if (fmt == REFMTDATE) {
	int i;
	time_t secs;

	if (buflen < 9) {
	    for (i = 0; i < width; i++) buf[i] = '*';
	    buf[i] = '\0';
	} else {
	    secs = (time_t)val;
	    strftime(buf,buflen,"%e %b %y",localtime(&secs));
	    for (i = 9; i < width; i++) buf[i] = ' ';
	    buf[i] = '\0';
	}
    }
    if (fmt == REFMTLDATE) {
	int i;
	time_t secs;

	if (buflen < 11) {
	    for (i = 0; i < width; i++) buf[i] = '*';
	    buf[i] = '\0';
	} else {
	    secs = (time_t)val;
	    strftime(buf,buflen,"%e %b %Y",localtime(&secs));
	    for (i = 11; i < width; i++) buf[i] = ' ';
	    buf[i] = '\0';
	}
    }
    return (true);
}
