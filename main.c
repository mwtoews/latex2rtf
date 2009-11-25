/* main.c - LaTeX to RTF conversion program

Copyright (C) 1995-2007 The Free Software Foundation

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

This file is available from http://sourceforge.net/projects/latex2rtf/
 
Authors:
	1995	  Fernando Dorner, Andreas Granzer, Freidrich Polzer, Gerhard Trisko
	1995-1997 Ralf Schlatterbeck
	1998-2000 Georg Lehner
	2001-2007 Scott Prahl
*/

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include "main.h"
#include "mygetopt.h"
#include "convert.h"
#include "commands.h"
#include "chars.h"
#include "fonts.h"
#include "stack.h"
#include "direct.h"
#include "ignore.h"
#include "version.h"
#include "funct1.h"
#include "cfg.h"
#include "encodings.h"
#include "utils.h"
#include "parser.h"
#include "lengths.h"
#include "counters.h"
#include "preamble.h"
#include "xrefs.h"
#include "preparse.h"
#include "vertical.h"
#include "fields.h"

FILE *fRtf = NULL;              /* file pointer to RTF file */
char *g_tex_name = NULL;
char *g_rtf_name = NULL;
char *g_aux_name = NULL;
char *g_toc_name = NULL;
char *g_lof_name = NULL;
char *g_lot_name = NULL;
char *g_fff_name = NULL;
char *g_ttt_name = NULL;
char *g_bbl_name = NULL;
char *g_home_dir = NULL;

char *progname;                 /* name of the executable file */
bool SpanishMode = FALSE;       /* support spanishstyle */
bool GermanMode = FALSE;        /* support germanstyle */
bool FrenchMode = FALSE;        /* support frenchstyle */
bool RussianMode = FALSE;       /* support russianstyle */
bool CzechMode = FALSE;         /* support czech */

char g_charset_encoding_name[20] = "cp1252";
int g_fcharset_number = 0;

bool twoside = FALSE;
int g_verbosity_level = WARNING;
bool g_little_endian = FALSE;   /* set properly in main() */
uint16_t g_dots_per_inch = 300;

bool pagenumbering = TRUE;      /* by default use plain style */
int headings = FALSE;

bool g_processing_preamble = TRUE;  /* flag set until \begin{document} */
bool g_processing_figure = FALSE;   /* flag, set for figures and not tables */
bool g_processing_eqnarray = FALSE; /* flag set when in an eqnarry */
int g_processing_arrays = 0;

bool g_show_equation_number = FALSE;
int g_enumerate_depth = 0;
bool g_suppress_equation_number = FALSE;
bool g_aux_file_missing = FALSE;    /* assume that it exists */
bool g_bbl_file_missing = FALSE;    /* assume that it exists */

bool g_document_type = FORMAT_ARTICLE;
int g_document_bibstyle = BIBSTYLE_STANDARD;

int g_safety_braces = 0;
bool g_processing_equation = FALSE;
bool g_RTF_warnings = FALSE;
char *g_config_path = NULL;
char *g_script_dir = NULL;
char *g_tmp_dir = NULL;
char *g_preamble = NULL;
bool g_escape_parens = FALSE;

bool g_equation_display_rtf = TRUE;
bool g_equation_inline_rtf = TRUE;
bool g_equation_inline_bitmap = FALSE;
bool g_equation_display_bitmap = FALSE;
bool g_equation_comment = FALSE;
bool g_equation_raw_latex = FALSE;
bool g_tableofcontents = FALSE;

bool g_tabular_display_rtf = TRUE;
bool g_tabular_display_bitmap = FALSE;
int g_tab_counter = 0;
bool g_processing_table = FALSE;
bool g_processing_tabbing = FALSE;
bool g_processing_tabular = FALSE;

double g_png_equation_scale = 1.00;
double g_png_figure_scale = 1.00;
bool g_latex_figures = FALSE;
bool g_endfloat_figures = FALSE;
bool g_endfloat_tables = FALSE;
bool g_endfloat_markers = TRUE;
int  g_graphics_package = GRAPHICS_NONE;

int indent = 0;
char alignment = JUSTIFIED;     /* default for justified: */

