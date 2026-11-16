//============================================================================
// Name        : turnouts_page.h
//
// purpose: "Aiguillages" page of the Roster program (list + form).
//          Data are kept in turnouts.xml, next to roster.xml (see turnouts_xml.h).
//
// history:  2026-11-16 step 1: load and display the list
//           2026-11-16 step 2: New / Edit / Delete with save
//==================================================================================

#ifndef TURNOUTS_PAGE_H_
#define TURNOUTS_PAGE_H_

#include <gtk/gtk.h>

/* call once, after gtk_builder_connect_signals() and BEFORE g_object_unref(builder).
 * roster_path is the path of roster.xml: turnouts.xml is looked for in the same folder. */
void turnouts_page_init(GtkBuilder *builder, const char *roster_path);

/* rebuild the GtkListStore from the in-memory list */
void turnouts_page_refresh(void);

/* button handlers: to be named in Glade (signal "clicked") */
void on_btn_turnout_new_clicked(GtkButton *button, gpointer data);
void on_btn_turnout_edit_clicked(GtkButton *button, gpointer data);
void on_btn_turnout_delete_clicked(GtkButton *button, gpointer data);

#endif /* TURNOUTS_PAGE_H_ */
