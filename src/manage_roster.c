//============================================================================
// Name        : manage_roster.c
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

#include "config.h"
#include "GTK_pointer.h"
#include "load_function.h"
#include "manage_function.h"
#include "load_roster.h"
#include "xmllint.h"


unsigned char *loco_adr;

uint8_t row_selected = 0;

gchar *ID = NULL;		// the selected row

extern char *roster;
extern GtkEntry *list_func_entry[];
extern GtkSwitch *list_func_switch[];

typedef  enum
    {
      col_ID= 0,	    	// gchararray
      col_DCC_ad = 1,   	// gchararray
	  col_protocol = 2,
  	  col_manufacturer = 3,	// gchararray
  	  col_model = 4,		// gchararray
      col_decoder = 5,	    // gchararray
	  col_owner = 6,
	  col_date = 7,
  	  col_road = 8,			// gchararray
    } t_colonne;

    /* ---- aides : <locomotive> par id, et cle M4_UID dans <attributepairs> ---- */
    static xmlNodePtr find_loco_by_id(xmlNode *a_node, const char *id)
    {
    	for (xmlNode *n = a_node; n; n = n->next)
    	{
    		if (n->type != XML_ELEMENT_NODE) continue;
    		if (!xmlStrcmp(n->name, (const xmlChar *)"locomotive"))
    		{
    			xmlChar *v = xmlGetProp(n, (const xmlChar *)"id");
    			int same = (v && !xmlStrcmp(v, (const xmlChar *)id));
    			if (v) xmlFree(v);
    			if (same) return n;
    		}
    		xmlNodePtr r = find_loco_by_id(n->children, id);
    		if (r) return r;
    	}
    	return NULL;
    }

    static xmlNodePtr find_child(xmlNodePtr p, const char *name)
    {
    	for (xmlNode *n = p->children; n; n = n->next)
    		if (n->type == XML_ELEMENT_NODE && !xmlStrcmp(n->name, (const xmlChar *)name)) return n;
    	return NULL;
    }

    static xmlNodePtr find_uid_pair(xmlNodePtr loco)
    {
    	xmlNodePtr ap = find_child(loco, "attributepairs");
    	if (!ap) return NULL;
    	for (xmlNode *kv = ap->children; kv; kv = kv->next)
    	{
    		if (kv->type != XML_ELEMENT_NODE || xmlStrcmp(kv->name, (const xmlChar *)"keyvaluepair")) continue;
    		xmlNodePtr key = find_child(kv, "key");
    		if (!key) continue;
    		xmlChar *k = xmlNodeGetContent(key);
    		int same = (k && !xmlStrcmp(k, (const xmlChar *)"M4_UID"));
    		if (k) xmlFree(k);
    		if (same) return kv;
    	}
    	return NULL;
    }

    static void get_uid(xmlNodePtr loco, char *out, size_t max)
    {
    	out[0] = '\0';
    	xmlNodePtr kv  = find_uid_pair(loco);
    	xmlNodePtr val = kv ? find_child(kv, "value") : NULL;
    	if (!val) return;
    	xmlChar *v = xmlNodeGetContent(val);
    	if (v) { g_strlcpy(out, (const char *)v, max); xmlFree(v); }
    }

    /* vide -> supprime la cle ; sinon 8 caracteres hexa obligatoires. 0 = ok, -1 = format invalide */
    static int set_uid(xmlNodePtr loco, const char *txt)
    {
    	gchar *t = g_strstrip(g_strdup(txt));
    	xmlNodePtr kv = find_uid_pair(loco);

    	if (t[0] == '\0') {
    		if (kv) { xmlUnlinkNode(kv); xmlFreeNode(kv); }
    		g_free(t);
    		return 0;
    	}
    	if (strlen(t) != 8) { g_free(t); return -1; }
    	for (int i = 0; i < 8; i++)
    		if (!g_ascii_isxdigit(t[i])) { g_free(t); return -1; }

    	gchar *uid = g_ascii_strdown(t, -1);
    	g_free(t);

    	if (kv) {
    		xmlNodePtr val = find_child(kv, "value");
    		if (val) xmlNodeSetContent(val, (const xmlChar *)uid);
    		else xmlNewTextChild(kv, NULL, (const xmlChar *)"value", (const xmlChar *)uid);
    	} else {
    		xmlNodePtr ap = find_child(loco, "attributepairs");
    		if (!ap) ap = xmlNewChild(loco, NULL, (const xmlChar *)"attributepairs", NULL);
    		kv = xmlNewChild(ap, NULL, (const xmlChar *)"keyvaluepair", NULL);
    		xmlNewTextChild(kv, NULL, (const xmlChar *)"key",   (const xmlChar *)"M4_UID");
    		xmlNewTextChild(kv, NULL, (const xmlChar *)"value", (const xmlChar *)uid);
    	}
    	g_free(uid);
    	return 0;
    }

    static void display_uid(void)
    {
    	gtk_entry_set_text(ent_uid, " ");
    	xmlDoc *doc = xmlReadFile(roster, NULL, 0);
    	if (!doc) return;
    	xmlNodePtr loco = find_loco_by_id(xmlDocGetRootElement(doc), ID);
    	if (loco) {
    		char uid[16];
    		get_uid(loco, uid, sizeof(uid));
    		if (uid[0]) gtk_entry_set_text(ent_uid, uid);
    	}
    	xmlFreeDoc(doc);
    }

    static void show_error(const char *msg)
    {
    	GtkWidget *top = gtk_widget_get_toplevel(GTK_WIDGET(ent_uid));
    	GtkWidget *dlg = gtk_message_dialog_new(GTK_IS_WINDOW(top) ? GTK_WINDOW(top) : NULL,
    	                                        GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR,
    	                                        GTK_BUTTONS_OK, "%s", msg);
    	gtk_dialog_run(GTK_DIALOG(dlg));
    	gtk_widget_destroy(dlg);
    }

