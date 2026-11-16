//============================================================================
 // Name        : GTK_pointer.h
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
// history:  2022-07-14 first version
//==================================================================================

#ifndef GTK_POINTER_H_
#define GTK_POINTER_H_

/* Main */
GtkStack *Main_stack;

GtkEntry *ent_ID;
GtkEntry *ent_add;
GtkEntry *ent_prot;
GtkEntry *ent_manuf;
GtkEntry *ent_model;
GtkEntry *ent_decoder;
GtkEntry *ent_owner;
GtkEntry *ent_date;
GtkEntry *ent_road;
GtkEntry *ent_uid;

/* function */
GtkEntry *ent_f0;
GtkEntry *ent_f1;
GtkEntry *ent_f2;
GtkEntry *ent_f3;
GtkEntry *ent_f4;
GtkEntry *ent_f5;
GtkEntry *ent_f6;
GtkEntry *ent_f7;
GtkEntry *ent_f8;
GtkEntry *ent_f9;
GtkEntry *ent_f10;
GtkEntry *ent_f11;
GtkEntry *ent_f12;
GtkEntry *ent_f13;
GtkEntry *ent_f14;
GtkEntry *ent_f15;
GtkEntry *ent_f16;
GtkEntry *ent_f17;
GtkEntry *ent_f18;
GtkEntry *ent_f19;
GtkEntry *ent_f20;
GtkEntry *ent_f21;
GtkEntry *ent_f22;
GtkEntry *ent_f23;
GtkEntry *ent_f24;
GtkEntry *ent_f25;
GtkEntry *ent_f26;
GtkEntry *ent_f27;
GtkEntry *ent_f28;

GtkSwitch *switch_f0;
GtkSwitch *switch_f1;
GtkSwitch *switch_f2;
GtkSwitch *switch_f3;
GtkSwitch *switch_f4;
GtkSwitch *switch_f5;
GtkSwitch *switch_f6;
GtkSwitch *switch_f7;
GtkSwitch *switch_f8;
GtkSwitch *switch_f9;
GtkSwitch *switch_f10;
GtkSwitch *switch_f11;
GtkSwitch *switch_f12;
GtkSwitch *switch_f13;
GtkSwitch *switch_f14;
GtkSwitch *switch_f15;
GtkSwitch *switch_f16;
GtkSwitch *switch_f17;
GtkSwitch *switch_f18;
GtkSwitch *switch_f19;
GtkSwitch *switch_f20;
GtkSwitch *switch_f21;
GtkSwitch *switch_f22;
GtkSwitch *switch_f23;
GtkSwitch *switch_f24;
GtkSwitch *switch_f25;
GtkSwitch *switch_f26;
GtkSwitch *switch_f27;
GtkSwitch *switch_f28;


GtkEntry *ent_actual_loco;

/* loco_list */
//GtkTreeModel *list_store;
GtkTreeIter   iter;
GtkListStore *lst_store_loco_info;
GtkWidget *select_pt_roster;


#endif /* GTK_POINTER_H_ */