int RecursionLevel = 0;
bool twocolumn = FALSE;
bool titlepage = FALSE;

static void OpenRtfFile(char *filename, FILE ** f);
static void CloseRtf(FILE ** f);
static void ConvertLatexPreamble(void);
static void InitializeLatexLengths(void);

static void SetEndianness(void);
static void ConvertWholeDocument(void);
static void print_usage(void);
static void print_version(void);

extern char *optarg;
extern int optind;

int main(int argc, char **argv)
{
    int c, x;
    char *p;
    char *basename = NULL;
    double xx;

    SetEndianness();
    progname = argv[0];

    InitializeStack();
    InitializeLatexLengths();
	InitializeBibliography();
	
    while ((c = my_getopt(argc, argv, "lhpuvFSVWZ:o:a:b:d:f:i:s:u:C:D:M:P:T:t:")) != EOF) {
        switch (c) {
            case 'a':
                g_aux_name = optarg;
                break;
            case 'b':
                g_bbl_name = optarg;
                break;
            case 'd':
                g_verbosity_level = *optarg - '0';
                if (g_verbosity_level < 0 || g_verbosity_level > 7) {
                    diagnostics(WARNING, "debug level (-d# option) must be 0-7");
                    print_usage();
                }
                break;
            case 'f':
                sscanf(optarg, "%d", &x);
                set_fields_use_EQ(x & 1);
                set_fields_use_REF(x & 2);
                break;
            case 'i':
                setPackageBabel(optarg);
                break;
            case 'l':
                setPackageBabel("latin1");
                break;
            case 'o':
                g_rtf_name = strdup(optarg);
                break;
            case 'p':
                g_escape_parens = TRUE;
                break;
            case 'v':
                print_version();
                return (0);
            case 'C':
                setPackageInputenc(optarg);
                break;
            case 'D':
                sscanf(optarg, "%d", &x);
				g_dots_per_inch = (uint16_t) x;
                if (g_dots_per_inch < 25 || g_dots_per_inch > 600)
                    diagnostics(WARNING, "Dots per inch must be between 25 and 600 dpi\n");
                break;
            case 'F':
                g_latex_figures = TRUE;
                break;
            case 'M':
                sscanf(optarg, "%d", &x);
                diagnostics(3, "Math option = %s x=%d", optarg, x);
                g_equation_display_rtf   = (x &  1) ? TRUE : FALSE;
                g_equation_inline_rtf    = (x &  2) ? TRUE : FALSE;
                g_equation_display_bitmap= (x &  4) ? TRUE : FALSE;
                g_equation_inline_bitmap = (x &  8) ? TRUE : FALSE;
                g_equation_comment       = (x & 16) ? TRUE : FALSE;
                g_equation_raw_latex     = (x & 32) ? TRUE : FALSE;
                diagnostics(3, "Math option g_equation_display_rtf    = %d", g_equation_display_rtf);
                diagnostics(3, "Math option g_equation_inline_rtf     = %d", g_equation_inline_rtf);
                diagnostics(3, "Math option g_equation_display_bitmap = %d", g_equation_display_bitmap);
                diagnostics(3, "Math option g_equation_inline_bitmap  = %d", g_equation_inline_bitmap);
                diagnostics(3, "Math option g_equation_comment        = %d", g_equation_comment);
                diagnostics(3, "Math option g_equation_raw_latex      = %d", g_equation_raw_latex);
                if (!g_equation_comment && !g_equation_inline_rtf && !g_equation_inline_bitmap && !g_equation_raw_latex)
                    g_equation_inline_rtf = TRUE;
                if (!g_equation_comment && !g_equation_display_rtf && !g_equation_display_bitmap && !g_equation_raw_latex)
                    g_equation_display_rtf = TRUE;
                break;

            case 't':
                sscanf(optarg, "%d", &x);
                diagnostics(3, "Table option = %s x=%d", optarg, x);
                g_tabular_display_rtf    = (x &  1) ? TRUE : FALSE;
                g_tabular_display_bitmap = (x &  2) ? TRUE : FALSE;
                diagnostics(3, "Table option g_tabular_display_rtf     = %d", g_tabular_display_rtf);
                diagnostics(3, "Table option g_tabular_display_bitmap  = %d", g_tabular_display_bitmap);
                break;

            case 'P':          /* -P path/to/cfg:path/to/script or -P path/to/cfg or -P :path/to/script */
                p = strchr(optarg, ENVSEP);
                if (p) {
                    *p = '\0';
                    g_script_dir = strdup(p + 1);
                }
                if (p != optarg)
                    g_config_path = strdup(optarg);
                diagnostics(2, "cfg=%s, script=%s", g_config_path, g_script_dir);
                break;

            case 's':
                if (optarg && optarg[0] == 'e') {
                    if (sscanf(optarg, "e%lf", &xx) == 1 && xx > 0) {
                        g_png_equation_scale = xx;
                    } else {
                        diagnostics(WARNING, "Mistake in command line number for scaling equations");
                        diagnostics(WARNING, "Either use no spaces: '-se1.22' or write as '-s e1.22'");
                    }
                    
                } else if (optarg && optarg[0] == 'f') {
                
                    if (sscanf(optarg, "f%lf", &xx) == 1 && xx > 0) {
                        g_png_figure_scale = xx;
                    } else {
                        diagnostics(WARNING, "Mistake in command line number for scaling figures");
                        diagnostics(WARNING, "Either use no spaces: '-sf1.35' or write as '-s f1.35'");
                    }
                } else {
                    diagnostics(WARNING, "Unknown option '-s' use '-se#' or '-sf#'");
                }
                break;

            case 'S':
                g_field_separator = ';';
                break;
            case 'T':
                g_tmp_dir = strdup(optarg);
                break;
            case 'V':
                print_version();
                return (0);
            case 'W':
                g_RTF_warnings = TRUE;
                break;
            case 'Z':
                g_safety_braces = FALSE;
                g_safety_braces = *optarg - '0';
                if (g_safety_braces < 0 || g_safety_braces > 9) {
                    diagnostics(WARNING, "Number of safety braces (-Z#) must be 0-9");
                    print_usage();
                }
                break;

            case 'h':
            case '?':
            default:
                print_usage();
        }
    }

    argc -= optind;
    argv += optind;

    if (argc > 1) {
        diagnostics(WARNING, "Only a single file can be processed at a time");
        diagnostics(ERROR, " Type \"latex2rtf -h\" for help");
    }

/* Parse filename.	Extract directory if possible.	Beware of stdin cases */

    if (argc == 1 && strcmp(*argv, "-") != 0) { /* filename exists and != "-" */
        char *s, *t;

        basename = strdup(*argv);   /* parse filename */
        s = strrchr(basename, PATHSEP);
        if (s != NULL) {
            g_home_dir = strdup(basename);  /* parse /tmp/file.tex */
            t = strdup(s + 1);
            free(basename);
            basename = t;       /* basename = file.tex */
            s = strrchr(g_home_dir, PATHSEP);
            *(s + 1) = '\0';    /* g_home_dir = /tmp/ */
        }

        t = strstr(basename, ".ltx");   /* remove .ltx if present */
        if (t != NULL) {
            *t = '\0';
            g_tex_name = strdup_together(basename, ".ltx");

        } else {

            t = strstr(basename, ".tex");   /* remove .tex if present */
            if (t != NULL)
                *t = '\0';

            g_tex_name = strdup_together(basename, ".tex");
        }

        if (g_rtf_name == NULL) {
            if (g_home_dir)
				g_rtf_name = strdup_together3(g_home_dir,basename,".rtf");
			else
            	g_rtf_name = strdup_together(basename, ".rtf");
        }
    }

    if (g_aux_name == NULL && basename != NULL)
        g_aux_name = strdup_together(basename, ".aux");

    if (g_bbl_name == NULL && basename != NULL)
        g_bbl_name = strdup_together(basename, ".bbl");

    if (g_toc_name == NULL && basename != NULL)
        g_toc_name = strdup_together(basename, ".toc");

    if (g_lof_name == NULL && basename != NULL)
        g_lof_name = strdup_together(basename, ".lof");

    if (g_lot_name == NULL && basename != NULL)
        g_lot_name = strdup_together(basename, ".lot");

    if (g_fff_name == NULL && basename != NULL)
        g_fff_name = strdup_together(basename, ".fff");

    if (g_ttt_name == NULL && basename != NULL)
        g_ttt_name = strdup_together(basename, ".ttt");

    if (basename) {
        diagnostics(2, "latex filename is <%s>", g_tex_name);
        diagnostics(2, "  rtf filename is <%s>", g_rtf_name);
        diagnostics(2, "  aux filename is <%s>", g_aux_name);
        diagnostics(2, "  bbl filename is <%s>", g_bbl_name);
        diagnostics(2, "home directory is <%s>", (g_home_dir) ? g_home_dir : "");
    }

    ReadCfg();

    if (PushSource(g_tex_name, NULL) == 0) {
        OpenRtfFile(g_rtf_name, &fRtf);

        InitializeDocumentFont(TexFontNumber("Roman"), 20, F_SHAPE_UPRIGHT, F_SERIES_MEDIUM);
        PushTrackLineNumber(TRUE);

        ConvertWholeDocument();
        PopSource();
        CloseRtf(&fRtf);
        printf("\n");

		if (0) debug_malloc();

        return 0;
    } else {
        printf("\n");
        return 1;
    }
}

