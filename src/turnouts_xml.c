//============================================================================
// Name        : turnouts_xml.c
//
// purpose: see turnouts_xml.h
//
// history:  2026-11-12 first version
//==================================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "turnouts_xml.h"

static void copy_attr(xmlNodePtr n, const char *attr, char *out, size_t max)
{
    out[0] = '\0';
    xmlChar *v = xmlGetProp(n, (const xmlChar *)attr);
    if (!v) return;
    strncpy(out, (const char *)v, max - 1);
    out[max - 1] = '\0';
    xmlFree(v);
}

int turnouts_load(const char *path, turnout_list_t *l)
{
    memset(l, 0, sizeof(*l));

    FILE *f = fopen(path, "r");
    if (!f) return 0;                       /* no file yet: empty list */
    fclose(f);

    xmlDocPtr doc = xmlReadFile(path, NULL, 0);
    if (!doc) return -1;
    xmlNodePtr root = xmlDocGetRootElement(doc);
    if (!root || xmlStrcmp(root->name, (const xmlChar *)"turnouts")) { xmlFreeDoc(doc); return -1; }

    for (xmlNodePtr n = root->children; n; n = n->next) {
        if (n->type != XML_ELEMENT_NODE || xmlStrcmp(n->name, (const xmlChar *)"turnout")) continue;
        if (l->count >= TURNOUT_MAX) break;

        char num[16], inv[8];
        turnout_t *t = &l->t[l->count];
        memset(t, 0, sizeof(*t));
        copy_attr(n, "number", num, sizeof(num));
        t->number = (unsigned)strtoul(num, NULL, 10);
        copy_attr(n, "name", t->name, sizeof(t->name));
        copy_attr(n, "comment", t->comment, sizeof(t->comment));
        copy_attr(n, "inverted", inv, sizeof(inv));
        t->inverted = (!strcasecmp(inv, "true") || !strcasecmp(inv, "yes") || !strcmp(inv, "1"));
        l->count++;
    }
    xmlFreeDoc(doc);
    return 0;
}

int turnouts_save(const char *path, const turnout_list_t *l)
{
    xmlDocPtr doc = xmlNewDoc((const xmlChar *)"1.0");
    xmlNodePtr root = xmlNewNode(NULL, (const xmlChar *)"turnouts");
    xmlDocSetRootElement(doc, root);

    for (int i = 0; i < l->count; i++) {
        char num[16];
        snprintf(num, sizeof(num), "%u", l->t[i].number);
        xmlNodePtr n = xmlNewChild(root, NULL, (const xmlChar *)"turnout", NULL);
        xmlNewProp(n, (const xmlChar *)"number",   (const xmlChar *)num);
        xmlNewProp(n, (const xmlChar *)"name",     (const xmlChar *)l->t[i].name);
        xmlNewProp(n, (const xmlChar *)"comment",  (const xmlChar *)l->t[i].comment);
        xmlNewProp(n, (const xmlChar *)"inverted", (const xmlChar *)(l->t[i].inverted ? "true" : "false"));
    }

    /* write to a temporary file then rename: a failure never truncates the existing file */
    char tmp[1100];
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);
    int ok = (xmlSaveFormatFileEnc(tmp, doc, "UTF-8", 1) >= 0);
    xmlFreeDoc(doc);
    if (!ok) { remove(tmp); return -1; }
    if (rename(tmp, path) != 0) { remove(tmp); return -1; }
    return 0;
}

int turnouts_validate(const turnout_list_t *l, char *msg, size_t max)
{
    for (int i = 0; i < l->count; i++) {
        const turnout_t *t = &l->t[i];
        if (t->number < TURNOUT_NUM_MIN || t->number > TURNOUT_NUM_MAX) {
            snprintf(msg, max, "Numero %u hors limites (%d a %d).", t->number, TURNOUT_NUM_MIN, TURNOUT_NUM_MAX);
            return -1;
        }
        for (int k = i + 1; k < l->count; k++)
            if (l->t[k].number == t->number) {
                snprintf(msg, max, "Le numero %u est utilise deux fois.", t->number);
                return -1;
            }
        for (const char *p = t->name; *p; p++)
            if (*p == '"' || *p == '|' || *p == '}' || *p == '{' || *p == '[' || *p == ']' || *p == '\n') {
                snprintf(msg, max, "Nom de l'aiguillage %u : caractere interdit (%c).", t->number, *p);
                return -1;
            }
    }
    return 0;
}

int turnouts_find(const turnout_list_t *l, unsigned number)
{
    for (int i = 0; i < l->count; i++)
        if (l->t[i].number == number) return i;
    return -1;
}

int turnouts_add(turnout_list_t *l, const turnout_t *t)
{
    if (l->count >= TURNOUT_MAX || turnouts_find(l, t->number) >= 0) return -1;
    l->t[l->count++] = *t;
    return 0;
}

int turnouts_remove(turnout_list_t *l, unsigned number)
{
    int i = turnouts_find(l, number);
    if (i < 0) return -1;
    for (int k = i; k < l->count - 1; k++) l->t[k] = l->t[k + 1];
    l->count--;
    return 0;
}

static int cmp_number(const void *a, const void *b)
{
    unsigned x = ((const turnout_t *)a)->number, y = ((const turnout_t *)b)->number;
    return (x > y) - (x < y);
}

void turnouts_sort(turnout_list_t *l)
{
    qsort(l->t, (size_t)l->count, sizeof(turnout_t), cmp_number);
}
