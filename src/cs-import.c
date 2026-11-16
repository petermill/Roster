//============================================================================
// Name        : cs_import.c
//
// purpose: import of the CS loco database (database.cur) into the roster.
//          see cs_import.h
//
// history:  2026-11-07 first version
//==================================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <strings.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "cs-import.h"

#define ROSTER_MAX_NODES  512
#define UID_KEY           "M4_UID"

/* ------------------------------------------------------------------ */
/*  petites aides                                                      */
/* ------------------------------------------------------------------ */
static void trim(char *s)
{
    size_t n = strlen(s);
    while (n > 0 && isspace((unsigned char)s[n - 1])) s[--n] = '\0';
    char *p = s;
    while (*p && isspace((unsigned char)*p)) p++;
    if (p != s) memmove(s, p, strlen(p) + 1);
}

static xmlNodePtr find_child(xmlNodePtr p, const char *name)
{
    for (xmlNode *n = p->children; n; n = n->next)
        if (n->type == XML_ELEMENT_NODE && !xmlStrcmp(n->name, (const xmlChar *)name)) return n;
    return NULL;
}

/* copie le contenu texte d'un noeud, sans espaces de debut/fin */
static void node_text(xmlNodePtr n, char *out, size_t max)
{
    out[0] = '\0';
    if (!n) return;
    xmlChar *c = xmlNodeGetContent(n);
    if (c) { strncpy(out, (const char *)c, max - 1); out[max - 1] = '\0'; xmlFree(c); }
    trim(out);
}

static void get_attr(xmlNodePtr n, const char *attr, char *out, size_t max)
{
    out[0] = '\0';
    xmlChar *v = xmlGetProp(n, (const xmlChar *)attr);
    if (v) { strncpy(out, (const char *)v, max - 1); out[max - 1] = '\0'; xmlFree(v); }
    trim(out);
}

/* <protocol> sous <locomotive>, ou <locoaddress><protocol> */
static void loco_protocol(xmlNodePtr loco, char *out, size_t max)
{
    xmlNodePtr p = find_child(loco, "protocol");
    if (!p) {
        xmlNodePtr la = find_child(loco, "locoaddress");
        if (la) p = find_child(la, "protocol");
    }
    node_text(p, out, max);
}

static xmlNodePtr find_uid_pair(xmlNodePtr loco)
{
    xmlNodePtr ap = find_child(loco, "attributepairs");
    if (!ap) return NULL;
    for (xmlNode *kv = ap->children; kv; kv = kv->next) {
        if (kv->type != XML_ELEMENT_NODE || xmlStrcmp(kv->name, (const xmlChar *)"keyvaluepair")) continue;
        xmlNodePtr key = find_child(kv, "key");
        if (!key) continue;
        char k[32];
        node_text(key, k, sizeof(k));
        if (!strcmp(k, UID_KEY)) return kv;
    }
    return NULL;
}

static void get_uid(xmlNodePtr loco, char *out, size_t max)
{
    out[0] = '\0';
    xmlNodePtr kv = find_uid_pair(loco);
    if (!kv) return;
    node_text(find_child(kv, "value"), out, max);
    for (char *p = out; *p; p++) *p = (char)tolower((unsigned char)*p);
}

static void set_uid(xmlNodePtr loco, const char *uid)
{
    xmlNodePtr kv = find_uid_pair(loco);
    if (kv) {
        xmlNodePtr val = find_child(kv, "value");
        if (val) xmlNodeSetContent(val, (const xmlChar *)uid);
        else xmlNewTextChild(kv, NULL, (const xmlChar *)"value", (const xmlChar *)uid);
        return;
    }
    xmlNodePtr ap = find_child(loco, "attributepairs");
    if (!ap) ap = xmlNewChild(loco, NULL, (const xmlChar *)"attributepairs", NULL);
    kv = xmlNewChild(ap, NULL, (const xmlChar *)"keyvaluepair", NULL);
    xmlNewTextChild(kv, NULL, (const xmlChar *)"key",   (const xmlChar *)UID_KEY);
    xmlNewTextChild(kv, NULL, (const xmlChar *)"value", (const xmlChar *)uid);
}

