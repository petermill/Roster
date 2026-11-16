/*
 * import_glue.c
 *
 *  Created on: 7 nov. 2026
 *      Author: pierre
 */
//============================================================================
// Name        : import_glue.c
//
// purpose: GTK part of the import from the CS (database.cur) into the roster.
//          Button handler: on_btn_import_clicked  (declare it in Glade)
//
// 1) choose database.cur  2) report (nothing written)  3) "Appliquer" writes
//    roster.xml after a copy to roster.xml.bak
//
// history:  2026-11-07 first version
//==================================================================================

#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "cs-import.h"

extern char *roster;
extern GtkListStore *lst_store_loco_info;
void load_roster_element(xmlNode *a_node);
int  xmllint_pm();

static char last_db[1024] = "";

static void popup(GtkMessageType type, const char *msg)
{
	GtkWidget *dlg = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, type, GTK_BUTTONS_OK, "%s", msg);
	gtk_dialog_run(GTK_DIALOG(dlg));
	gtk_widget_destroy(dlg);
}

static char *choose_database(void)
{
	GtkWidget *dlg = gtk_file_chooser_dialog_new("Choisir le database.cur du CS", NULL,
	                       GTK_FILE_CHOOSER_ACTION_OPEN,
	                       "_Annuler", GTK_RESPONSE_CANCEL,
	                       "_Ouvrir",  GTK_RESPONSE_ACCEPT, NULL);
	if (last_db[0]) gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dlg), last_db);

	char *res = NULL;
	if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_ACCEPT) {
		res = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg));
		if (res) g_strlcpy(last_db, res, sizeof(last_db));
	}
	gtk_widget_destroy(dlg);
	return res;
}

/* affiche le rapport ; retourne TRUE si l'utilisateur clique sur "Appliquer" */
static gboolean show_report(const char *text, gboolean can_apply)
{
	GtkWidget *dlg = gtk_dialog_new_with_buttons("Import depuis le CS", NULL, GTK_DIALOG_MODAL,
	                       "_Fermer", GTK_RESPONSE_CANCEL, NULL);
	if (can_apply) gtk_dialog_add_button(GTK_DIALOG(dlg), "_Appliquer", GTK_RESPONSE_ACCEPT);

	GtkWidget *sw = gtk_scrolled_window_new(NULL, NULL);
	GtkWidget *tv = gtk_text_view_new();
	gtk_text_view_set_editable(GTK_TEXT_VIEW(tv), FALSE);
	gtk_text_view_set_monospace(GTK_TEXT_VIEW(tv), TRUE);
	gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv)), text, -1);
	gtk_container_add(GTK_CONTAINER(sw), tv);
	gtk_widget_set_size_request(sw, 820, 420);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dlg))), sw, TRUE, TRUE, 0);
	gtk_widget_show_all(dlg);

	gboolean apply = (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_ACCEPT);
	gtk_widget_destroy(dlg);
	return apply;
}

void on_btn_import_clicked()
{
	char *db = choose_database();
	if (!db) return;

	cs_import_t *imp = g_malloc0(sizeof(*imp));
	if (cs_import_read(db, imp) < 0) {
		popup(GTK_MESSAGE_ERROR, "Impossible de lire le fichier database.cur choisi.");
		g_free(db); g_free(imp);
		return;
	}
	if (imp->count == 0) {
		popup(GTK_MESSAGE_WARNING, "Aucune loco trouvee dans ce fichier (format inattendu ?).");
		g_free(db); g_free(imp);
		return;
	}

	xmlDoc *doc = xmlReadFile(roster, NULL, 0);
	xmlNodePtr root = doc ? xmlDocGetRootElement(doc) : NULL;
	if (!doc || !root) {
		popup(GTK_MESSAGE_ERROR, "Impossible de lire roster.xml.");
		if (doc) xmlFreeDoc(doc);
		g_free(db); g_free(imp);
		return;
	}

	/* 1) plan + rapport : rien n'est ecrit */
	cs_import_plan(root, imp);
	char *report = g_malloc(32768);
	cs_import_report(imp, report, 32768);
	gboolean apply = show_report(report, cs_import_actionable(imp) > 0);

	/* 2) application, apres sauvegarde */
	if (apply) {
		gchar *content = NULL;
		gsize  len = 0;
		gchar *bak = g_strconcat(roster, ".bak", NULL);
		if (!g_file_get_contents(roster, &content, &len, NULL) ||
		    !g_file_set_contents(bak, content, (gssize)len, NULL)) {
			popup(GTK_MESSAGE_ERROR, "Sauvegarde roster.xml.bak impossible : import annule.");
		} else {
			int n = cs_import_apply(doc, root, imp);
			xmlSaveFormatFile(roster, doc, 1);
			xmllint_pm();                       /* reformate le fichier */

			gtk_list_store_clear(lst_store_loco_info);
			load_roster_element(root);          /* rafraichit la liste depuis le DOM */

			char msg[160];
			g_snprintf(msg, sizeof(msg), "%d modification(s) appliquee(s).\nSauvegarde : roster.xml.bak", n);
			popup(GTK_MESSAGE_INFO, msg);
		}
		g_free(content);
		g_free(bak);
	}

	xmlFreeDoc(doc);
	g_free(report);
	g_free(db);
	g_free(imp);
}



