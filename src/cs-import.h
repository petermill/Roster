/*
 * import.h
 *
 *  Created on: 7 nov. 2026
 *      Author: pierre
 */

#ifndef SRC_CS_IMPORT_H_
#define SRC_CS_IMPORT_H_
//============================================================================
// Name        : cs_import.h
//
// purpose: import of the CS loco database (database.cur) into the roster
//          (JMRI format). Without GTK: reads database.cur, builds a plan,
//          writes a report, applies the plan to an xml document.
//
// identity key : M4 loco -> UID (stored in roster as attributepairs M4_UID)
//                DCC/MM  -> (protocol, address)
//
// history:  2026-11-07 first version
//==================================================================================

#ifndef CS_IMPORT_H_
#define CS_IMPORT_H_

#include <stddef.h>
#include <libxml/tree.h>

#define CS_IMPORT_MAX        256
#define CS_IMPORT_ONLY_MAX   64
#define CS_FMT_M4            7      /* t_format: DCC14=0 DCC27=1 DCC28=2 DCC128=3 MM14=4 MM27a=5 MM27b=6 M4=7 */

typedef enum {
    IMP_OK = 0,     /* deja a jour                                   */
    IMP_UPDATE,     /* UID connu, adresse differente -> mise a jour  */
    IMP_LINK,       /* M4 de meme nom sans UID -> lier l'UID         */
    IMP_ADD,        /* absente du roster -> ajout                    */
    IMP_CONFLICT    /* a traiter a la main, jamais appliquee         */
} cs_action_t;

typedef struct {
    int      index;         /* index dans database.cur (0 = entree vide) */
    unsigned addr;
    unsigned format;
    char     name[64];
    char     uid[9];        /* 8 hexa minuscules, "" si aucun            */
} cs_loco_t;

typedef struct {
    int         count;
    cs_loco_t   loco[CS_IMPORT_MAX];
    cs_action_t action[CS_IMPORT_MAX];
    char        note[CS_IMPORT_MAX][192];
    xmlNodePtr  target[CS_IMPORT_MAX];       /* noeud du roster concerne (valide pour le doc du plan) */
    int         n_only_roster;
    char        only_roster[CS_IMPORT_ONLY_MAX][64];
} cs_import_t;

/* lit database.cur ; retourne le nombre de locos (index > 0) ou -1 si fichier illisible */
int  cs_import_read(const char *db_path, cs_import_t *imp);

/* compare avec le roster (racine <roster-config>) et remplit action[] / note[] */
void cs_import_plan(xmlNodePtr root, cs_import_t *imp);

/* texte du rapport */
void cs_import_report(const cs_import_t *imp, char *buf, size_t max);

/* nombre d'actions applicables (UPDATE + LINK + ADD) */
int  cs_import_actionable(const cs_import_t *imp);

/* applique le plan (calcule sur CE MEME doc) ; retourne le nombre de modifications */
int  cs_import_apply(xmlDocPtr doc, xmlNodePtr root, const cs_import_t *imp);

#endif /* CS_IMPORT_H_ */


#endif /* SRC_CS_IMPORT_H_ */
