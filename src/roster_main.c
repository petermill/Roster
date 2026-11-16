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


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pthread.h>
#include <gtk/gtk.h>
#include <gtk/gtkmain.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "GTK_pointer.h"


char *roster = "../../roster.xml";


GtkEntry *list_func_entry[29];
GtkSwitch *list_func_switch[29];

void init()
{
	list_func_entry[0]  = ent_f0; list_func_entry[1]  = ent_f1;  list_func_entry[2]  = ent_f2;
	list_func_entry[3]  = ent_f3; list_func_entry[4]  = ent_f4;  list_func_entry[5]  = ent_f5;
	list_func_entry[6]  = ent_f6; list_func_entry[7]  = ent_f7;  list_func_entry[8]  = ent_f8;
	list_func_entry[9]  = ent_f9; list_func_entry[10]  = ent_f10;list_func_entry[11] = ent_f11;
	list_func_entry[12] = ent_f12; list_func_entry[13] = ent_f13;list_func_entry[14] = ent_f14;
	list_func_entry[15] = ent_f15; list_func_entry[16] = ent_f16;list_func_entry[17] = ent_f17;
	list_func_entry[18] = ent_f18; list_func_entry[19] = ent_f19;list_func_entry[20] = ent_f20;
	list_func_entry[21] = ent_f21; list_func_entry[22] = ent_f22;list_func_entry[23] = ent_f23;
	list_func_entry[24] = ent_f24; list_func_entry[25] = ent_f25;list_func_entry[26] = ent_f26;
	list_func_entry[27] = ent_f27; list_func_entry[28] = ent_f28 ;

	list_func_switch[0]  = switch_f0; list_func_switch[1]  = switch_f1;  list_func_switch[2]  = switch_f2;
	list_func_switch[3]  = switch_f3; list_func_switch[4]  = switch_f4;  list_func_switch[5]  = switch_f5;
	list_func_switch[6]  = switch_f6; list_func_switch[7]  = switch_f7;  list_func_switch[8]  = switch_f8;
	list_func_switch[9]  = switch_f9; list_func_switch[10]  = switch_f10;list_func_switch[11] = switch_f11;
	list_func_switch[12] = switch_f12; list_func_switch[13] = switch_f13;list_func_switch[14] = switch_f14;
	list_func_switch[15] = switch_f15; list_func_switch[16] = switch_f16;list_func_switch[17] = switch_f17;
	list_func_switch[18] = switch_f18; list_func_switch[19] = switch_f19;list_func_switch[20] = switch_f20;
	list_func_switch[21] = switch_f21; list_func_switch[22] = switch_f22;list_func_switch[23] = switch_f23;
	list_func_switch[24] = switch_f24;  list_func_switch[25] = switch_f25;list_func_switch[26] = switch_f26;
	list_func_switch[27] = switch_f27; list_func_switch[28] = switch_f28 ;
}

