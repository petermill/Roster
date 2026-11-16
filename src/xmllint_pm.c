//*===============================================================================
// * xmllint_pm.c
// *
// *  Created on: 29 juil. 2022
// *      extracted from github https://github.com/GNOME/libxml2/blob/master/xmllint.c
// *      with option --format -o roster.xml
// */
//
// OpenDCC - BiDiB ---> BiDiB_PI1
//
// Copyright (c) 2018 Pierre Moulin
//           (based on some parts from OpenDCC from Wolfgang Kufer)
//
// This source file is subject of the GNU general public license 2,
// that is available at the world-wide-web at
// http://www.gnu.org/licenses/gpl.txt
//
// purpose: Manage the loco roster to be used in WiFi with EngineDriver
//Author: pierre
//
// how: roster is inspired from JMRI roster
//
// history:  2022-07-30 first version
//
//===============================================================================
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/parserInternals.h>
#include <libxml/HTMLparser.h>
#include <libxml/HTMLtree.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/debugXML.h>
#include <libxml/xmlerror.h>

#include <libxml/globals.h>
#include <libxml/xmlreader.h>

#include <libxml/xmlsave.h>


typedef enum {
    XMLLINT_RETURN_OK = 0,	/* No error */
    XMLLINT_ERR_UNCLASS = 1,	/* Unclassified */
    XMLLINT_ERR_DTD = 2,	/* Error in DTD */
    XMLLINT_ERR_VALID = 3,	/* Validation error */
    XMLLINT_ERR_RDFILE = 4,	/* CtxtReadFile error */
    XMLLINT_ERR_SCHEMACOMP = 5,	/* Schema compilation */
    XMLLINT_ERR_OUT = 6,	/* Error writing output */
    XMLLINT_ERR_SCHEMAPAT = 7,	/* Error in schema pattern */
    XMLLINT_ERR_RDREGIS = 8,	/* Error in Reader registration */
    XMLLINT_ERR_MEM = 9,	/* Out of memory error */
    XMLLINT_ERR_XPATH = 10	/* XPath evaluation error */
} xmllintReturnCode;

extern char *roster;

static int noblanks = 0;
static int format = 0;
static const char *output = NULL;
static char *encoding = NULL;
static xmllintReturnCode progresult = XMLLINT_RETURN_OK;
static int options = XML_PARSE_COMPACT | XML_PARSE_BIG_LINES;

/************************************************************************
 *									*
 *			I/O Interfaces					*
 *									*
 ************************************************************************/

static int myRead(void *f, char *buf, int len) {
    return(fread(buf, 1, len, (FILE *) f));
}

static int myClose(void *context) {
    FILE *f = (FILE *) context;
    if (f == stdin)
        return(0);
    return(fclose(f));
}

/************************************
 *									*
 *			 processing				*
 *									*
 ************************************/
static void parseAndPrintFile(char *filename, xmlParserCtxtPtr rectxt)
{
    xmlDocPtr doc = NULL;
	FILE *f;

		f = fopen(filename, "rb");
		if (f != NULL) {
			// parse an XML document from I/O functions and source and build a tree.
			// Returns:	the resulting document tree
			doc = xmlReadIO(myRead, myClose, f, filename, NULL, options);
		} else
		    doc = NULL;
	    xmlSaveCtxtPtr ctxt;
		int saveOpts = 0;

           saveOpts |=  XML_SAVE_FORMAT;    /* format save output */

		    ctxt = xmlSaveToFilename(output, encoding, saveOpts);

		if (ctxt != NULL) {
		    if (xmlSaveDoc(ctxt, doc) < 0) {
			fprintf(stderr, "failed save to %s\n",
				output ? output : "-");
			progresult = XMLLINT_ERR_OUT;
		    }
		    xmlSaveClose(ctxt);
		} else {
		    progresult = XMLLINT_ERR_OUT;		/* Error writing output */
		}
    xmlFreeDoc(doc);
}

// in main.c
// argc stands for argument count and argv stands for argument values.
// These are variables passed to the main function when it starts executing.
// argc will be the number of strings pointed to by argv.
// This will (in practice) be 1 plus the number of arguments,
// as virtually all implementations will prepend the name of the program to the array.
// The variables are named argc (argument count) and argv (argument vector) by convention,
// but they can be given any valid identifier

int xmllint_pm(){			// int argc, char **argv) {
    int i;
    int files = 0;

    int argc = 4;
    char *argv[5];
    argv[1] = "--format";
    argv[2] = roster;
    argv[3] = "-o";
    argv[4] = roster;

    for (i = 1; i < argc ; i++) {

		if ((!strcmp(argv[i], "-o")) ||(!strcmp(argv[i], "-output"))
				||(!strcmp(argv[i], "--output"))) {
			i++;
	    output = argv[i];
	    printf(" index %d output %s  \n",i,output);
	}

		else if ((!strcmp(argv[i], "-format")) || (!strcmp(argv[i], "--format"))) {
	     noblanks++;
			#ifdef LIBXML_OUTPUT_ENABLED
	     format = 1;
			#endif /* LIBXML_OUTPUT_ENABLED */
	     xmlKeepBlanksDefault(0);
	     printf(" index %d format %d  \n",i,format);
		}
    }  // end for


    for (i = 1; i < argc ; i++) {
		if ((argv[i][0] != '-') || (strcmp(argv[i], "-") == 0))
		{
		    parseAndPrintFile(argv[i], NULL);
		    printf(" index %d  parseAndPrintFile %s \n",i,argv[i]);
	    files ++;
		}
    }

    return(progresult);
}
