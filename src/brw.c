/* See LICENSE file for copyright and license details. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>

typedef struct {
	Cursor cursor;
} Cur;

typedef struct Fnt {
	Display *dpy;
	unsigned int h;
	XftFont *xfont;
	FcPattern *pattern;
	struct Fnt *next;
} Fnt;

enum { ColFg, ColBg, ColBorder }; /* Clr scheme index */
typedef XftColor Clr;

typedef struct {
	unsigned int w, h;
	Display *dpy;
	int screen;
	Window root;
	Drawable drawable;
	GC gc;
	Clr *scheme;
	Fnt *fonts;
} Drw;

/* Drawable abstraction */
Drw *brw_create(Display *dpy, int screen, Window win, unsigned int w, unsigned int h);
void brw_resize(Drw *brw, unsigned int w, unsigned int h);
void brw_free(Drw *brw);

/* Fnt abstraction */
Fnt *brw_fontset_create(Drw* brw, const char *fonts[], size_t fontcount);
void brw_fontset_free(Fnt* set);
unsigned int brw_fontset_getwidth(Drw *brw, const char *text);
unsigned int brw_fontset_getwidth_clamp(Drw *brw, const char *text, unsigned int n);
void brw_font_getexts(Fnt *font, const char *text, unsigned int len, unsigned int *w, unsigned int *h);

/* Colorscheme abstraction */
void brw_clr_create(Drw *brw, Clr *dest, const char *clrname);
void brw_clr_free(Drw *brw, Clr *c);
Clr *brw_scm_create(Drw *brw, const char *clrnames[], size_t clrcount);
void brw_scm_free(Drw *brw, Clr *scm, size_t clrcount);

/* Cursor abstraction */
Cur *brw_cur_create(Drw *brw, int shape);
void brw_cur_free(Drw *brw, Cur *cursor);

/* Drawing context manipulation */
void brw_setfontset(Drw *brw, Fnt *set);
void brw_setscheme(Drw *brw, Clr *scm);

/* Drawing functions */
void brw_rect(Drw *brw, int x, int y, unsigned int w, unsigned int h, int filled, int invert);
int brw_text(Drw *brw, int x, int y, unsigned int w, unsigned int h, unsigned int lpad, const char *text, int invert);

/* Map functions */
void brw_map(Drw *brw, Window win, int x, int y, unsigned int w, unsigned int h);

#define MAX(A, B)               ((A) > (B) ? (A) : (B))
#define MIN(A, B)               ((A) < (B) ? (A) : (B))
#define BETWEEN(X, A, B)        ((A) <= (X) && (X) <= (B))
#define LENGTH(X)               (sizeof (X) / sizeof (X)[0])

void die(const char *fmt, ...);
void *ecalloc(size_t nmemb, size_t size);

#define UTF_INVALID 0xFFFD

static int
utf8decode(const char *s_in, long *u, int *err)
{
	static const unsigned char lens[] = {
		/* 0XXXX */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		/* 10XXX */ 0, 0, 0, 0, 0, 0, 0, 0,  /* invalid */
		/* 110XX */ 2, 2, 2, 2,
		/* 1110X */ 3, 3,
		/* 11110 */ 4,
		/* 11111 */ 0,  /* invalid */
	};
	static const unsigned char leading_mask[] = { 0x7F, 0x1F, 0x0F, 0x07 };
	static const unsigned int overlong[] = { 0x0, 0x80, 0x0800, 0x10000 };

	const unsigned char *s = (const unsigned char *)s_in;
	int len = lens[*s >> 3];
	*u = UTF_INVALID;
	*err = 1;
	if (len == 0)
		return 1;

	long cp = s[0] & leading_mask[len - 1];
	for (int i = 1; i < len; ++i) {
		if (s[i] == '\0' || (s[i] & 0xC0) != 0x80)
			return i;
		cp = (cp << 6) | (s[i] & 0x3F);
	}
	/* out of range, surrogate, overlong encoding */
	if (cp > 0x10FFFF || (cp >> 11) == 0x1B || cp < overlong[len - 1])
		return len;

	*err = 0;
	*u = cp;
	return len;
}

