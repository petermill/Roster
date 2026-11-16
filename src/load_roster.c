//============================================================================
// Name        : roster_main.c
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
//
// purpose: Manage the loco roster to be used in WiFi with EngineDriver
//
//
// how: roster is inspired from JMRI roster
//
// history:  2022-07-30 first version
//==================================================================================


/**
 * section: Tree
 * synopsis: Navigates a tree to print element names
 * purpose: Parse a file to a tree, use xmlDocGetRootElement() to
 *          get the root element, then walk the document and print
 *          all the element name in document order.
 * usage: tree1 filename_or_URL
 * test: tree1 test2.xml > tree1.tmp && diff tree1.tmp $(srcdir)/tree1.res
 * author: Dodji Seketeli
 * copy: see Copyright for the status of this software.
 */

/*
Structure xmlNode
struct _xmlNode {
    void *	_private	: application data
    xmlElementType	type	: type number, must be second !
    const xmlChar *	name	: the name of the node, or the entity
    struct _xmlNode *	children	: parent->childs link
    struct _xmlNode *	last	: last child link
    struct _xmlNode *	parent	: child->parent link
    struct _xmlNode *	next	: next sibling link
    struct _xmlNode *	prev	: previous sibling link
    struct _xmlDoc *	doc	: the containing document End of common p
    xmlNs *	ns	: pointer to the associated namespace
    xmlChar *content	: the content
    struct _xmlAttr *	properties	: properties list
    xmlNs *	nsDef	: namespace definitions on this node
    void *	psvi	: for type/PSVI informations
    unsigned short	line	: line number
    unsigned short	extra	: extra data for XPath/XSLT
}

Enum xmlElementType {
    XML_ELEMENT_NODE = 1
    XML_ATTRIBUTE_NODE = 2
    XML_TEXT_NODE = 3
    XML_CDATA_SECTION_NODE = 4
    XML_ENTITY_REF_NODE = 5
    XML_ENTITY_NODE = 6
    XML_PI_NODE = 7
    XML_COMMENT_NODE = 8
    XML_DOCUMENT_NODE = 9
    XML_DOCUMENT_TYPE_NODE = 10
    XML_DOCUMENT_FRAG_NODE = 11
    XML_NOTATION_NODE = 12
    XML_HTML_DOCUMENT_NODE = 13
    XML_DTD_NODE = 14
    XML_ELEMENT_DECL = 15
    XML_ATTRIBUTE_DECL = 16
    XML_ENTITY_DECL = 17
    XML_NAMESPACE_DECL = 18
    XML_XINCLUDE_START = 19
    XML_XINCLUDE_END = 20
    XML_DOCB_DOCUMENT_NODE = 21
}
*/
#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <gtk/gtkmain.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <string.h>

#include "GTK_pointer.h"

extern char *roster;
uint8_t protocol = 0;
uint8_t date = 0;

/* valeur de la cle M4_UID dans <attributepairs> de la loco ; "" si absente */
static void loco_uid(xmlNode *loco, char *out, size_t max)
{
	out[0] = '\0';
	for (xmlNode *ap = loco->children; ap; ap = ap->next)
	{
		if (ap->type != XML_ELEMENT_NODE || xmlStrcmp(ap->name, (const xmlChar *)"attributepairs")) continue;
		for (xmlNode *kv = ap->children; kv; kv = kv->next)
		{
			if (kv->type != XML_ELEMENT_NODE || xmlStrcmp(kv->name, (const xmlChar *)"keyvaluepair")) continue;
			xmlChar *key = NULL, *val = NULL;
			for (xmlNode *c = kv->children; c; c = c->next)
			{
				if (c->type != XML_ELEMENT_NODE) continue;
				if (!xmlStrcmp(c->name, (const xmlChar *)"key"))        key = xmlNodeGetContent(c);
				else if (!xmlStrcmp(c->name, (const xmlChar *)"value")) val = xmlNodeGetContent(c);
			}
			if (key && val && !xmlStrcmp(key, (const xmlChar *)"M4_UID")) {
				strncpy(out, (const char *)val, max - 1);
				out[max - 1] = '\0';
			}
			if (key) xmlFree(key);
			if (val) xmlFree(val);
		}
	}
}

