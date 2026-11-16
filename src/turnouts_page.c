//============================================================================
// Name        : turnouts_page.c
//
// purpose: see turnouts_page.h
//
// history:  2026-11-16 step 1: load and display the list, selection fills the form
//           2026-11-16 step 2: New / Edit / Delete with save
//==================================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include "turnouts_xml.h"
#include "turnouts_page.h"

enum { COL_NUMBER, COL_NAME, COL_INVERTED, COL_COMMENT };

static turnout_list_t tl;                    /* the list in memory */
static char           tpath[1024];           /* path of turnouts.xml */

static GtkListStore  *store;
static GtkTreeView   *tree;
static GtkEntry      *ent_number, *ent_name, *ent_comment;
static GtkToggleButton *chk_inverted;

static void show_error(const char *text)
{
    GtkWidget *d = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR,
                                          GTK_BUTTONS_OK, "%s", text);
    gtk_dialog_run(GTK_DIALOG(d));
    gtk_widget_destroy(d);
}

/* turnouts.xml in the folder of roster.xml */
static void build_path(const char *roster_path)
{
    const char *slash = strrchr(roster_path, '/');
    if (slash) {
        size_t n = (size_t)(slash - roster_path) + 1;
        if (n > sizeof(tpath) - 20) n = sizeof(tpath) - 20;
        memcpy(tpath, roster_path, n);
        tpath[n] = '\0';
        strcat(tpath, "turnouts.xml");
    } else {
        strcpy(tpath, "turnouts.xml");
    }
}

void turnouts_page_refresh(void)
{
    gtk_list_store_clear(store);
    for (int i = 0; i < tl.count; i++) {
        char num[16];
        GtkTreeIter it;
        snprintf(num, sizeof(num), "%u", tl.t[i].number);
        gtk_list_store_append(store, &it);
        gtk_list_store_set(store, &it,
                           COL_NUMBER,   num,
                           COL_NAME,     tl.t[i].name,
                           COL_INVERTED, tl.t[i].inverted ? "oui" : "non",
                           COL_COMMENT,  tl.t[i].comment,
                           -1);
    }
}

static void show_info(const char *text)
{
    GtkWidget *d = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO,
                                          GTK_BUTTONS_OK, "%s", text);
    gtk_dialog_run(GTK_DIALOG(d));
    gtk_widget_destroy(d);
}

/* confirmation bubble: closes by itself after 1.5 s (or on OK) */
typedef struct { GtkWidget *dlg; guint id; int fired; } notice_t;

static gboolean close_notice(gpointer p)
{
    notice_t *n = p;
    n->fired = 1;
    gtk_dialog_response(GTK_DIALOG(n->dlg), GTK_RESPONSE_OK);
    return G_SOURCE_REMOVE;
}

static void show_notice(const char *text)
{
    notice_t n;
    n.dlg = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO,
                                   GTK_BUTTONS_OK, "%s", text);
    n.fired = 0;
    n.id = g_timeout_add(1500, close_notice, &n);
    gtk_dialog_run(GTK_DIALOG(n.dlg));
    if (!n.fired) g_source_remove(n.id);
    gtk_widget_destroy(n.dlg);
}

static int ask_yes_no(const char *text)
{
    GtkWidget *d = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION,
                                          GTK_BUTTONS_YES_NO, "%s", text);
    int r = gtk_dialog_run(GTK_DIALOG(d));
    gtk_widget_destroy(d);
    return r == GTK_RESPONSE_YES;
}

/* number of the selected row, 0 if none */
static unsigned selected_number(void)
{
    GtkTreeModel *model;
    GtkTreeIter it;
    unsigned n = 0;
    if (gtk_tree_selection_get_selected(gtk_tree_view_get_selection(tree), &model, &it)) {
        gchar *num;
        gtk_tree_model_get(model, &it, COL_NUMBER, &num, -1);
        if (num) n = (unsigned)strtoul(num, NULL, 10);
        g_free(num);
    }
    return n;
}

/* select the row of this number and scroll to it */
static void select_number(unsigned number)
{
    int i = turnouts_find(&tl, number);
    GtkTreeIter it;
    if (i < 0) return;
    if (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &it, NULL, i)) {
        GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &it);
        gtk_tree_selection_select_iter(gtk_tree_view_get_selection(tree), &it);
        gtk_tree_view_scroll_to_cell(tree, path, NULL, FALSE, 0, 0);
        gtk_tree_path_free(path);
    }
}

/* read the form into t. 0 = ok, -1 = refused (message already shown) */
static int read_form(turnout_t *t)
{
    memset(t, 0, sizeof(*t));

    gchar *num = g_strstrip(g_strdup(gtk_entry_get_text(ent_number)));
    char *end = NULL;
    unsigned long v = strtoul(num, &end, 10);
    if (num[0] == '\0' || *end != '\0' || v < TURNOUT_NUM_MIN || v > TURNOUT_NUM_MAX) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Le numéro doit être un entier de %d à %d.",
                 TURNOUT_NUM_MIN, TURNOUT_NUM_MAX);
        show_error(msg);
        g_free(num);
        return -1;
    }
    g_free(num);
    t->number = (unsigned)v;

    gchar *name = g_strstrip(g_strdup(gtk_entry_get_text(ent_name)));
    gchar *comment = g_strstrip(g_strdup(gtk_entry_get_text(ent_comment)));
    snprintf(t->name, sizeof(t->name), "%s", name);
    snprintf(t->comment, sizeof(t->comment), "%s", comment);
    g_free(name);
    g_free(comment);
    t->inverted = gtk_toggle_button_get_active(chk_inverted) ? 1 : 0;

    /* same checks as for the whole file (forbidden characters in the name) */
    turnout_list_t one = { 1, { *t } };
    char msg[200];
    if (turnouts_validate(&one, msg, sizeof(msg)) != 0) {
        show_error(msg);
        return -1;
    }
    return 0;
}

