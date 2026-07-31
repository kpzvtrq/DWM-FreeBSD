/* See LICENSE file for copyright and license details. */

/* appearance */
static const unsigned int borderpx = 3;        /* border pixel of windows */
static const unsigned int snap     = 32;       /* snap pixel */

// static const unsigned int gappih   = 8;       /* horiz inner gap between windows */
// static const unsigned int gappiv   = 8;       /* vert inner gap between windows */
// static const unsigned int gappoh   = 8;       /* horiz outer gap between windows and screen edge */
// static const unsigned int gappov   = 8;       /* vert outer gap between windows and screen edge */

enum { GAP = 8 };
static const unsigned int gappih = GAP, gappiv = GAP, gappoh = GAP, gappov = GAP;

static       int smartgaps         = 1;        /* 1 means no outer gap when there is only one window */
static const int showbar           = 1;        /* 0 means no bar */
static const int topbar            = 0;        /* 0 means bottom bar */

#define FONT "JetBrainsMono Nerd Font:style=SemiBold:pixelsize=15:antialias=true:autohint=true"
static const char *fonts[] = { FONT };

static const char col_gray1[]       	= "#1d2021";   // Bar background color
static const char col_gray2[]       	= "#282828";   // Inactive window border
static const char col_gray3[]       	= "#a89984";
static const char col_gray4[]       	= "#ddc7a1";
static const char col_gray_light[]  	= "#26292a";   // Background of the active bar element
static const char col_active_border[] = "#504945"; // Active window border

static const char *colors[][3] = {
    /*               fg         bg               border */
    [SchemeNorm] = { col_gray3, col_gray1,       col_gray2 },
    [SchemeSel]  = { col_gray4, col_gray_light,  col_active_border }, };

/* tagging */
static const char *tags[] = { "1", "2", "3", "4", "5", "6" };

static const Rule rules[] = {
	/* class      	instance    title       tags mask     isfloating   monitor */
	{ "chrome",  NULL,       NULL,       1 << 0,       0,           -1 }, };

/* layout(s) */
static const float mfact     		= 0.55; /* factor of master area size [0.05..0.95] */
static const int nmaster     		= 1;    /* number of clients in master area */
static const int resizehints 		= 1;    /* 1 means respect size hints in tiled resizals */
static const int lockfullscreen = 1; /* 1 will force focus on the fullscreen window */
static const int refreshrate 		= 120;  /* refresh rate (per second) for client move/resize */

#define FORCE_VSPLIT 1  /* nrowgrid layout: force two clients to always split vertically */

static const Layout layouts[] = {
	/* symbol		arrange function */
	{ "[]=", 		tile },    /* first entry is default */
	{ "[M]", 		monocle },
	{ "[@]", 		spiral },
	{ "[\\]",		dwindle },
	{ "H[]", 		deck },
	{ "TTT", 		bstack },
	{ "===", 		bstackhoriz },
	{ "HHH", 		grid },
	{ "###", 		nrowgrid },
	{ "---", 		horizgrid },
	{ ":::", 		gaplessgrid },
	{ "|M|", 		centeredmaster },
	{ ">M>", 		centeredfloatingmaster },
	{ "><>", 		NULL },    /* no layout function means floating behavior */
	{ NULL,  		NULL }, };

/* key definitions */
#define MODKEY Mod4Mask
#define TAGKEYS(KEY, TAG) \
	{ MODKEY,           KEY,	 view, {.ui = 1 << TAG} }, \
	{ MODKEY|ShiftMask, KEY,	 tag,  {.ui = 1 << TAG} },

/* helper for spawning shell commands in the pre bwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

/* commands */
static const char *dmenucmd[] = {
	"dmenu_run",
	"-b",
	"-m", "0",
	"-fn", FONT,
	"-nb", col_gray1,
	"-nf", col_gray3,
	"-sb", col_gray_light,
	"-sf", col_gray4,
	"-i",
	NULL
};