/* protocole JMRI correspondant a un format du CS */
static const char *protocol_of(unsigned fmt, unsigned addr)
{
    if (fmt == CS_FMT_M4) return "M4";
    if (fmt <= 3)         return (addr > 127) ? "dcc_long" : "dcc_short";
    return "motorola";    /* MM : valeur a confirmer, pas de loco MM pour l'instant */
}

static int is_dcc_family(const char *proto)
{
    return !strncasecmp(proto, "dcc", 3);
}

/* ------------------------------------------------------------------ */
/*  lecture de database.cur                                            */
/*  ligne : " index 1 {2 0}{3 3}  0x5002  BR50.png {  BR50  } { ffc4184f }" */
/* ------------------------------------------------------------------ */
int cs_import_read(const char *db_path, cs_import_t *imp)
{
    memset(imp, 0, sizeof(*imp));
    FILE *f = fopen(db_path, "r");
    if (!f) return -1;

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        int index, nx, pv, n = 0;
        unsigned addr, fmt;
        /* un espace du format accepte 0 ou plusieurs blancs : "{1 4}" comme "{ 1 4 }" */
        if (sscanf(line, " index %x { %d %d } { %u %u }%n", &index, &nx, &pv, &addr, &fmt, &n) < 5 || n == 0)
            continue;
        if (index == 0) continue;                      /* entree vide "Neu?" */
        if (imp->count >= CS_IMPORT_MAX) break;

        const char *p = line + n;
        const char *q = strchr(p, '{');                /* debut du nom (le nom d'image ne contient pas '{') */
        if (!q) continue;
        q++;
        const char *r = strchr(q, '}');
        if (!r) continue;

        cs_loco_t *c = &imp->loco[imp->count];
        memset(c, 0, sizeof(*c));
        c->index  = index;
        c->addr   = addr;
        c->format = fmt;
        size_t ln = (size_t)(r - q);
        if (ln > sizeof(c->name) - 1) ln = sizeof(c->name) - 1;
        memcpy(c->name, q, ln);
        c->name[ln] = '\0';
        trim(c->name);

        const char *s = strchr(r, '{');                /* decoder_id */
        if (s) {
            s++;
            while (*s == ' ') s++;
            char u[9]; int k = 0, nonzero = 0;
            while (isxdigit((unsigned char)s[k]) && k < 8) {
                u[k] = (char)tolower((unsigned char)s[k]);
                if (u[k] != '0') nonzero = 1;
                k++;
            }
            u[k] = '\0';
            if (k == 8 && nonzero) strcpy(c->uid, u);
        }
        imp->count++;
    }
    fclose(f);
    return imp->count;
}

/* ------------------------------------------------------------------ */
/*  plan                                                               */
/* ------------------------------------------------------------------ */
static int collect_locos(xmlNodePtr root, xmlNodePtr *arr, int max)
{
    int n = 0;
    if (!root) return 0;
    for (xmlNode *r = root->children; r; r = r->next) {
        if (r->type != XML_ELEMENT_NODE || xmlStrcmp(r->name, (const xmlChar *)"roster")) continue;
        for (xmlNode *l = r->children; l && n < max; l = l->next)
            if (l->type == XML_ELEMENT_NODE && !xmlStrcmp(l->name, (const xmlChar *)"locomotive"))
                arr[n++] = l;
    }
    return n;
}

typedef struct {
    xmlNodePtr node;
    char  id[64];
    int   addr;
    char  proto[24];
    char  uid[16];
    int   matched;
} rloco_t;

/* une autre loco du roster (que 'self') utilise-t-elle deja cette adresse ? */
static rloco_t *address_used_by_other(rloco_t *r, int nr, int addr, rloco_t *self)
{
    for (int k = 0; k < nr; k++)
        if (&r[k] != self && r[k].addr == addr) return &r[k];
    return NULL;
}