Drw *
brw_create(Display *dpy, int screen, Window root, unsigned int w, unsigned int h)
{
	Drw *brw = ecalloc(1, sizeof(Drw));

	brw->dpy = dpy;
	brw->screen = screen;
	brw->root = root;
	brw->w = w;
	brw->h = h;
	brw->drawable = XCreatePixmap(dpy, root, w, h, DefaultDepth(dpy, screen));
	brw->gc = XCreateGC(dpy, root, 0, NULL);
	XSetLineAttributes(dpy, brw->gc, 1, LineSolid, CapButt, JoinMiter);

	return brw;
}

void
brw_resize(Drw *brw, unsigned int w, unsigned int h)
{
	if (!brw)
		return;

	brw->w = w;
	brw->h = h;
	if (brw->drawable)
		XFreePixmap(brw->dpy, brw->drawable);
	brw->drawable = XCreatePixmap(brw->dpy, brw->root, w, h, DefaultDepth(brw->dpy, brw->screen));
}

void
brw_free(Drw *brw)
{
	XFreePixmap(brw->dpy, brw->drawable);
	XFreeGC(brw->dpy, brw->gc);
	brw_fontset_free(brw->fonts);
	free(brw);
}

/* This function is an implementation detail. Library users should use
 * brw_fontset_create instead.
 */
static Fnt *
xfont_create(Drw *brw, const char *fontname, FcPattern *fontpattern)
{
	Fnt *font;
	XftFont *xfont = NULL;
	FcPattern *pattern = NULL;

	if (fontname) {
		/* Using the pattern found at font->xfont->pattern does not yield the
		 * same substitution results as using the pattern returned by
		 * FcNameParse; using the latter results in the desired fallback
		 * behaviour whereas the former just results in missing-character
		 * rectangles being drawn, at least with some fonts. */
		if (!(xfont = XftFontOpenName(brw->dpy, brw->screen, fontname))) {
			fprintf(stderr, "error, cannot load font from name: '%s'\n", fontname);
			return NULL;
		}
		if (!(pattern = FcNameParse((FcChar8 *) fontname))) {
			fprintf(stderr, "error, cannot parse font name to pattern: '%s'\n", fontname);
			XftFontClose(brw->dpy, xfont);
			return NULL;
		}
	} else if (fontpattern) {
		if (!(xfont = XftFontOpenPattern(brw->dpy, fontpattern))) {
			fprintf(stderr, "error, cannot load font from pattern.\n");
			return NULL;
		}
	} else {
		die("no font specified.");
	}

	font = ecalloc(1, sizeof(Fnt));
	font->xfont = xfont;
	font->pattern = pattern;
	font->h = xfont->ascent + xfont->descent;
	font->dpy = brw->dpy;

	return font;
}

static void
xfont_free(Fnt *font)
{
	if (!font)
		return;
	if (font->pattern)
		FcPatternDestroy(font->pattern);
	XftFontClose(font->dpy, font->xfont);
	free(font);
}

Fnt*
brw_fontset_create(Drw* brw, const char *fonts[], size_t fontcount)
{
	Fnt *cur, *ret = NULL;
	size_t i;

	if (!brw || !fonts)
		return NULL;

	for (i = 1; i <= fontcount; i++) {
		if ((cur = xfont_create(brw, fonts[fontcount - i], NULL))) {
			cur->next = ret;
			ret = cur;
		}
	}
	return (brw->fonts = ret);
}

void
brw_fontset_free(Fnt *font)
{
	if (font) {
		brw_fontset_free(font->next);
		xfont_free(font);
	}
}

void
brw_clr_create(Drw *brw, Clr *dest, const char *clrname)
{
	if (!brw || !dest || !clrname)
		return;

	if (!XftColorAllocName(brw->dpy, DefaultVisual(brw->dpy, brw->screen),
	                       DefaultColormap(brw->dpy, brw->screen),
	                       clrname, dest))
		die("error, cannot allocate color '%s'", clrname);
}

/* Create color schemes. */
Clr *
brw_scm_create(Drw *brw, const char *clrnames[], size_t clrcount)
{
	size_t i;
	Clr *ret;

	/* need at least two colors for a scheme */
	if (!brw || !clrnames || clrcount < 2 || !(ret = ecalloc(clrcount, sizeof(Clr))))
		return NULL;

	for (i = 0; i < clrcount; i++)
		brw_clr_create(brw, &ret[i], clrnames[i]);
	return ret;
}

void
brw_clr_free(Drw *brw, Clr *c)
{
	if (!brw || !c)
		return;

	/* c is typedef XftColor Clr */
	XftColorFree(brw->dpy, DefaultVisual(brw->dpy, brw->screen),
	             DefaultColormap(brw->dpy, brw->screen), c);
}