int main( int argc, char **argv )
{
	/* Init GTK+ */
	gtk_init( &argc, &argv );

	  GtkWidget *window;
	  GtkBuilder *builder;
	  GError     *error = NULL;

	  /* Create new GtkBuilder object */
	 builder = gtk_builder_new();

	 /* Load UI from file. If error occurs, report it and quit application.*/

	if( ! gtk_builder_add_from_file( builder, "roster.glade", &error ))
		 {
			 g_warning( "%s", error->message );
			 g_free( error );
			 return( 1 );
		 }
	 gtk_builder_new_from_file(  "roster.glade" );

	 /* Get main window pointer from UI */
	window = GTK_WIDGET( gtk_builder_get_object( builder, "GTK_WINDOW_TOPLEVEL" ));
	Main_stack = GTK_STACK( gtk_builder_get_object( builder, "Main_stack" ));

	/* get loco_info*/
		 lst_store_loco_info = GTK_LIST_STORE ( gtk_builder_get_object( builder, "lst_store_loco_infos" ));
		 ent_ID = GTK_ENTRY( gtk_builder_get_object( builder, "entry_ID"));
		 ent_add = GTK_ENTRY( gtk_builder_get_object( builder, "entry_add"));
		 ent_prot = GTK_ENTRY( gtk_builder_get_object( builder, "entry_protocol"));
		 ent_manuf = GTK_ENTRY( gtk_builder_get_object( builder, "entry_manuf"));
		 ent_model = GTK_ENTRY( gtk_builder_get_object( builder, "entry_model"));
		 ent_decoder = GTK_ENTRY( gtk_builder_get_object( builder, "entry_decoder"));
		 ent_owner = GTK_ENTRY( gtk_builder_get_object( builder, "entry_owner"));
		 ent_date = GTK_ENTRY( gtk_builder_get_object( builder, "entry_date"));
		 ent_road = GTK_ENTRY( gtk_builder_get_object( builder, "entry_road"));
		 ent_uid = GTK_ENTRY( gtk_builder_get_object( builder, "entry_uid"));

		 /* function  */
		 ent_actual_loco = GTK_ENTRY( gtk_builder_get_object( builder, "ent_actual_loco"));
		 ent_f0 = GTK_ENTRY( gtk_builder_get_object( builder, "ent_f0"));
		 ent_f1 = GTK_ENTRY( gtk_builder_get_object( builder, "ent_f1"));
		 ent_f2 = GTK_ENTRY( gtk_builder_get_object( builder, "ent_f2"));
		 ent_f3 = GTK_ENTRY( gtk_builder_get_object( builder, "ent_f3"));
		 ent_f4 = GTK_ENTRY( gtk_builder_get_object( builder, "ent_f4"));
		 ent_f5 = GTK_ENTRY( gtk_builder_get_object( builder, "ent_f5"));
		 ent_f6 = GTK_ENTRY( gtk_builder_get_object( builder, "ent_f6"));
		 ent_f7 = GTK_ENTRY( gtk_builder_get_object( builder, "ent_f7"));
		 ent_f8 = GTK_ENTRY( gtk_builder_get_object( builder, "ent_f8"));
		 ent_f9 = GTK_ENTRY( gtk_builder_get_object( builder, "ent_f9"));
		 ent_f10 = GTK_ENTRY( gtk_builder_get_object( builder, "ent_f10"));
		 ent_f11 = GTK_ENTRY( gtk_builder_get_object( builder, "ent_f11"));
		 ent_f12 = GTK_ENTRY( gtk_builder_get_object( builder, "ent_f12"));
		 ent_f13 = GTK_ENTRY( gtk_builder_get_object( builder, "ent_f13"));
		 ent_f14 = GTK_ENTRY( gtk_builder_get_object( builder, "ent_f14"));
		 ent_f15 = GTK_ENTRY( gtk_builder_get_object( builder, "ent_f15"));
		 ent_f16 = GTK_ENTRY( gtk_builder_get_object( builder, "ent_f16"));
		 ent_f17 = GTK_ENTRY( gtk_builder_get_object( builder, "ent_f17"));
		 ent_f18 = GTK_ENTRY( gtk_builder_get_object( builder, "ent_f18"));
		 ent_f19 = GTK_ENTRY( gtk_builder_get_object( builder, "ent_f19"));
		 ent_f20 = GTK_ENTRY( gtk_builder_get_object( builder, "ent_f20"));
		 ent_f21 = GTK_ENTRY( gtk_builder_get_object( builder, "ent_f21"));
		 ent_f22 = GTK_ENTRY( gtk_builder_get_object( builder, "ent_f22"));
		 ent_f23 = GTK_ENTRY( gtk_builder_get_object( builder, "ent_f23"));
		 ent_f24 = GTK_ENTRY( gtk_builder_get_object( builder, "ent_f24"));
		 ent_f25 = GTK_ENTRY( gtk_builder_get_object( builder, "ent_f25"));
		 ent_f26 = GTK_ENTRY( gtk_builder_get_object( builder, "ent_f26"));
		 ent_f27 = GTK_ENTRY( gtk_builder_get_object( builder, "ent_f27"));
		 ent_f28 = GTK_ENTRY( gtk_builder_get_object( builder, "ent_f28"));

		 switch_f0 = GTK_SWITCH( gtk_builder_get_object( builder, "switch_f0"));
		 switch_f1 = GTK_SWITCH( gtk_builder_get_object( builder, "switch_f1"));
		 switch_f2 = GTK_SWITCH( gtk_builder_get_object( builder, "switch_f2"));
		 switch_f3 = GTK_SWITCH( gtk_builder_get_object( builder, "switch_f3"));
		 switch_f4 = GTK_SWITCH( gtk_builder_get_object( builder, "switch_f4"));
		 switch_f5 = GTK_SWITCH( gtk_builder_get_object( builder, "switch_f5"));
		 switch_f6 = GTK_SWITCH( gtk_builder_get_object( builder, "switch_f6"));
		 switch_f7 = GTK_SWITCH( gtk_builder_get_object( builder, "switch_f7"));
		 switch_f8 = GTK_SWITCH( gtk_builder_get_object( builder, "switch_f8"));
		 switch_f9 = GTK_SWITCH( gtk_builder_get_object( builder, "switch_f9"));
		 switch_f10 = GTK_SWITCH( gtk_builder_get_object( builder, "switch_f10"));
		 switch_f11 = GTK_SWITCH( gtk_builder_get_object( builder, "switch_f11"));
		 switch_f12 = GTK_SWITCH( gtk_builder_get_object( builder, "switch_f12"));
		 switch_f13 = GTK_SWITCH( gtk_builder_get_object( builder, "switch_f13"));
		 switch_f14 = GTK_SWITCH( gtk_builder_get_object( builder, "switch_f14"));
		 switch_f15 = GTK_SWITCH( gtk_builder_get_object( builder, "switch_f15"));
		 switch_f16 = GTK_SWITCH( gtk_builder_get_object( builder, "switch_f16"));
		 switch_f17 = GTK_SWITCH( gtk_builder_get_object( builder, "switch_f17"));
		 switch_f18 = GTK_SWITCH( gtk_builder_get_object( builder, "switch_f18"));
		 switch_f19 = GTK_SWITCH( gtk_builder_get_object( builder, "switch_f19"));
		 switch_f20 = GTK_SWITCH( gtk_builder_get_object( builder, "switch_f20"));
		 switch_f21 = GTK_SWITCH( gtk_builder_get_object( builder, "switch_f21"));
		 switch_f22 = GTK_SWITCH( gtk_builder_get_object( builder, "switch_f22"));
		 switch_f23 = GTK_SWITCH( gtk_builder_get_object( builder, "switch_f23"));
		 switch_f24 = GTK_SWITCH( gtk_builder_get_object( builder, "switch_f24"));
		 switch_f25 = GTK_SWITCH( gtk_builder_get_object( builder, "switch_f25"));
		 switch_f26 = GTK_SWITCH( gtk_builder_get_object( builder, "switch_f26"));
		 switch_f27 = GTK_SWITCH( gtk_builder_get_object( builder, "switch_f27"));
		 switch_f28 = GTK_SWITCH( gtk_builder_get_object( builder, "switch_f28"));

		 /* Connect signals */
		 gtk_builder_connect_signals( builder, NULL );

		 /* page Aiguillages  */
		 turnouts_page_init( builder, roster );

	  /* Destroy builder, since we don't need it anymore */
	  g_object_unref( G_OBJECT( builder ) );

	  gtk_widget_show_all(window);

	  init();

	 /* Start main loop */
	  gtk_main ();

	return EXIT_SUCCESS;
}