void cs_import_plan(xmlNodePtr root, cs_import_t *imp)
{
    xmlNodePtr nodes[ROSTER_MAX_NODES];
    rloco_t r[ROSTER_MAX_NODES];
    int nr = collect_locos(root, nodes, ROSTER_MAX_NODES);

    for (int k = 0; k < nr; k++) {
        char a[16];
        r[k].node = nodes[k];
        get_attr(nodes[k], "id", r[k].id, sizeof(r[k].id));
        get_attr(nodes[k], "dccAddress", a, sizeof(a));
        r[k].addr = atoi(a);
        loco_protocol(nodes[k], r[k].proto, sizeof(r[k].proto));
        get_uid(nodes[k], r[k].uid, sizeof(r[k].uid));
        r[k].matched = 0;
    }
    imp->n_only_roster = 0;

    for (int i = 0; i < imp->count; i++) {
        cs_loco_t *c = &imp->loco[i];
        rloco_t *m = NULL;
        imp->note[i][0] = '\0';
        imp->target[i] = NULL;

        if (c->format == CS_FMT_M4) {
            /* 1) meme UID */
            if (c->uid[0])
                for (int k = 0; k < nr; k++)
                    if (r[k].uid[0] && !strcmp(r[k].uid, c->uid)) { m = &r[k]; break; }
            if (m) {
                m->matched = 1;
                imp->target[i] = m->node;
                if ((unsigned)m->addr != c->addr) {
                    rloco_t *o = address_used_by_other(r, nr, (int)c->addr, m);
                    if (o) {
                        imp->action[i] = IMP_CONFLICT;
                        snprintf(imp->note[i], sizeof(imp->note[i]),
                                 "adresse %u deja utilisee par '%s'", c->addr, o->id);
                    } else {
                        imp->action[i] = IMP_UPDATE;
                        snprintf(imp->note[i], sizeof(imp->note[i]),
                                 "adresse roster %d -> %u", m->addr, c->addr);
                    }
                } else {
                    imp->action[i] = IMP_OK;
                    if (strcasecmp(m->id, c->name))
                        snprintf(imp->note[i], sizeof(imp->note[i]),
                                 "nom roster '%s' / nom CS '%s' (inchange)", m->id, c->name);
                }
                continue;
            }
            /* 2) meme nom, sans UID : proposer le lien (impossible si le nom est vide) */
            if (c->name[0])
                for (int k = 0; k < nr; k++)
                    if (!strcasecmp(r[k].id, c->name)) { m = &r[k]; break; }
            if (m) {
                m->matched = 1;
                imp->target[i] = m->node;
                if (!c->uid[0]) {
                    imp->action[i] = IMP_OK;
                    snprintf(imp->note[i], sizeof(imp->note[i]), "M4 sans UID dans le CS : rien a lier");
                } else if (m->uid[0]) {
                    imp->action[i] = IMP_CONFLICT;
                    snprintf(imp->note[i], sizeof(imp->note[i]),
                             "'%s' a deja un autre UID (%s)", m->id, m->uid);
                } else {
                    rloco_t *o = (m->addr != (int)c->addr) ? address_used_by_other(r, nr, (int)c->addr, m) : NULL;
                    if (o) {
                        imp->action[i] = IMP_CONFLICT;
                        snprintf(imp->note[i], sizeof(imp->note[i]),
                                 "adresse %u deja utilisee par '%s'", c->addr, o->id);
                    } else {
                        imp->action[i] = IMP_LINK;
                        if ((unsigned)m->addr != c->addr)
                            snprintf(imp->note[i], sizeof(imp->note[i]),
                                     "lier l'UID a '%s' ; adresse %d -> %u", m->id, m->addr, c->addr);
                        else
                            snprintf(imp->note[i], sizeof(imp->note[i]), "lier l'UID a '%s'", m->id);
                    }
                }
                continue;
            }
        } else {
            /* DCC / MM : (protocole, adresse), sans UID M4 */
            int cs_dcc = (c->format <= 3);
            for (int k = 0; k < nr; k++) {
                if (r[k].uid[0]) continue;
                if (r[k].addr != (int)c->addr) continue;
                int r_dcc = is_dcc_family(r[k].proto);
                if (r_dcc == cs_dcc) { m = &r[k]; break; }
            }
            if (m) {
                m->matched = 1;
                imp->target[i] = m->node;
                imp->action[i] = IMP_OK;
                if (strcasecmp(m->id, c->name))
                    snprintf(imp->note[i], sizeof(imp->note[i]),
                             "nom roster '%s' / nom CS '%s' (inchange)", m->id, c->name);
                continue;
            }
        }

        /* 3) absente du roster : ajout, sauf nom vide, collision d'id ou d'adresse */
        if (!c->name[0]) {
            imp->action[i] = IMP_CONFLICT;
            snprintf(imp->note[i], sizeof(imp->note[i]),
                     "nom vide dans le CS : nommez la loco dans le CS avant l'import");
            continue;
        }
        rloco_t *same_id = NULL;
        for (int k = 0; k < nr; k++)
            if (!strcasecmp(r[k].id, c->name)) { same_id = &r[k]; break; }
        rloco_t *same_addr = address_used_by_other(r, nr, (int)c->addr, NULL);
        if (same_id) {
            imp->action[i] = IMP_CONFLICT;
            snprintf(imp->note[i], sizeof(imp->note[i]),
                     "l'id '%s' existe deja (autre protocole/adresse)", same_id->id);
            same_id->matched = 1;
        } else if (same_addr) {
            imp->action[i] = IMP_CONFLICT;
            snprintf(imp->note[i], sizeof(imp->note[i]),
                     "adresse %u deja utilisee par '%s'", c->addr, same_addr->id);
        } else {
            imp->action[i] = IMP_ADD;
        }
    }

    for (int k = 0; k < nr; k++)
        if (!r[k].matched && imp->n_only_roster < CS_IMPORT_ONLY_MAX) {
            strncpy(imp->only_roster[imp->n_only_roster], r[k].id, 63);
            imp->only_roster[imp->n_only_roster][63] = '\0';
            imp->n_only_roster++;
        }
}

