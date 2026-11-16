//============================================================================
// Name        : load_function.h
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

#ifndef LOAD_FUNCTION_H_
#define LOAD_FUNCTION_H_

void Clean_previous_function_entry();
void Clean_parameters_pan();
int load_function();

#endif /* LOAD_FUNCTION_H */
