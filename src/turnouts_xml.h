//============================================================================
// Name        : turnouts_xml.h
//
// purpose: list of turnouts for Engine Driver (WiThrottle PTL), stored in
//          turnouts.xml next to roster.xml. No GTK: libxml2 only.
//
// file format:
//   <?xml version="1.0" encoding="UTF-8"?>
//   <turnouts>
//     <turnout number="21" name="VI-1" comment="" inverted="false"/>
//   </turnouts>
//
//   number   : turnout number as shown in Engine Driver, system name LT<number>.
//              The address sent on the bus is number - 1 (TURNOUT_ADDR_OFFSET).
//   name     : user name, second field of the WiThrottle PTL entry
//   comment  : free note, never sent to the Pico
//   inverted : swaps thrown / closed for a turnout wired the other way round
//
// history:  2026-11-12 first version
//==================================================================================

#ifndef TURNOUTS_XML_H_
#define TURNOUTS_XML_H_

#include <stddef.h>

#define TURNOUT_MAX         32
#define TURNOUT_NUM_MIN     1
#define TURNOUT_NUM_MAX     2048     /* DCC accessory addresses 1..2048 */
#define TURNOUT_NAME_LEN    32
#define TURNOUT_COMMENT_LEN 64

typedef struct {
    unsigned number;
    char     name[TURNOUT_NAME_LEN];
    char     comment[TURNOUT_COMMENT_LEN];
    int      inverted;
} turnout_t;

typedef struct {
    int       count;
    turnout_t t[TURNOUT_MAX];
} turnout_list_t;

/* 0 = ok (a missing file gives an empty list), -1 = unreadable or not a turnouts file */
int  turnouts_load(const char *path, turnout_list_t *l);

/* 0 = ok, -1 = write error */
int  turnouts_save(const char *path, const turnout_list_t *l);

/* 0 = ok, otherwise -1 and a message (french) in msg */
int  turnouts_validate(const turnout_list_t *l, char *msg, size_t max);

/* index of the turnout with this number, or -1 */
int  turnouts_find(const turnout_list_t *l, unsigned number);

/* add (-1 if full or number already used) / remove (-1 if absent) / sort by number */
int  turnouts_add(turnout_list_t *l, const turnout_t *t);
int  turnouts_remove(turnout_list_t *l, unsigned number);
void turnouts_sort(turnout_list_t *l);

#endif /* TURNOUTS_XML_H_ */