static void SetEndianness(void)

/*
purpose : Figure out endianness of machine.	 Needed for graphics support
*/
{
    unsigned int endian_test = (unsigned int) 0xaabbccdd;
    unsigned char endian_test_char = *(unsigned char *) &endian_test;

    if (endian_test_char == 0xdd)
        g_little_endian = TRUE;
}


static void ConvertWholeDocument(void)
{
    char *body, *sec_head, *sec_head2, *label;
    char t[] = "\\begin{document}";

    PushEnvironment(DOCUMENT_MODE);  /* because we use ConvertString in preamble.c */
    PushEnvironment(PREAMBLE_MODE);
    setTexMode(MODE_VERTICAL);
    ConvertLatexPreamble();
    WriteRtfHeader();
    ConvertString(t);

    g_processing_preamble = FALSE;
    preParse(&body, &sec_head, &label);

    diagnostics(2, "\\begin{document}");
	diagnostics(5,"label for this section is'%s'", label);
	diagnostics(5, "next section '%s'", sec_head);
	show_string(2, body, "body ");
	
    ConvertString(body);
    free(body);
    if (label)
        free(label);

    while (strcmp(sec_head,"\\end{document}")!=0) {
        preParse(&body, &sec_head2, &g_section_label);
        label = ExtractLabelTag(sec_head);
        if (label) {
            if (g_section_label)
                free(g_section_label);
            g_section_label = label;
        }
        
		diagnostics(2, "processing '%s'", sec_head);	
		diagnostics(5, "label is   '%s'", g_section_label);
		diagnostics(5, "next  is   '%s'", sec_head2);	
		show_string(2, body, "body ");

        ConvertString(sec_head);
        ConvertString(body);
        free(body);
        free(sec_head);
        sec_head = sec_head2;
    }
    
    if (g_endfloat_figures && g_fff_name) {
    	g_endfloat_figures = FALSE;
        if (PushSource(g_fff_name, NULL) == 0) {
        	CmdNewPage(NewPage);
        	CmdListOf(LIST_OF_FIGURES);
        	Convert();
        }
     }
    
    if (g_endfloat_tables && g_ttt_name) {
    	g_endfloat_tables = FALSE;
        if (PushSource(g_ttt_name, NULL) == 0) {
        	CmdNewPage(NewPage);
        	CmdListOf(LIST_OF_TABLES);
        	Convert();
        }
     }
     
    if (strcmp(sec_head,"\\end{document}")==0) {
        diagnostics(2, "\\end{document}");
    	ConvertString(sec_head);
	}
}