void load_roster_element(xmlNode * a_node)
{
    xmlNode *cur_node = NULL;
	xmlChar *id;
	xmlChar *dccAddress;

	xmlChar *mfg;
	xmlChar *model;
	xmlChar *content;
	xmlChar *owner;

	xmlChar *road;

	//  struct _xmlNode *	next	: next sibling link
    for (cur_node = a_node; cur_node; cur_node = cur_node->next) {

		// Appends a new row to list_store . iter will be changed to point to this new row.
		// The row will be empty after this function is called.
		// To fill in values, you need to call gtk_list_store_set().
    	//g_print("LRE line %d type %d content: %s  \n", cur_node-> line ,cur_node->type,cur_node->content);

		if ( cur_node->type == XML_ELEMENT_NODE ) {
			//g_print("line type XML_ELEMENT_NODE \n");
			/*  ID / Name  */
			if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"locomotive"))) {
			gtk_list_store_append (lst_store_loco_info,&iter);
			id = xmlGetProp(cur_node, (xmlChar *) "id");
			g_print(" id: %s ", id);
			gtk_list_store_set (lst_store_loco_info ,&iter,0, id,-1);

			char uid[16];
			loco_uid(cur_node, uid, sizeof(uid));
			gtk_list_store_set (lst_store_loco_info ,&iter,9, uid,-1);
			g_print(" uid: %s\n", uid);
			}
			/* DCC add  */
			if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"locomotive"))) {
			dccAddress = xmlGetProp(cur_node, (xmlChar *)"dccAddress");
			g_print(" loc dccAddress: %s", dccAddress);
			gtk_list_store_set (lst_store_loco_info ,&iter,1,dccAddress,-1);
			}

			/* protocol */
			if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"protocol"))) {
				protocol = 1;
				//g_print(" loc protocol: %d \n", protocol);
			}

			/* mfg  */
			if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"locomotive"))) {
			mfg = xmlGetProp(cur_node,(xmlChar *) "mfg");
			g_print(" loc mfg: %s\n", mfg);
			gtk_list_store_set (lst_store_loco_info ,&iter,3,mfg,-1);
			}

			/* model  */
			if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"locomotive"))) {
			model = xmlGetProp(cur_node, (xmlChar *)"model");
			g_print(" loco productID: %s\n", model);
			gtk_list_store_set (lst_store_loco_info ,&iter,4,model,-1);
			}

			/* decoder model  */
			if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"decoder"))) {
			content = xmlGetProp(cur_node, (xmlChar *)"model");
			g_print(" decoder: %s\n", content);
			gtk_list_store_set (lst_store_loco_info ,&iter,5,content,-1);
			}

			/* owner  */
			if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"locomotive"))) {
			owner = xmlGetProp(cur_node, (xmlChar *)"owner");
			g_print(" owner: %s\n", owner);
			gtk_list_store_set (lst_store_loco_info ,&iter,6,owner,-1);
			}

			// void	xmlNodeAddContent (xmlNodePtr cur, const xmlChar * content)

			if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"dateUpdated"))) {
				date = 1;
				//g_print(" date updated: %d \n", date);
			}

			/* roadName  */
			if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"locomotive"))) {
			road = xmlGetProp(cur_node, (xmlChar *)"roadName");
			gtk_list_store_set (lst_store_loco_info ,&iter,8,road,-1);
			}

		}//end element

		if ( cur_node->type == XML_TEXT_NODE )
		{
			//g_print("line type XML_TEXT_NODE \n");
			// returns zero if strings are equal
			if ( (xmlStrcmp(cur_node->content, (const xmlChar *)" ")) > 0 )
			{
			//g_print("line %d content: %s  \n", cur_node-> line ,cur_node->content);
			if (protocol == 1)
				{
					protocol = 0;
					gtk_list_store_set (lst_store_loco_info ,&iter,2,cur_node->content,-1);
				}
			if (date == 1)
				{
					date = 0;
					gtk_list_store_set (lst_store_loco_info ,&iter,7,cur_node->content,-1);
				}
			}

		}// end text node
        load_roster_element(cur_node->children);
    }// end for
}

int load_roster()
{
	char *docname = roster;
	xmlDoc *doc = NULL;
	xmlNode *root_element = NULL;

	/*parse the file and get the DOM */
	doc = xmlReadFile(docname, NULL, 0);

	if (doc == NULL) {
		printf("error: could not parse file n");
	}
	else { printf (" roster parsed ");
		printf("read file %s\n",docname);}

	/*Get the root element node */
	root_element = xmlDocGetRootElement(doc);

    gtk_list_store_clear(lst_store_loco_info);
    load_roster_element(root_element);

    /*free the document */
  //  xmlFreeDoc(doc);
    xmlFreeDoc(doc);
    /*
     *Free the global variables that may
     *have been allocated by the parser.
     */
    xmlCleanupParser();

    return 0;
}

void on_btn_load_clicked()
{
	printf(" load_roster() \n");
	load_roster();
}