void on_roster_tree_view_row_activated(GtkTreeView *view, GtkTreePath *path,
		                        GtkTreeViewColumn *col, gpointer userdata)
{
	// select with (single) click declared in Glade

		// https://en.wikibooks.org/wiki/GTK%2B_By_Example/Tree_View/Tree_Models#GtkTreeIter
	    /************************************************************
		 *                                                          *
		 * Converting a GtkTreePath into a GtkTreeIter              *
		 *                                                          *
		 ************************************************************/

		/************************************************************
		 *
		 * onTreeViewRowActivated: a row has been clicked
		 *
		 ************************************************************/

		  GtkTreeModel *model;
		  model = gtk_tree_view_get_model(view);
		  // gboolean gtk_tree_model_get_iter (GtkTreeModel *tree_model,GtkTreeIter *iter,GtkTreePath *path);
		  if (gtk_tree_model_get_iter(model, &iter, path))
		 {
			  gtk_tree_model_get(model, &iter, col_ID, &loco_adr, -1);

			  gchar *rucksack;

			  gtk_tree_model_get(model, &iter, col_ID, &ID, -1);
#if DEBUG == 1
			  g_print ("The row containing the ID/name '%s' has been clicked.\n", ID);
#endif
			  row_selected = 1;

			 // prepare display function pan
			  Clean_previous_function_entry();
			  load_function();
			  display_uid();

			  // display data on roster pan
			  g_print(" display data on roster pan \n");
			  gtk_entry_set_text(GTK_ENTRY(ent_ID), (gchar *)ID);
			  // enter actual ID in page function
			  gtk_entry_set_text(GTK_ENTRY(ent_actual_loco), (gchar *)ID);

			  gtk_tree_model_get(model, &iter, col_DCC_ad, &rucksack, -1);
			  gtk_entry_set_text(GTK_ENTRY(ent_add), (gchar *)rucksack);

			  gtk_tree_model_get(model, &iter, col_protocol, &rucksack, -1);
			  gtk_entry_set_text(GTK_ENTRY(ent_prot), (gchar *)rucksack);

			  gtk_tree_model_get(model, &iter, col_manufacturer, &rucksack, -1);
			  gtk_entry_set_text(GTK_ENTRY(ent_manuf), (gchar *)rucksack);

			  gtk_tree_model_get(model, &iter, col_model, &rucksack, -1);
			  gtk_entry_set_text(GTK_ENTRY(ent_model), (gchar *)rucksack);

			  gtk_tree_model_get(model, &iter, col_decoder, &rucksack, -1);
			  gtk_entry_set_text(GTK_ENTRY(ent_decoder), (gchar *)rucksack);

			  gtk_tree_model_get(model, &iter, col_owner, &rucksack, -1);
			  gtk_entry_set_text(GTK_ENTRY(ent_owner), (gchar *)rucksack);

			  gtk_tree_model_get(model, &iter, col_date, &rucksack, -1);
			  gtk_entry_set_text(GTK_ENTRY(ent_date), (gchar *)rucksack);

			  gtk_tree_model_get(model, &iter, col_road, &rucksack, -1);
			  gtk_entry_set_text(GTK_ENTRY(ent_road), (gchar *)rucksack);

			  g_free(rucksack);

	       }
}