static void print_version(void)
{
    fprintf(stdout, "latex2rtf %s\n\n", Version);
    fprintf(stdout, "Copyright (C) 2007 Free Software Foundation, Inc.\n");
    fprintf(stdout, "This is free software; see the source for copying conditions.  There is NO\n");
    fprintf(stdout, "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n");
    fprintf(stdout, "Written by Prahl, Lehner, Granzer, Dorner, Polzer, Trisko, Schlatterbeck.\n");

/*		fprintf(stdout, "RTFPATH = '%s'\n", getenv("RTFPATH"));*/
}

static void print_usage(void)
{
    char *s;

    fprintf(stdout, "`%s' converts text files in LaTeX format to rich text format (RTF).\n\n", progname);
    fprintf(stdout, "Usage:  %s [options] input[.tex]\n\n", progname);
    fprintf(stdout, "Options:\n");
    fprintf(stdout, "  -a auxfile       use LaTeX auxfile rather than input.aux\n");
    fprintf(stdout, "  -b bblfile       use BibTex bblfile rather than input.bbl\n");
    fprintf(stdout, "  -C codepage      charset used by the latex document (latin1, cp850, raw, etc.)\n");
    fprintf(stdout, "  -d level         debugging output (level is 0-6)\n");
    fprintf(stdout, "  -f#              field handling\n");
    fprintf(stdout, "       -f0          do not use fields\n");
    fprintf(stdout, "       -f1          use fields for equations but not \\ref{} & \\cite{}\n");
    fprintf(stdout, "       -f2          use fields for \\cite{} & \\ref{}, but not equations\n");
    fprintf(stdout, "       -f3          use fields when possible (default)\n");
    fprintf(stdout, "  -F               use LaTeX to convert all figures to bitmaps\n");
    fprintf(stdout, "  -D dpi           number of dots per inch for bitmaps\n");
    fprintf(stdout, "  -h               display help\n");
    fprintf(stdout, "  -i language      idiom or language (e.g., german, french)\n");
    fprintf(stdout, "  -l               use latin1 encoding (default)\n");
    fprintf(stdout, "  -M#              math equation handling\n");
    fprintf(stdout, "       -M1          displayed equations to RTF\n");
    fprintf(stdout, "       -M2          inline equations to RTF\n");
    fprintf(stdout, "       -M3          inline and displayed equations to RTF (default)\n");
    fprintf(stdout, "       -M4          displayed equations to bitmap\n");
    fprintf(stdout, "       -M6          inline equations to RTF and displayed equations to bitmaps\n");
    fprintf(stdout, "       -M8          inline equations to bitmap\n");
    fprintf(stdout, "       -M12         inline and displayed equations to bitmaps\n");
    fprintf(stdout, "       -M16         insert Word comment field that the original equation text\n");
    fprintf(stdout, "       -M32         insert the raw LaTeX equation delimited by <<: and :>>\n");
    fprintf(stdout, "  -o outputfile    file for RTF output\n");
    fprintf(stdout, "  -p               option to avoid bug in Word for some equations\n");
    fprintf(stdout, "  -P path          paths to *.cfg & latex2png\n");
    fprintf(stdout, "  -S               use ';' to separate args in RTF fields\n");
    fprintf(stdout, "  -se#             scale factor for bitmap equations\n");
    fprintf(stdout, "  -sf#             scale factor for bitmap figures\n");
    fprintf(stdout, "  -t#              table handling\n");
    fprintf(stdout, "       -t1          tabular and tabbing environments as RTF\n");
    fprintf(stdout, "       -t2          tabular and tabbing environments as bitmaps\n");
    fprintf(stdout, "  -T /path/to/tmp  temporary directory (not used in DOS/Win version)\n");
    fprintf(stdout, "  -v               version information\n");
    fprintf(stdout, "  -V               version information\n");
    fprintf(stdout, "  -W               include warnings in RTF\n");
    fprintf(stdout, "  -Z#              add # of '}'s at end of rtf file (# is 0-9)\n\n");
    fprintf(stdout, "Examples:\n");
    fprintf(stdout, "  latex2rtf foo                       convert foo.tex to foo.rtf\n");
    fprintf(stdout, "  latex2rtf <foo >foo.RTF             convert foo to foo.RTF\n");
    fprintf(stdout, "  latex2rtf -P ./cfg/:./scripts/ foo  use alternate cfg and latex2png files\n");
    fprintf(stdout, "  latex2rtf -M12 foo                  replace equations with bitmaps\n");
    fprintf(stdout, "  latex2rtf -t3  foo                  tables as RTF *and* bitmaps\n");
    fprintf(stdout, "  latex2rtf -i russian foo            assume russian tex conventions\n");
    fprintf(stdout, "  latex2rtf -C raw foo                retain font encoding in rtf file\n");
    fprintf(stdout, "  latex2rtf -f0 foo                   create foo.rtf without fields\n");
    fprintf(stdout, "  latex2rtf -d4 foo                   lots of debugging information\n\n");
    fprintf(stdout, "Report bugs to <latex2rtf-developers@lists.sourceforge.net>\n\n");
    fprintf(stdout, "$RTFPATH designates the directory for configuration files (*.cfg)\n");
    s = getenv("RTFPATH");
    fprintf(stdout, "$RTFPATH = '%s'\n\n", (s) ? s : "not defined");
    s = CFGDIR;
    fprintf(stdout, "CFGDIR compiled-in directory for configuration files (*.cfg)\n");
    fprintf(stdout, "CFGDIR  = '%s'\n\n", (s) ? s : "not defined");
    fprintf(stdout, "latex2rtf %s\n", Version);
    exit(1);
}

