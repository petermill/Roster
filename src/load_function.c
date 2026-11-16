//============================================================================
// Name        : load_function.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gtk/gtkmain.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <stdbool.h>

#include "GTK_pointer.h"
#include "config.h"

extern GtkEntry *list_func_entry[];
extern GtkSwitch *list_func_switch[];

extern char *roster;
extern gchar *ID;
extern int found;

void Clean_previous_function_entry()
{
	for (uint8_t i = 0; i < 29; i++)
	{
		gtk_entry_set_text(list_func_entry[i],(gchar *) " ");
		gtk_switch_set_active((list_func_switch[i]), 0 );
	}
}

void Clean_parameters_pan()
{
	 // prepare display function pan
	  Clean_previous_function_entry();

	  // empty data on roster pan
	  g_print(" empty data on roster pan \n");
	  gtk_entry_set_text(GTK_ENTRY(ent_ID), (gchar *)" ");
	  gtk_entry_set_text(GTK_ENTRY(ent_actual_loco), (gchar *)" ");

	  gtk_entry_set_text(GTK_ENTRY(ent_add), (gchar *)" ");

	   gtk_entry_set_text(GTK_ENTRY(ent_prot), (gchar *)" ");

	 gtk_entry_set_text(GTK_ENTRY(ent_manuf), (gchar *)" ");

	  gtk_entry_set_text(GTK_ENTRY(ent_model), (gchar *)" ");

	 gtk_entry_set_text(GTK_ENTRY(ent_decoder), (gchar *)" ");

	  gtk_entry_set_text(GTK_ENTRY(ent_owner), (gchar *)" ");

	  gtk_entry_set_text(GTK_ENTRY(ent_date), (gchar *)" ");

	  gtk_entry_set_text(GTK_ENTRY(ent_road), (gchar *)" ");

	  gtk_entry_set_text(GTK_ENTRY(ent_uid), (gchar *)" ");
}

void display_element_function(xmlNode * a_node)
{
	xmlChar *id;
	xmlNode *cur_node = NULL;
	xmlChar *num;
	xmlChar *lockable;
	bool switch_state;

	//  struct _xmlNode *	next	: next sibling link
    for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
		if ( cur_node->type == XML_ELEMENT_NODE )
		{
			if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"locomotive"))) {
				found = 0;
			id = xmlGetProp(cur_node, (xmlChar *) "id");
#if DEBUG == 1
			//printf("LF loc id: %s \n", id);
#endif
				if ((!xmlStrcmp(id, (const xmlChar *)ID)))
				{
					printf("LF found id for %s \n", ID) ;
					found = 1;
				}
			}	// end look for locomotive

			if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"functionlabel"))) {
				if (found == 1){

					num = xmlGetProp(cur_node, (xmlChar *) "num");
				//	printf("LF  %s  %s ",num, xmlNodeGetContent(cur_node));
					lockable = xmlGetProp(cur_node, (xmlChar *) "lockable");
					if (!xmlStrcmp((xmlChar *)lockable, (xmlChar *)"true"))  switch_state = 1 ;
					else switch_state = 0;
				 //  printf("LF  %s %d \n",lockable, switch_state);
				gtk_entry_set_text((list_func_entry[atoi((char *)num)]),(gchar *) xmlNodeGetContent(cur_node));
				gtk_switch_set_active((list_func_switch[atoi((char *)num)]), switch_state );
				}
		   }	// end look for functionlabel
	  }		// end cur_node->type == XML_ELEMENT_NODE
		//printf( "LF call disp_element ");
	display_element_function(cur_node->children);
    }	// end for loop
}

int load_function()
{
	char *docname = roster;
		xmlDoc *doc = NULL;
		xmlNode *root_element = NULL;

		/*parse the file and get the DOM */
		doc = xmlReadFile(docname, NULL, 0);

		if (doc == NULL) {
			printf("error: could not parse file n");
		}
		else { printf (" roster parsed; read file %s\n",docname);
			}

    /*Get the root element node */
    root_element = xmlDocGetRootElement(doc);

    printf("in load_function Clean_previous_function_entry \n");

    Clean_previous_function_entry();

    display_element_function(root_element);

    printf("in load_function free the document \n");
    /*free the document */
    xmlFreeDoc(doc);

    /*
     *Free the global variables that may
     *have been allocated by the parser.
     */
    printf("in load_function CleanupParser \n");
    xmlCleanupParser();

    return 0;
}