void
brw_scm_free(Drw *brw, Clr *scm, size_t clrcount)
{
	size_t i;

	if (!brw || !scm)
		return;

	for (i = 0; i < clrcount; i++)
		brw_clr_free(brw, &scm[i]);
	free(scm);
}

void
brw_setfontset(Drw *brw, Fnt *set)
{
	if (brw)
		brw->fonts = set;
}

void
brw_setscheme(Drw *brw, Clr *scm)
{
	if (brw)
		brw->scheme = scm;
}

void
brw_rect(Drw *brw, int x, int y, unsigned int w, unsigned int h, int filled, int invert)
{
	if (!brw || !brw->scheme)
		return;
	XSetForeground(brw->dpy, brw->gc, invert ? brw->scheme[ColBg].pixel : brw->scheme[ColFg].pixel);
	if (filled)
		XFillRectangle(brw->dpy, brw->drawable, brw->gc, x, y, w, h);
	else
		XDrawRectangle(brw->dpy, brw->drawable, brw->gc, x, y, w - 1, h - 1);
}

int
brw_text(Drw *brw, int x, int y, unsigned int w, unsigned int h, unsigned int lpad, const char *text, int invert)
{
	int ty, ellipsis_x = 0;
	unsigned int tmpw, ew, ellipsis_w = 0, ellipsis_len, hash, h0, h1;
	XftDraw *d = NULL;
	Fnt *usedfont, *curfont, *nextfont;
	int utf8strlen, utf8charlen, utf8err, render = x || y || w || h;
	long utf8codepoint = 0;
	const char *utf8str;
	FcCharSet *fccharset;
	FcPattern *fcpattern;
	FcPattern *match;
	XftResult result;
	int charexists = 0, overflow = 0;
	/* keep track of a couple codepoints for which we have no match. */
	static unsigned int nomatches[128], ellipsis_width, invalid_width;
	static const char invalid[] = "�";

	if (!brw || (render && (!brw->scheme || !w)) || !text || !brw->fonts)
		return 0;

	if (!render) {
		w = invert ? invert : ~invert;
	} else {
		XSetForeground(brw->dpy, brw->gc, brw->scheme[invert ? ColFg : ColBg].pixel);
		XFillRectangle(brw->dpy, brw->drawable, brw->gc, x, y, w, h);
		if (w < lpad)
			return x + w;
		d = XftDrawCreate(brw->dpy, brw->drawable,
		                  DefaultVisual(brw->dpy, brw->screen),
		                  DefaultColormap(brw->dpy, brw->screen));
		x += lpad;
		w -= lpad;
	}

	usedfont = brw->fonts;
	if (!ellipsis_width && render)
		ellipsis_width = brw_fontset_getwidth(brw, "...");
	if (!invalid_width && render)
		invalid_width = brw_fontset_getwidth(brw, invalid);
	while (1) {
		ew = ellipsis_len = utf8err = utf8charlen = utf8strlen = 0;
		utf8str = text;
		nextfont = NULL;
		while (*text) {
			utf8charlen = utf8decode(text, &utf8codepoint, &utf8err);
			for (curfont = brw->fonts; curfont; curfont = curfont->next) {
				charexists = charexists || XftCharExists(brw->dpy, curfont->xfont, utf8codepoint);
				if (charexists) {
					brw_font_getexts(curfont, text, utf8charlen, &tmpw, NULL);
					if (ew + ellipsis_width <= w) {
						/* keep track where the ellipsis still fits */
						ellipsis_x = x + ew;
						ellipsis_w = w - ew;
						ellipsis_len = utf8strlen;
					}

					if (ew + tmpw > w) {
						overflow = 1;
						/* called from brw_fontset_getwidth_clamp():
						 * it wants the width AFTER the overflow
						 */
						if (!render)
							x += tmpw;
						else
							utf8strlen = ellipsis_len;
					} else if (curfont == usedfont) {
						text += utf8charlen;
						utf8strlen += utf8err ? 0 : utf8charlen;
						ew += utf8err ? 0 : tmpw;
					} else {
						nextfont = curfont;
					}
					break;
				}
			}

			if (overflow || !charexists || nextfont || utf8err)
				break;
			else
				charexists = 0;
		}

		if (utf8strlen) {
			if (render) {
				ty = y + (h - usedfont->h) / 2 + usedfont->xfont->ascent;
				XftDrawStringUtf8(d, &brw->scheme[invert ? ColBg : ColFg],
				                  usedfont->xfont, x, ty, (XftChar8 *)utf8str, utf8strlen);
			}
			x += ew;
			w -= ew;
		}
		if (utf8err && (!render || invalid_width < w)) {
			if (render)
				brw_text(brw, x, y, w, h, 0, invalid, invert);
			x += invalid_width;
			w -= invalid_width;
		}
		if (render && overflow)
			brw_text(brw, ellipsis_x, y, ellipsis_w, h, 0, "...", invert);

		if (!*text || overflow) {
			break;
		} else if (nextfont) {
			charexists = 0;
			usedfont = nextfont;
		} else {
			/* Regardless of whether or not a fallback font is found, the
			 * character must be drawn. */
			charexists = 1;

			hash = (unsigned int)utf8codepoint;
			hash = ((hash >> 16) ^ hash) * 0x21F0AAAD;
			hash = ((hash >> 15) ^ hash) * 0xD35A2D97;
			h0 = ((hash >> 15) ^ hash) % LENGTH(nomatches);
			h1 = (hash >> 17) % LENGTH(nomatches);
			/* avoid expensive XftFontMatch call when we know we won't find a match */
			if (nomatches[h0] == utf8codepoint || nomatches[h1] == utf8codepoint)
				goto no_match;

			fccharset = FcCharSetCreate();
			FcCharSetAddChar(fccharset, utf8codepoint);

			if (!brw->fonts->pattern) {
				/* Refer to the comment in xfont_create for more information. */
				die("the first font in the cache must be loaded from a font string.");
			}

			fcpattern = FcPatternDuplicate(brw->fonts->pattern);
			FcPatternAddCharSet(fcpattern, FC_CHARSET, fccharset);
			FcPatternAddBool(fcpattern, FC_SCALABLE, FcTrue);

			FcConfigSubstitute(NULL, fcpattern, FcMatchPattern);
			FcDefaultSubstitute(fcpattern);
			match = XftFontMatch(brw->dpy, brw->screen, fcpattern, &result);

			FcCharSetDestroy(fccharset);
			FcPatternDestroy(fcpattern);

			if (match) {
				usedfont = xfont_create(brw, NULL, match);
				if (usedfont && XftCharExists(brw->dpy, usedfont->xfont, utf8codepoint)) {
					for (curfont = brw->fonts; curfont->next; curfont = curfont->next)
						; /* NOP */
					curfont->next = usedfont;
				} else {
					xfont_free(usedfont);
					nomatches[nomatches[h0] ? h1 : h0] = utf8codepoint;
no_match:
					usedfont = brw->fonts;
				}
			}
		}
	}
	if (d)
		XftDrawDestroy(d);

	return x + (render ? w : 0);
}