void diagnostics(int level, char *format, ...)

/****************************************************************************
purpose: Writes the message to stderr depending on debugging level
 ****************************************************************************/
{
    static int first = TRUE;
    
    char buffer[512], *buff_ptr;
    va_list apf;
    int i;

    buff_ptr = buffer;

    va_start(apf, format);

    if (level <= g_verbosity_level) {

        CurrentEnvironmentCount();

        if (!first) fprintf(stderr,"\n");
        
        fprintf(stderr, "%s:%-3d ",CurrentFileName(),CurrentLineNumber());
        switch (level) {
            case 0:
                fprintf(stderr, "Error! ");
                break;
            case 1:
                if (g_RTF_warnings) {
                    vsnprintf(buffer, 512, format, apf);
                    fprintRTF("{\\plain\\cf2 [latex2rtf:");
                    while (*buff_ptr) {
                        putRtfCharEscaped(*buff_ptr);
                        buff_ptr++;
                    }
                    fprintRTF("]}");
                }
                break;
            case 5:
            case 6:
                fprintf(stderr, " rec=%d ", RecursionLevel);
                /*fall through */
            case 2:
            case 3:
            case 4:
                for (i = 0; i < BraceLevel; i++)
                    fprintf(stderr, "{");
                for (i = 8; i > BraceLevel; i--)
                    fprintf(stderr, " ");

                for (i = 0; i < RecursionLevel; i++)
                    fprintf(stderr, "  ");
                break;
            default:
                break;
        }
        vfprintf(stderr, format, apf);
    	first = FALSE;
    }
    va_end(apf);

    if (level == 0) {
        fprintf(stderr, "\n");
        fflush(stderr);
        if (fRtf) 
            fflush(fRtf);
            
        exit(EXIT_FAILURE);
    }
}