static const Key keys[] = {
	/* modifier         key       	 function        argument */
	{ MODKEY,           XK_b,     	 		togglebar,      {0} },
	{ MODKEY,           XK_n,     	 		focusstack,     {.i = +1 } },
	{ MODKEY,           XK_e,     	 		focusstack,     {.i = -1 } },
	{ MODKEY,           XK_i,     	 		incnmaster,     {.i = +1 } },
	{ MODKEY,           XK_d,     	 		incnmaster,     {.i = -1 } },
	{ MODKEY,           XK_y,     	 		setmfact,       {.f = -0.05} },
	{ MODKEY,           XK_semicolon,		setmfact,       {.f = +0.05} },
	{ MODKEY|ShiftMask, XK_y,     	 		setcfact,       {.f = +0.25} },
	{ MODKEY|ShiftMask, XK_semicolon,		setcfact,       {.f = -0.25} },
	{ MODKEY|ShiftMask, XK_o,     	 		setcfact,       {.f =  0.00} },
	{ MODKEY,           XK_Return,	 		zoom,           {0} },
	{ MODKEY|Mod4Mask,  XK_0,     	 		togglegaps,     {0} },
	{ MODKEY,           XK_Tab,   	 		view,           {0} },
	{ MODKEY,      			XK_q,     	 		killclient,     {0} },
	{ MODKEY,           XK_t,     	 		setlayout,      {.v = &layouts[0]} },
	{ MODKEY,           XK_f,     	 		setlayout,      {.v = &layouts[1]} },
	{ MODKEY,           XK_m,     	 		setlayout,      {.v = &layouts[2]} },
	{ MODKEY,           XK_space, 	 		setlayout,      {0} },
	{ MODKEY|ShiftMask, XK_f, 	 		togglefloating, {0} },
	{ MODKEY,           XK_0,     	 		view,           {.ui = ~0 } },
	{ MODKEY|ShiftMask, XK_0,     	 		tag,            {.ui = ~0 } },
	{ MODKEY,           XK_comma, 	 		focusmon,       {.i = -1 } },
	{ MODKEY,           XK_period,	 		focusmon,       {.i = +1 } },
	{ MODKEY|ShiftMask, XK_comma, 	 		tagmon,         {.i = -1 } },
	{ MODKEY|ShiftMask, XK_period,	 		tagmon,         {.i = +1 } },
	{ MODKEY|ShiftMask, XK_q,     	 		quit,           {0} },

/* Brightness (BackLight) */
	{ 0, XK_F5, 	spawn, SHCMD("backlight decr 10") },
	{ 0, XK_F6, 	spawn, SHCMD("backlight incr 10") },

/* Volume (OSS - Mixer) */
	{ 0, XK_F2, 	spawn, SHCMD("mixer vol=-0.05") },
	{ 0, XK_F3, 	spawn, SHCMD("mixer vol=+0.05") },

	{ MODKEY, XK_space,		spawn, {.v = dmenucmd } },
	{ MODKEY, XK_F12,			spawn, SHCMD("flameshot gui --path ~/img/screenshots") },
	{ MODKEY, XK_w, 			spawn, SHCMD("kitty") },
	{ MODKEY, XK_l, 			spawn, SHCMD("chrome") },

	{ MODKEY,	XK_1, 	viewprev, {0} },
	{ MODKEY,	XK_2, 	viewnext, {0} },

	{ MODKEY|ShiftMask,	XK_1, 	tag, {.ui = 1 << 0} },
	{ MODKEY|ShiftMask,	XK_2, 	tag, {.ui = 1 << 1} },

	TAGKEYS( XK_3, 2 )
	TAGKEYS( XK_4, 3 )
	TAGKEYS( XK_5, 4 )
	TAGKEYS( XK_6, 5 ) };

static const Button buttons[] = {
	/* click    	  event mask    button    function       argument */
	{ ClkClientWin, 	MODKEY, 	Button1,	 movemouse,     	 {0} },
	{ ClkClientWin, 	MODKEY, 	Button2,	 togglefloating,	 {0} },
	{ ClkClientWin, 	MODKEY, 	Button3,	 resizemouse,   	 {0} }, };