void delete_node(xmlNode * a_node)
{
	//xmlChar *id;
	xmlNode *cur_node = NULL;
	//xmlNodePtr newnode;		// xmlNewTextChild return a pointer to next node

	for (cur_node = a_node; cur_node; cur_node = cur_node->next)
	{
		if ( cur_node->type == XML_ELEMENT_NODE )
		{
		/*  ID / Name  */
			//printf(" name %s \n", cur_node->name);
		if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"locomotive"))) {

			xmlChar *id = xmlGetProp(cur_node, (xmlChar *) "id");
			// printf(" loc id: %s \n", id);

			if ((!xmlStrcmp(id, (const xmlChar *)ID)))
			{
#if DEBUG == 1
				printf(" delete_node found id for %s \n", ID) ;
#endif
				xmlUnlinkNode(cur_node);
				xmlFreeNode(cur_node);
			}
		}  // end if locomotive

	  }  // end if type == XML_ELEMENT_NODE
		delete_node(cur_node->children);
    }		// end for
}

void on_btn_delete_clicked()
{
	char *docname = roster;
	xmlDoc *doc = NULL;
	xmlNode *root_element = NULL;

	doc = xmlReadFile(docname, NULL, 0);
	if (doc == NULL) {
	        printf("error: could not parse file n");
	    }
	    else { printf (" roster parsed \n");}

	root_element = xmlDocGetRootElement(doc);

	delete_node(root_element);

	printf("save file %s\n", docname);
		if (doc != NULL) {
		xmlSaveFormatFile (docname, doc, 1);
		}

		xmllint_pm();

	// update display
		#if DEBUG == 1
		printf(" update display \n");
		#endif
		Clean_parameters_pan();

		gtk_list_store_clear(lst_store_loco_info);
		load_roster_element(root_element);

	if (doc != NULL) {
	xmlSaveFormatFile (docname, doc, 1);
	}
	xmlFreeDoc(doc);
	xmlCleanupParser();
}