static void InitializeLatexLengths(void)
{
    /* Default Page Sizes */
    setLength("pageheight", 795 * 20);
    setLength("hoffset", 0 * 20);
    setLength("oddsidemargin", 62 * 20);
    setLength("headheight", 12 * 20);
    setLength("textheight", 550 * 20);
    setLength("footskip", 30 * 20);
    setLength("marginparpush", 5 * 20);

    setLength("pagewidth", 614 * 20);
    setLength("voffset", 0 * 20);
    setLength("topmargin", 18 * 20);
    setLength("headsep", 25 * 20);
    setLength("textwidth", 345 * 20);
    setLength("columnwidth", 345 * 20);
    setLength("linewidth", 345 * 20);
    setLength("columnsep", 10 * 20);
    setLength("evensidemargin", 11 * 20);

    /* Default Paragraph Sizes */
    setLength("baselineskip", 12 * 20);
    setLength("parindent", 15 * 20);
    setLength("parskip", 0 * 20);

    setCounter("page", 0);
    setCounter("part", 0);
    setCounter("chapter", 0);
    setCounter("section", 0);
    setCounter("subsection", 0);
    setCounter("subsubsection", 0);
    setCounter("paragraph", 0);
    setCounter("subparagraph", 0);
    setCounter("figure", 0);
    setCounter("table", 0);
    setCounter("equation", 0);
    setCounter("footnote", 0);
    setCounter("mpfootnote", 0);
    setCounter("secnumdepth", 2);
    setCounter("endfloatfigure", 0);
    setCounter("endfloattable", 0);

/* vertical separation lengths */
    setLength("topsep", 3 * 20);
    setLength("partopsep", 2 * 20);
    setLength("parsep", (int) (2.5 * 20));
    setLength("itemsep", 0 * 20);
    setLength("labelwidth", 0 * 20);
    setLength("labelsep", 0 * 20);
    setLength("itemindent", 0 * 20);
    setLength("listparindent", 0 * 20);
    setLength("leftmargin", 0 * 20);
    setLength("floatsep", 0 * 20);
    setLength("intextsep", 0 * 20);
    setLength("textfloatsep", 0 * 20);
    setLength("abovedisplayskip", 0 * 20);
    setLength("belowdisplayskip", 0 * 20);
    setLength("abovecaptionskip", 0 * 20);
    setLength("belowcaptionskip", 0 * 20);
    setLength("intextsep", 0 * 20);

    setLength("smallskipamount", 3 * 20);
    setLength("medskipamount", 6 * 20);
    setLength("bigskipamount", 12 * 20);

    setLength("marginparsep", 10 * 20);
}