int cs_import_actionable(const cs_import_t *imp)
{
    int n = 0;
    for (int i = 0; i < imp->count; i++)
        if (imp->action[i] == IMP_UPDATE || imp->action[i] == IMP_LINK || imp->action[i] == IMP_ADD) n++;
    return n;
}

/* ------------------------------------------------------------------ */
/*  rapport                                                            */
/* ------------------------------------------------------------------ */
void cs_import_report(const cs_import_t *imp, char *buf, size_t max)
{
    static const char *label[] = { "  =  a jour    ", "  ~  mise a jour", "  +  lier UID  ", "  +  ajout     ", "  !  CONFLIT   " };
    size_t pos = 0;
    int n_ok = 0, n_upd = 0, n_link = 0, n_add = 0, n_conf = 0;
    buf[0] = '\0';

#define APPEND(...) do { if (pos < max) { int w = snprintf(buf + pos, max - pos, __VA_ARGS__); if (w > 0) pos += (size_t)w; } } while (0)

    APPEND("Base du CS : %d locos (entree vide ignoree)\n\n", imp->count);
    for (int i = 0; i < imp->count; i++) {
        const cs_loco_t *c = &imp->loco[i];
        switch (imp->action[i]) {
            case IMP_OK:       n_ok++;   break;
            case IMP_UPDATE:   n_upd++;  break;
            case IMP_LINK:     n_link++; break;
            case IMP_ADD:      n_add++;  break;
            case IMP_CONFLICT: n_conf++; break;
        }
        APPEND("%s %-14s adr %-4u %-9s", label[imp->action[i]], c->name[0] ? c->name : "(sans nom)",
               c->addr, protocol_of(c->format, c->addr));
        if (c->uid[0]) APPEND(" UID %s", c->uid);
        if (imp->note[i][0]) APPEND("   (%s)", imp->note[i]);
        APPEND("\n");
    }

    if (imp->n_only_roster > 0) {
        APPEND("\nDans le roster seulement (aucun equivalent dans le CS, non modifie) :\n");
        for (int k = 0; k < imp->n_only_roster; k++) APPEND("     %s\n", imp->only_roster[k]);
    }

    APPEND("\nResume : %d a jour, %d adresse(s) a mettre a jour, %d UID a lier, %d ajout(s), %d conflit(s).\n",
           n_ok, n_upd, n_link, n_add, n_conf);
    if (n_conf) APPEND("Les conflits ne sont jamais appliques : a corriger a la main.\n");
#undef APPEND
}