// Hereafter there si two nested loops
// the for loop with cur_node->next
// and the second with add_new_element(cur_node->children);
void add_new_element(xmlNode * a_node)
{
	xmlNode *cur_node = NULL;
	xmlNodePtr newnode;		// xmlNewTextChild return a pointer to next node
	xmlNodePtr NewLocoNode;
	xmlNodePtr FunclabelsNode;
	xmlNodePtr FunclabelNode;
	char str_num[9];
	bool switch_state = 1;
	
	for (cur_node = a_node; cur_node; cur_node = cur_node->next)
	{
#if DEBUG ==1
		g_print("NE for loop line %d type %d name %s \n",cur_node->line, cur_node->type,cur_node->name);
#endif
		if ( cur_node->type == XML_ELEMENT_NODE )
		{
			if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"roster"))) {
			// create a node at the end of cur_node
#if DEBUG ==1
				g_print(" create a node at the end of <roster> \n");
#endif
			newnode = xmlNewTextChild (cur_node, NULL, (xmlChar *)"locomotive",(xmlChar *)" ");

			const gchar *ID = gtk_entry_get_text (ent_ID);
			xmlNewProp (newnode,(xmlChar *) "id",(xmlChar *)ID);

			const gchar *add = gtk_entry_get_text (ent_add);
			xmlNewProp (newnode,(xmlChar *) "dccAddress",(xmlChar *)add);

			const gchar *manuf = gtk_entry_get_text (ent_manuf);
			xmlNewProp (newnode,(xmlChar *) "mfg",(xmlChar *)manuf);

			const gchar *model = gtk_entry_get_text (ent_model);
			xmlNewProp (newnode,(xmlChar *) "model",(xmlChar *)model);

			const gchar *own = gtk_entry_get_text (ent_owner);
			xmlNewProp (newnode,(xmlChar *) "owner",(xmlChar *)own);

			const gchar *road = gtk_entry_get_text (ent_road);
			xmlNewProp (newnode,(xmlChar *) "roadName",(xmlChar *)road);
#if DEBUG ==1
			g_print("create a node at the end of newnode ");
			g_print("NE line %d type %d name %s \n",newnode->line, newnode->type,newnode->name);
#endif
			NewLocoNode = newnode;

			// new element for date updated
			const gchar *date = gtk_entry_get_text (ent_date);
				// returns zero if strings are equal  int xmlStrcmp(const xmlChar *str1,const xmlChar *str2);
				if ( (xmlStrcmp((const xmlChar *)date, (const xmlChar *)" ")) > 0 )   // date is not empty
			newnode = xmlNewTextChild (NewLocoNode, NULL, (xmlChar *)"dateUpdated",(xmlChar *)date);
				else
			newnode = xmlNewTextChild (NewLocoNode, NULL, (xmlChar *)"dateUpdated",(xmlChar *)"?");

				//	 new element for decoder
			newnode = xmlNewTextChild (NewLocoNode, NULL, (xmlChar *)"decoder",(xmlChar *)" ");
			const gchar *decoder = gtk_entry_get_text (ent_decoder);
			xmlNewProp (newnode,(xmlChar *) "model",(xmlChar *)decoder);

			// new element for protocol
			const gchar *pro = gtk_entry_get_text (ent_prot);
				// returns zero if strings are equal  int xmlStrcmp(const xmlChar *str1,const xmlChar *str2);
				if ( (xmlStrcmp((const xmlChar *)pro, (const xmlChar *)" ")) > 0 )   // date is not empty
			newnode = xmlNewTextChild (NewLocoNode, NULL, (xmlChar *)"protocol",(xmlChar *)pro);
				else
			newnode = xmlNewTextChild (NewLocoNode, NULL, (xmlChar *)"protocol",(xmlChar *)"?");

			// new element for functionlabels
			newnode = xmlNewTextChild (NewLocoNode, NULL, (xmlChar *)"functionlabels",(xmlChar *)" ");

			FunclabelsNode = newnode;

			for (uint8_t count = 0; count <29; count++)
			{
				const gchar *func_value = gtk_entry_get_text(list_func_entry[count]);

				if (xmlStrcmp((xmlChar *)func_value, (xmlChar *)" ")) {
					g_print("count %d fonction value %s  ",count, func_value);
					newnode = xmlNewChild(FunclabelsNode, NULL, (xmlChar *)"functionlabel",(xmlChar *)func_value);
					// xmlNewTextChild return a pointer to next node
					FunclabelNode = newnode;
					// add attributes  "num"
					sprintf(str_num, "%d", count);
					xmlNewProp(FunclabelNode, (xmlChar *)"num", (xmlChar *)str_num );
					// add attributes  "lockable"
					// Gets whether the GtkSwitch is in its “on” or “off” state.
					switch_state = 	gtk_switch_get_active (list_func_switch[count]);
					char str_true[] = {'t', 'r', 'u','e', '\0' };
					char str_false[] = {'f', 'a', 'l', 's','e', '\0' };
					char str_lock[8] = {};
					if (switch_state == 1) strcat( str_lock, str_true);
					if (switch_state == 0) strcat( str_lock, str_false);	// dest src
					xmlNewProp(FunclabelNode, (xmlChar *)"lockable",(unsigned char *) str_lock);
					g_print("lockable %s \n  ",str_lock);
					}
				}// end for
			set_uid(NewLocoNode, gtk_entry_get_text(ent_uid));
			}// end if roster
		}// end if XML_ELEMENT_NODE
	add_new_element(cur_node->children);
	}// end for loop
}

void on_btn_new_clicked()
{
	char *docname = roster;

	xmlDoc *doc = NULL;
	xmlNode *root_element = NULL;

	/*parse the file and get the DOM */
	doc = xmlReadFile(docname, NULL, 0);
	g_print("read file %s\n", docname);

	/*Get the root element node */
	root_element = xmlDocGetRootElement(doc);

	if (doc == NULL || root_element == NULL)
		{ g_print("new: roster illisible\n");
		if (doc) xmlFreeDoc(doc);
		return;
		}

	const gchar *newid = gtk_entry_get_text(ent_ID);
	gchar *t = g_strstrip(g_strdup(newid));
	int vide = (t[0] == '\0');
	g_free(t);
	if (vide) {
		show_error("Identifiant vide : saisissez un nom pour la loco.");
		xmlFreeDoc(doc);
		return;
	}
	if (find_loco_by_id(root_element, newid) != NULL) {
		show_error("Cet identifiant existe déjà dans le roster.");
		xmlFreeDoc(doc);
		return;
	}

	g_print("add_new_element(root_element) \n");
	add_new_element(root_element);

#if DEBUG == 1
	g_print("save file %s\n", docname);
#endif
	if (doc != NULL) {
	xmlSaveFormatFile (docname, doc, 1);
	}

	xmllint_pm();

	xmlFreeDoc(doc);
	xmlCleanupParser();

// update display
	#if DEBUG == 1
	g_print("at end new: update display\n ");
	#endif

	gtk_list_store_clear(lst_store_loco_info);

	doc = xmlReadFile(docname, NULL, 0);
	g_print("read file %s\n", docname);
	root_element = xmlDocGetRootElement(doc);

	load_roster_element(root_element);

	xmlFreeDoc(doc);
	xmlCleanupParser();
}