static void ConvertLatexPreamble(void)

/****************************************************************************
purpose: reads the LaTeX preamble (to \begin{document} ) for the file
 ****************************************************************************/
{
    char t[] = "\\begin|{|document|}";
    FILE *rtf_file;

	/* Here we switch the file pointers ... it is important that nothing
	   get printed to fRtf until the entire preamble has been processed.  
	   This is really hard to track down, so the processed RTF get sent
	   directly to stderr instead.
	*/
	rtf_file = fRtf;
	fRtf = stderr;
	
    g_preamble = getSpacedTexUntil(t, 1);

    diagnostics(2, "Read LaTeX Preamble");
    diagnostics(5, "Entering ConvertString() from ConvertLatexPreamble");

	show_string(5, g_preamble, "preamble");	

    ConvertString(g_preamble);
    diagnostics(5, "Exiting ConvertString() from ConvertLatexPreamble");
    fRtf = rtf_file;
}


void OpenRtfFile(char *filename, FILE ** f)

/****************************************************************************
purpose: creates output file and writes RTF-header.
params: filename - name of outputfile, possibly NULL for already open file
	f - pointer to filepointer to store file ID
 ****************************************************************************/
{
    if (filename == NULL) {
        diagnostics(4, "Writing RTF to stdout");
        *f = stdout;

    } else {

        *f = fopen(filename, "w");

        if (*f == NULL)
            diagnostics(ERROR, "Error opening RTF file <%s>\n", filename);

        diagnostics(2, "Opened RTF file <%s>", filename);
    }
}

void CloseRtf(FILE ** f)

/****************************************************************************
purpose: closes output file.
params: f - pointer to filepointer to invalidate
globals: g_tex_name;
 ****************************************************************************/
{
    int i;

    CmdEndParagraph(0);
    if (BraceLevel > 1) {
        diagnostics(WARNING, "Mismatched '{' in RTF file, Conversion may cause problems.");
        diagnostics(WARNING, "This is often caused by having environments that span ");
        diagnostics(WARNING, "\\section{}s.  For example ");
        diagnostics(WARNING, "   \\begin{small} ... \\section{A} ... \\section{B} ... \\end{small}");
        diagnostics(WARNING, "will definitely fail.");
	}
	
    if (BraceLevel - 1 > g_safety_braces)
        diagnostics(WARNING, "Try translating with 'latex2rtf -Z%d %s'", BraceLevel - 1, g_tex_name);

    fprintf(*f, "}\n");
    for (i = 0; i < g_safety_braces; i++)
        fprintf(*f, "}");
    if (*f != stdout) {
        if (fclose(*f) == EOF) {
            diagnostics(WARNING, "Error closing RTF-File");
        }
    }
    *f = NULL;
    diagnostics(4, "Closed RTF file");
    fprintf(stderr,"\n");
}

void putRtfCharEscaped(char cThis)