/* ------------------------------------------------------------------ */
/*  application                                                        */
/* ------------------------------------------------------------------ */
static void set_addr(xmlNodePtr loco, unsigned addr)
{
    char a[16];
    snprintf(a, sizeof(a), "%u", addr);
    xmlSetProp(loco, (const xmlChar *)"dccAddress", (const xmlChar *)a);
}

static void add_loco(xmlNodePtr rosterNode, const cs_loco_t *c)
{
    char a[16];
    snprintf(a, sizeof(a), "%u", c->addr);

    xmlNodePtr l = xmlNewChild(rosterNode, NULL, (const xmlChar *)"locomotive", NULL);
    xmlNewProp(l, (const xmlChar *)"id",         (const xmlChar *)c->name);
    xmlNewProp(l, (const xmlChar *)"dccAddress", (const xmlChar *)a);
    xmlNewProp(l, (const xmlChar *)"mfg",        (const xmlChar *)"");
    xmlNewProp(l, (const xmlChar *)"model",      (const xmlChar *)"");
    xmlNewProp(l, (const xmlChar *)"owner",      (const xmlChar *)"");
    xmlNewProp(l, (const xmlChar *)"roadName",   (const xmlChar *)"");

    xmlNewTextChild(l, NULL, (const xmlChar *)"dateUpdated", (const xmlChar *)"?");
    xmlNodePtr d = xmlNewTextChild(l, NULL, (const xmlChar *)"decoder", (const xmlChar *)" ");
    xmlNewProp(d, (const xmlChar *)"model", (const xmlChar *)"");
    xmlNewTextChild(l, NULL, (const xmlChar *)"protocol", (const xmlChar *)protocol_of(c->format, c->addr));
    xmlNewTextChild(l, NULL, (const xmlChar *)"functionlabels", (const xmlChar *)" ");
    if (c->uid[0]) set_uid(l, c->uid);
}

int cs_import_apply(xmlDocPtr doc, xmlNodePtr root, const cs_import_t *imp)
{
    (void)doc;
    int changes = 0;
    xmlNodePtr rosterNode = NULL;
    for (xmlNode *n = root ? root->children : NULL; n; n = n->next)
        if (n->type == XML_ELEMENT_NODE && !xmlStrcmp(n->name, (const xmlChar *)"roster")) { rosterNode = n; break; }
    if (!rosterNode) return 0;

    for (int i = 0; i < imp->count; i++) {
        const cs_loco_t *c = &imp->loco[i];
        switch (imp->action[i]) {
            case IMP_UPDATE:
                if (imp->target[i]) { set_addr(imp->target[i], c->addr); changes++; }
                break;
            case IMP_LINK:
                if (imp->target[i]) { set_uid(imp->target[i], c->uid); set_addr(imp->target[i], c->addr); changes++; }
                break;
            case IMP_ADD:
                add_loco(rosterNode, c); changes++;
                break;
            default:
                break;
        }
    }
    return changes;
}