/* met a jour (ou cree) <name>text</name> sous la loco, sans toucher au reste */
static xmlNodePtr set_child_text(xmlNodePtr loco, const char *name, const char *text)
{
	xmlNodePtr c = NULL;
	for (xmlNode *n = loco->children; n; n = n->next)
		if (n->type == XML_ELEMENT_NODE && !xmlStrcmp(n->name, (const xmlChar *)name)) { c = n; break; }

	if (!c) return xmlNewTextChild(loco, NULL, (const xmlChar *)name, (const xmlChar *)text);

	xmlChar *enc = xmlEncodeSpecialChars(loco->doc, (const xmlChar *)text);
	xmlNodeSetContent(c, enc);
	xmlFree(enc);
	return c;
}

void on_btn_edit_clicked()
{
	if (ID == NULL) { g_print("edit: aucune loco selectionnee\n"); return; }

	xmlDoc *doc = xmlReadFile(roster, NULL, 0);
	if (doc == NULL) { printf("error: could not parse file %s\n", roster); return; }
	xmlNode *root_element = xmlDocGetRootElement(doc);

	xmlNodePtr loco = find_loco_by_id(root_element, ID);
	if (loco == NULL) {
		g_print("edit: loco '%s' introuvable\n", ID);
		xmlFreeDoc(doc);
		return;
	}

	const gchar *newid = gtk_entry_get_text(ent_ID);

	/* refuse un nouvel id deja pris par une AUTRE loco */
	xmlNodePtr other = find_loco_by_id(root_element, newid);
	if (other != NULL && other != loco) {
		show_error("Cet identifiant existe déjà dans le roster.");
		g_print("edit: l'id '%s' existe deja\n", newid);
		xmlFreeDoc(doc);
		return;
	}

	xmlSetProp(loco, (const xmlChar *)"id",         (const xmlChar *)newid);
	xmlSetProp(loco, (const xmlChar *)"dccAddress", (const xmlChar *)gtk_entry_get_text(ent_add));
	xmlSetProp(loco, (const xmlChar *)"mfg",        (const xmlChar *)gtk_entry_get_text(ent_manuf));
	xmlSetProp(loco, (const xmlChar *)"model",      (const xmlChar *)gtk_entry_get_text(ent_model));
	xmlSetProp(loco, (const xmlChar *)"owner",      (const xmlChar *)gtk_entry_get_text(ent_owner));
	xmlSetProp(loco, (const xmlChar *)"roadName",   (const xmlChar *)gtk_entry_get_text(ent_road));

	const gchar *date = gtk_entry_get_text(ent_date);
	set_child_text(loco, "dateUpdated",
	               (xmlStrcmp((const xmlChar *)date, (const xmlChar *)" ") > 0) ? date : "?");


	xmlNodePtr dec = set_child_text(loco, "decoder", " ");
	xmlSetProp(dec, (const xmlChar *)"model", (const xmlChar *)gtk_entry_get_text(ent_decoder));
	const gchar *pro = gtk_entry_get_text(ent_prot);
		const char *pro_val = (xmlStrcmp((const xmlChar *)pro, (const xmlChar *)" ") > 0) ? pro : "?";
		xmlNodePtr la = find_child(loco, "locoaddress");
		if (la && find_child(la, "protocol"))
			set_child_text(la, "protocol", pro_val);      /* format JMRI d'origine */
		else
			set_child_text(loco, "protocol", pro_val);    /* locos créées par ce programme */

	/* functionlabels, attributepairs, etc. : non touches */
	if (set_uid(loco, gtk_entry_get_text(ent_uid)) != 0) {
			g_print("edit: UID invalide (8 caracteres hexa attendus)\n");
			show_error("UID invalide : 8 caractères hexadécimaux attendus (ex. ffd4e3d2).");
			xmlFreeDoc(doc);
			return;
		}
	xmlSaveFormatFile(roster, doc, 1);
	xmllint_pm();

	/* l'id selectionne devient le nouvel id */
	g_free(ID);
	ID = g_strdup(newid);

	/* rafraichit la liste depuis le DOM en memoire */
	gtk_list_store_clear(lst_store_loco_info);
	load_roster_element(root_element);

	xmlFreeDoc(doc);
	xmlCleanupParser();
}




void on_btn_leave_clicked()
{
	gtk_main_quit();
}

// called when window is closed
void delete_event()
{
	gtk_main_quit();
}