void
brw_map(Drw *brw, Window win, int x, int y, unsigned int w, unsigned int h)
{
	if (!brw)
		return;

	XCopyArea(brw->dpy, brw->drawable, win, brw->gc, x, y, w, h, x, y);
	XSync(brw->dpy, False);
}

unsigned int
brw_fontset_getwidth(Drw *brw, const char *text)
{
	if (!brw || !brw->fonts || !text)
		return 0;
	return brw_text(brw, 0, 0, 0, 0, 0, text, 0);
}

unsigned int
brw_fontset_getwidth_clamp(Drw *brw, const char *text, unsigned int n)
{
	unsigned int tmp = 0;
	if (brw && brw->fonts && text && n)
		tmp = brw_text(brw, 0, 0, 0, 0, 0, text, n);
	return MIN(n, tmp);
}

void
brw_font_getexts(Fnt *font, const char *text, unsigned int len, unsigned int *w, unsigned int *h)
{
	XGlyphInfo ext;

	if (!font || !text)
		return;

	XftTextExtentsUtf8(font->dpy, font->xfont, (XftChar8 *)text, len, &ext);
	if (w)
		*w = ext.xOff;
	if (h)
		*h = font->h;
}

Cur *
brw_cur_create(Drw *brw, int shape)
{
	Cur *cur;

	if (!brw || !(cur = ecalloc(1, sizeof(Cur))))
		return NULL;

	cur->cursor = XCreateFontCursor(brw->dpy, shape);

	return cur;
}

void
brw_cur_free(Drw *brw, Cur *cursor)
{
	if (!cursor)
		return;

	XFreeCursor(brw->dpy, cursor->cursor);
	free(cursor);
}
