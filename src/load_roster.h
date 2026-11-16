//============================================================================
// Name        : load_roster.h
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
// history:  2022-07-23 first version
//==================================================================================

#ifndef LOAD_ROSTER_H_
#define LOAD_ROSTER_H_

void load_roster_element(xmlNode * a_node);

xmlNode *get_root_element();

int load_roster();

#endif /* LOAD_ROSTER_H_ */
