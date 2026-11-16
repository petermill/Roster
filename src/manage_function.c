//============================================================================
// Name        : manage_function.c
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
// history:  2022-07-20 first version
//==================================================================================

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gtk/gtkmain.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <stdbool.h>
//#include <libxml/xmlsave.h>

#include "config.h"
#include "GTK_pointer.h"
#include "load_function.h"
#include "xmllint.h"

extern char *roster ;
extern gchar *ID;// the selected row

extern GtkEntry *list_func_entry[];
extern GtkSwitch *list_func_switch[];

unsigned int found = 0;

xmlNode *id_node = NULL;

void delete_function(xmlNode * a_node)
{
	xmlChar *id;
	xmlNode *cur_node = NULL;

	//  struct _xmlNode *	next	: next sibling link   lien frère et sœur
	for (cur_node = a_node; cur_node; cur_node = cur_node->next)
	{
		if ( cur_node->type == XML_ELEMENT_NODE )
		{
			//printf(" DF line %d name: %s\n", cur_node-> line ,cur_node->name);
			/*  ID / Name  */
		if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"locomotive"))) {
			found = 0;
		id = xmlGetProp(cur_node, (xmlChar *) "id");
		//printf("DF loc id found: %s \n", id);

			if ((!xmlStrcmp(id, (const xmlChar *)ID)))
			{
#if DEBUG == 1
				printf("DF delete_function found id for %s \n", ID) ;
#endif
				found = 1;
			}
			else
				{found = 0; } // printf("found = 0 \n");
		}// end locomotive  ID trouvé

		// cette recherche permet de trouver à partir de quel node to delete
		if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"functionlabels")))
		{
			if (found == 1)
			{
				xmlUnlinkNode(cur_node);
				xmlFreeNode(cur_node);
			}
		}
		// cur_node is "functionlabels" node.
	 delete_function(cur_node->children);
	}// end element node
  }// end for
}// end module

void save_function(xmlNode * a_node)
{
	xmlChar *id;
	xmlNodePtr newnode = NULL;		// xmlNewTextChild return a pointer to next node
	bool switch_state = 1;
	xmlNode *cur_node;
	xmlNodePtr LocoNode;
	xmlNodePtr FunclabelsNode;
	xmlNodePtr FunclabelNode;

	char str_num[9];

	// if GtkEntry not NULL
	// create a new node at the end of <functionlabel>
	for (cur_node = a_node; cur_node; cur_node = cur_node->next)
		{
			if ( cur_node->type == XML_ELEMENT_NODE )
			{
				//g_print(" NF line %d name: %s\n", cur_node-> line ,cur_node->name);
				/*  ID / Name  */
			if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"locomotive"))) {
				found = 0;
			id = xmlGetProp(cur_node, (xmlChar *) "id");
			g_print(" loc id: %s \n", id);

				if ((!xmlStrcmp(id, (const xmlChar *)ID)))
				{
					#if DEBUG == 1
					g_print(" NF save_function found id for %s \n", ID);
					#endif
					found = 1;

				}
				else found = 0;

			}// end locomotive  ID trouvé


			if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"functionlabels"))) {
				if (found == 1)
				{
					// cur_node is "functionlabels" node. Need to be deleted to avoid redefinition of functionlabel
#if DEBUG == 1
					printf( " NF found = 1 line %d name: %s \n", cur_node-> line ,cur_node->name);
#endif
					LocoNode = cur_node -> parent;
					FunclabelsNode = cur_node;
					xmlUnlinkNode(FunclabelsNode);
					xmlFreeNode(FunclabelsNode);

				// new element for functionlabels recreated at the end of <locomotive>
					newnode = xmlNewTextChild (LocoNode, NULL, (xmlChar *)"functionlabels",(xmlChar *)" ");

					FunclabelsNode = newnode;

// les autres éléments sont à trouver dans list_func_entry[count] si occupés et non dans le fichier.xml !
					for (uint8_t count = 0; count <29; count++)
					{
						const gchar *func_value = gtk_entry_get_text(list_func_entry[count]);

					if (xmlStrcmp((xmlChar *)func_value, (xmlChar *)" ")) {
#if DEBUG == 1
						g_print("count %d fonction value %s  \n",count, func_value);
#endif
						newnode = xmlNewChild(FunclabelsNode, NULL, (xmlChar *)"functionlabel",(xmlChar *)func_value);
						// xmlNewTextChild return a pointer to next node
						FunclabelNode = newnode;
						// add attributes  "num"
							sprintf(str_num, "%d", count);
							xmlNewProp(FunclabelNode, (xmlChar *)"num", (xmlChar *)str_num );
						// add attributes  "lockable"
						// Gets whether the GtkSwitch is in its “on” or “off” state.
							switch_state = 	gtk_switch_get_active (list_func_switch[count]);
							char str_true[] = {'t','r','u','e','\0' };
							char str_false[] = {'f','a','l','s','e','\0' };
							char str_lock[10] = {};
							if (switch_state == 1) strcat( str_lock, str_true);
							if (switch_state == 0) strcat( str_lock, str_false);	// dest src
							xmlNewProp(FunclabelNode, (xmlChar *)"lockable",(unsigned char *) str_lock);
#if DEBUG == 1
							g_print("lockable %s \n  ",str_lock);
#endif
						}
					}// end for
			}// end found = 1
		}// end function label;
			save_function(cur_node->children);
	}// end element node
  }// end for

}

// to save to roster function and image
void on_btn_sav_clicked()
{
	char *docname = roster;
	xmlDoc *doc = NULL;
	xmlNode *root_element = NULL;

	/*parse the file and get the DOM */
	doc = xmlReadFile(docname, NULL, 0);
#if DEBUG == 1
		    g_print("MF on_btn_sav_clicked read file and get the DOM %s\n", docname);
#endif
		/*Get the root element node */
		root_element = xmlDocGetRootElement(doc);

		save_function(root_element);

#if DEBUG == 1
				printf("save file %s\n", docname);
#endif
				if (doc != NULL) {
			xmlSaveFormatFile (docname, doc,  1);
			}

			xmllint_pm();

			/*free the document */
			printf ("free the document \n");
		    xmlFreeDoc(doc);
		    /* Free the global variables that may have been allocated by the parser.  */
		    printf ("CleanupParser \n");
		    xmlCleanupParser();
}

void on_btn_leave_function_clicked()
{
	// reset to Null the actual loco ID before leaving
	gtk_entry_set_text(GTK_ENTRY(ent_actual_loco), " ");

    //  return to page roster
    gtk_stack_set_visible_child_full(Main_stack,"page_roster",GTK_STACK_TRANSITION_TYPE_NONE);
#if DEBUG == 1
	printf(" leave function page \n");
#endif
}