/****************************************************************************
purpose: output a single escaped character to the RTF file
         this is primarily useful for the verbatim-like enviroments
 ****************************************************************************/
{
	if (getTexMode() == MODE_VERTICAL)
		changeTexMode(MODE_HORIZONTAL);
	if (cThis == '\\')
        fprintRTF("%s","\\\\");
    else if (cThis == '{')
        fprintRTF("%s", "\\{");
    else if (cThis == '}')
        fprintRTF("%s", "\\}");
    else if (cThis == '\n')
        fprintRTF("%s", "\n\\par ");
    else
        fprintRTF("%c",cThis);
}

/****************************************************************************
purpose: output a string with escaped characters to the RTF file
         this is primarily useful for the verbatim-like enviroments
 ****************************************************************************/
void putRtfStrEscaped(const char * string)
{
	char *s = (char *) string;
	if (string == NULL) return;
    while (*s) putRtfCharEscaped(*s++);
}


void fprintRTF(char *format, ...)

/****************************************************************************
purpose: output a formatted string to the RTF file.  It is assumed that the
         formatted string has been properly escaped for the RTF file.  
         *ALL* output to the RTF file passes through this routine.
 ****************************************************************************/
{
    char buffer[1024];
    unsigned char *text;
    char last='\0';
    
    va_list apf;

    va_start(apf, format);
    vsnprintf(buffer, 1024, format, apf);
    va_end(apf);
    text = (unsigned char *) buffer;

    while (*text) {

		WriteEightBitChar(text[0], fRtf);
	
		if (*text == '{' && last != '\\')
			PushFontSettings();
	
		if (*text == '}' && last != '\\')
			PopFontSettings();
	
		if (*text == '\\' && last != '\\')
			MonitorFontChanges(text);
	
		last= *text;
		text++;
    }
}

char *getTmpPath(void)

/****************************************************************************
purpose: return the directory to store temporary files
 ****************************************************************************/
{
#if defined(MSDOS) || defined(MACINTOSH) || defined(__MWERKS__)

    return strdup("");

#else

    char *t, *u;
    char pathsep_str[2] = { PATHSEP, 0 };   /* for os2 or w32 "unix" compiler */

    /* first use any temporary directory specified as an option */
    if (g_tmp_dir)
        t = strdup(g_tmp_dir);

    /* next try the environment variable TMPDIR */
    else if ((u = getenv("TMPDIR")) != NULL)
        t = strdup(u);

    /* finally just return "/tmp/" */
    else
        t = strdup("/tmp/");

    /* append a final '/' if missing */
    if (*(t + strlen(t) - 1) != PATHSEP) {
        u = strdup_together(t, pathsep_str);
        free(t);
        return u;
    }

    return t;
#endif
}

char *my_strdup(const char *str)

/****************************************************************************
purpose: duplicate string --- exists to ease porting
 ****************************************************************************/
{
    char *s = NULL;
    unsigned long strsize;

    strsize = strlen(str) + 1;
    s = (char *) malloc(strsize);
    *s = '\0';
    if (s == NULL)
        diagnostics(ERROR, "Cannot allocate memory to duplicate string");
    my_strlcpy(s, str, strsize);

/*	diagnostics(5,"ptr %x",(unsigned long)s);*/
    return s;
}

FILE *my_fopen(char *path, char *mode)

/****************************************************************************
purpose: opens "g_home_dir/path"  and 
 ****************************************************************************/
{
    char *name;
    FILE *p;

    if (path == NULL || mode == NULL)
        return (NULL);

    if (g_home_dir == NULL)
        name = strdup(path);
    else
        name = strdup_together(g_home_dir, path);

    p = fopen(name, mode);

    if (p == NULL) {
    	if (strstr(path, ".tex") != NULL)
        	p = (FILE *) open_cfg(path, FALSE);
	} else
    	diagnostics(2, "Opened '%s'", name);
	
    if (p == NULL) {
        diagnostics(WARNING, "Cannot open '%s'", name);
        fflush(NULL);
    }

    free(name);
    return p;
}

void debug_malloc(void)
{
    char c;

    diagnostics(WARNING, "Malloc Debugging --- press return to continue");
    fflush(NULL);
    fscanf(stdin, "%c", &c);
}