/* validate and save the list; on failure the previous list is put back */
static int commit(const turnout_list_t *before)
{
    char msg[1300];
    if (turnouts_validate(&tl, msg, sizeof(msg)) != 0) {
        show_error(msg);
        tl = *before;
        return -1;
    }
    turnouts_sort(&tl);
    if (turnouts_save(tpath, &tl) != 0) {
        snprintf(msg, sizeof(msg), "Écriture impossible : %s\nModification annulée.", tpath);
        show_error(msg);
        tl = *before;
        turnouts_sort(&tl);
        turnouts_page_refresh();
        return -1;
    }
    turnouts_page_refresh();
    return 0;
}

void on_btn_turnout_new_clicked(GtkButton *button, gpointer data)
{
    turnout_t t;
    turnout_list_t before = tl;
    (void)button; (void)data;

    if (read_form(&t) != 0) return;
    if (turnouts_find(&tl, t.number) >= 0) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Le numéro %u existe déjà : choisir un autre numéro, ou utiliser Éditer.", t.number);
        show_error(msg);
        return;
    }
    if (turnouts_add(&tl, &t) != 0) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Liste pleine (%d aiguillages maximum).", TURNOUT_MAX);
        show_error(msg);
        return;
    }
    if (commit(&before) == 0) {
        char msg[128];
        select_number(t.number);
        snprintf(msg, sizeof(msg), "Aiguillage %u ajouté et enregistré.", t.number);
        show_notice(msg);
    }
}

void on_btn_turnout_edit_clicked(GtkButton *button, gpointer data)
{
    turnout_t t;
    turnout_list_t before = tl;
    unsigned sel = selected_number();
    (void)button; (void)data;

    if (sel == 0) { show_info("Sélectionner d'abord un aiguillage dans la liste."); return; }
    if (read_form(&t) != 0) return;
    if (t.number != sel) {
        show_error("Le numéro d'un aiguillage ne peut pas être modifié.\n"
                   "Pour changer de numéro : ajouter un nouvel aiguillage, puis supprimer l'ancien.");
        return;
    }
    int i = turnouts_find(&tl, sel);
    if (i < 0) return;
    tl.t[i] = t;
    if (commit(&before) == 0) {
        char msg[128];
        select_number(sel);
        snprintf(msg, sizeof(msg), "Aiguillage %u modifié et enregistré.", sel);
        show_notice(msg);
    }
}

void on_btn_turnout_delete_clicked(GtkButton *button, gpointer data)
{
    turnout_list_t before = tl;
    unsigned sel = selected_number();
    (void)button; (void)data;

    if (sel == 0) { show_info("Sélectionner d'abord un aiguillage dans la liste."); return; }
    int i = turnouts_find(&tl, sel);
    if (i < 0) return;

    char msg[160];
    snprintf(msg, sizeof(msg), "Supprimer l'aiguillage %u (%s) ?", sel, tl.t[i].name);
    if (!ask_yes_no(msg)) return;

    turnouts_remove(&tl, sel);
    if (commit(&before) == 0) {
        gtk_entry_set_text(ent_number, "");
        gtk_entry_set_text(ent_name, "");
        gtk_entry_set_text(ent_comment, "");
        gtk_toggle_button_set_active(chk_inverted, FALSE);
        snprintf(msg, sizeof(msg), "Aiguillage %u supprimé et enregistré.", sel);
        show_notice(msg);
    }
}

/* a row is selected: copy it into the form */
static void on_selection_changed(GtkTreeSelection *sel, gpointer data)
{
    GtkTreeModel *model;
    GtkTreeIter it;
    (void)data;
    if (!gtk_tree_selection_get_selected(sel, &model, &it)) return;

    gchar *num, *name, *inv, *comment;
    gtk_tree_model_get(model, &it, COL_NUMBER, &num, COL_NAME, &name,
                       COL_INVERTED, &inv, COL_COMMENT, &comment, -1);
    gtk_entry_set_text(ent_number,  num     ? num     : "");
    gtk_entry_set_text(ent_name,    name    ? name    : "");
    gtk_entry_set_text(ent_comment, comment ? comment : "");
    gtk_toggle_button_set_active(chk_inverted, inv && !strcmp(inv, "oui"));
    g_free(num); g_free(name); g_free(inv); g_free(comment);
}

void turnouts_page_init(GtkBuilder *builder, const char *roster_path)
{
    store        = GTK_LIST_STORE(gtk_builder_get_object(builder, "lst_store_aiguillage"));
    tree         = GTK_TREE_VIEW(gtk_builder_get_object(builder, "point_tree_view"));
    ent_number   = GTK_ENTRY(gtk_builder_get_object(builder, "ent_turnout_number"));
    ent_name     = GTK_ENTRY(gtk_builder_get_object(builder, "ent_turnout_name"));
    ent_comment  = GTK_ENTRY(gtk_builder_get_object(builder, "ent_turnout_comment"));
    chk_inverted = GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "chk_turnout_inverted"));

    if (!store || !tree || !ent_number || !ent_name || !ent_comment || !chk_inverted) {
        fprintf(stderr, "turnouts_page_init: widget missing in roster.glade\n");
        return;
    }

    build_path(roster_path);
    if (turnouts_load(tpath, &tl) != 0) {
        char msg[1200];
        snprintf(msg, sizeof(msg), "Fichier illisible : %s", tpath);
        show_error(msg);
    } else {
        turnouts_sort(&tl);
    }
    turnouts_page_refresh();

    g_signal_connect(gtk_tree_view_get_selection(tree), "changed",
                     G_CALLBACK(on_selection_changed), NULL);
}
