/*
 *  Imported from tinyxml.c - Created by xant on 2/17/06.
 */
#include "bsd_queue.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef WIN32
#include <winsock2.h> // for w32lock/unlock functions
#include <io.h>
/* strings */
#if !defined snprintf
#define snprintf _snprintf
#endif

#if !defined strncasecmp
#define strncasecmp _strnicmp
#endif

#if !defined strcasecmp
#define strcasecmp _stricmp
#endif

#if !defined strdup
#define strdup _strdup
#endif

/* files */
#if !defined stat
#define stat _stat
#endif

/* time */
#if !defined sleep
#define sleep(_duration) (Sleep(_duration * 1000))
#endif

#endif // WIN32

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#ifdef USE_ICONV
#include <iconv.h>
#endif
#include <errno.h>

#include "txml.h"

#define XML_ELEMENT_NONE   0
#define XML_ELEMENT_START  1
#define XML_ELEMENT_VALUE  2
#define XML_ELEMENT_END    3
#define XML_ELEMENT_UNIQUE 4

//
// INTERNAL HELPERS
//
enum {
    ENCODING_UTF8,
    ENCODING_UTF16LE,
    ENCODING_UTF16BE,
    ENCODING_UTF32LE,
    ENCODING_UTF32BE,
    ENCODING_UTF7
} XML_ENCODING;

static int
detect_encoding(char *buffer) {
    if (buffer[0] == (char)0xef &&
        buffer[1] == (char)0xbb &&
        buffer[2] == (char)0xbf) 
    {
        return ENCODING_UTF8;
    } else if (buffer[0] == (char)0xff && 
               buffer[1] == (char)0xfe && 
               buffer[3] != (char)0x00)
    {
        return ENCODING_UTF16LE; // utf-16le
    } else if (buffer[0] == (char)0xfe && 
               buffer[1] == (char)0xff)
    {
        return ENCODING_UTF16BE; // utf-16be
    } else if (buffer[0] == (char)0xff &&
               buffer[1] == (char)0xfe &&
               buffer[2] == (char)0x00 &&
               buffer[3] == (char)0x00)
    {
        return ENCODING_UTF32LE; //utf-32le
    } else if (buffer[0] == 0 &&
               buffer[1] == 0 &&
               buffer[2] == (char)0xfe &&
               buffer[3] == (char)0xff)
    {
        return ENCODING_UTF32BE; //utf-32be
    } else if (buffer[0] == (char)0x2b &&
               buffer[1] == (char)0x2f &&
               buffer[2] == (char)0x76)
    {
        return ENCODING_UTF7;
    }
    return -1;
}

int errno;

static inline char *
dexmlize(char *string)
{
    int i, p = 0;
    int len = strlen(string);
    char *unescaped = NULL;

    if (string) {
        unescaped = (char *)calloc(1, len+1); // inlude null-byte
        for (i = 0; i < len; i++) {
            switch (string[i]) {
                case '&':
                    if (string[i+1] == '#') {
                        char *marker;
                        i+=2;
                        marker = &string[i];
                        if (string[i] >= '0' && string[i] <= '9' &&
                            string[i+1] >= '0' && string[i+1] <= '9')
                        {
                            char chr = 0;
                            i+=2;
                            if (string[i] >= '0' && string[i] <= '9' && string[i+1] == ';')
                                i++;
                            else if (string[i] == ';')
                                ; // do nothing
                            else
                                return NULL;
                            chr = (char)strtol(marker, NULL, 0);
                            unescaped[p] = chr;
                        }
                    } else if (strncmp(&string[i], "&amp;", 5) == 0) {
                        i+=4;
                        unescaped[p] = '&';
                    } else if (strncmp(&string[i], "&lt;", 4) == 0) {
                        i+=3;
                        unescaped[p] = '<';
                    } else if (strncmp(&string[i], "&gt;", 4) == 0) {
                        i+=3;
                        unescaped[p] = '>';
                    } else if (strncmp(&string[i], "&quot;", 6) == 0) {
                        i+=5;
                        unescaped[p] = '"';
                    } else if (strncmp(&string[i], "&apos;", 6) == 0) {
                        i+=5;
                        unescaped[p] = '\'';
                    } else {
                        return NULL;
                    }
                    p++;
                    break;
                default:
                    unescaped[p] = string[i];
                    p++;
            }
        }
    }
    return unescaped;
}

static inline char *
xmlize(char *string)
{
    int i, p = 0;
    int len;
    int bufsize;
    char *escaped = NULL;

    len = strlen(string);
    if (string) {
        bufsize = len+1;
        escaped = (char *)calloc(1, bufsize); // inlude null-byte
        for (i = 0; i < len; i++) {
            switch (string[i]) {
                case '&':
                    bufsize += 5;
                    escaped = realloc(escaped, bufsize);
                    memset(escaped+p, 0, bufsize-p);
                    strcpy(&escaped[p], "&amp;");
                    p += 5;
                    break;
                case '<':
                    bufsize += 4;
                    escaped = realloc(escaped, bufsize);
                    memset(escaped+p, 0, bufsize-p);
                    strcpy(&escaped[p], "&lt;");
                    p += 4;
                    break;
                case '>':
                    bufsize += 4;
                    escaped = realloc(escaped, bufsize);
                    memset(escaped+p, 0, bufsize-p);
                    strcpy(&escaped[p], "&gt;");
                    p += 4;
                    break;
                case '"':
                    bufsize += 6;
                    escaped = realloc(escaped, bufsize);
                    memset(escaped+p, 0, bufsize-p);
                    strcpy(&escaped[p], "&quot;");
                    p += 6;
                    break;
                case '\'':
                    bufsize += 6;
                    escaped = realloc(escaped, bufsize);
                    memset(escaped+p, 0, bufsize-p);
                    strcpy(&escaped[p], "&apos;");
                    p += 6;
                    break;
/*
                    bufsize += 5;
                    escaped = realloc(escaped, bufsize);
                    memset(escaped+p, 0, bufsize-p);
                    sprintf(&escaped[p], "&#%02d;", string[i]);
                    p += 5;
                    break;
*/
                default:
                    escaped[p] = string[i];
                    p++;
            }
        }
    }
    return escaped;
}

// reimplementing strcasestr since it's not present on all systems
// and we still need to be portable.
static inline
char *txml_strcasestr (char *h, char *n)
{
   char *hp, *np = n, *match = 0;
   if(!*np) {
       return NULL;
   }

   for (hp = h; *hp; hp++) {
       if (toupper(*hp) == toupper(*np)) {
           if (!match) {
               match = hp;
           }
               if(!*++np) {
                   return match;
           }
       } else {
           if (match) { 
               match = 0;
               np = n;
           }
       }
   }

   return NULL; 
}

//
// TXML IMPLEMENTATION
//

txml_t *
txml_context_create()
{
    txml_t *xml;

    xml = (txml_t *)calloc(1, sizeof(txml_t));
    xml->cnode = NULL;
    xml->ignore_white_spaces = 1; // defaults to old behaviour (all blanks are not taken into account)
    xml->ignore_blanks = 1; // defaults to old behaviour (all blanks are not taken into account)
    TAILQ_INIT(&xml->root_elements);
    xml->head = NULL;
    // default is UTF-8
    sprintf(xml->output_encoding, "utf-8");
    sprintf(xml->document_encoding, "utf-8");
    return xml;
}

void
txml_context_reset(txml_t *xml)
{
    txml_node_t *rnode, *tmp;
    TAILQ_FOREACH_SAFE(rnode, &xml->root_elements, siblings, tmp) {
        TAILQ_REMOVE(&xml->root_elements, rnode, siblings);
        txml_node_destroy(rnode);
    }
    if(xml->head)
        free(xml->head);
    xml->head = NULL;
}

txml_t *
txml_context_get(txml_node_t *node)
{
    txml_node_t *p = node;
    do {
        if (!p->parent)
            return p->context;
        p = p->parent;
    } while (p);
    return NULL; // should never arrive here
}

void
txml_document_set_encoding(txml_t *xml, char *encoding)
{
    strncpy(xml->document_encoding, encoding, sizeof(xml->document_encoding)-1);
}

void
txml_set_output_encoding(txml_t *xml, char *encoding)
{
    strncpy(xml->output_encoding, encoding, sizeof(xml->output_encoding)-1);
}

void
txml_context_destroy(txml_t *xml)
{
    txml_context_reset(xml);
    free(xml);
}

static void
txml_node_set_path(txml_node_t *node, txml_node_t *parent)
{
    unsigned int path_len;

    if (node->path)
        free(node->path);

    if(parent) {
        if(parent->path) {
            path_len = (unsigned int)strlen(parent->path)+1+strlen(node->name)+1;
            node->path = (char *)calloc(1, path_len);
            sprintf(node->path, "%s/%s", parent->path, node->name);
        } else {
            path_len = (unsigned int)strlen(parent->name)+1+strlen(node->name)+1;
            node->path = (char *)calloc(1, path_len);
            sprintf(node->path, "%s/%s", parent->name, node->name);
        }
    } else { /* root node */
        node->path = (char *)calloc(1, strlen(node->name)+2);
        sprintf(node->path, "/%s", node->name);
    }

}

txml_node_t *
txml_node_create(char *name, char *value, txml_node_t *parent)
{
    txml_node_t *node = NULL;
    node = (txml_node_t *)calloc(1, sizeof(txml_node_t));
    if(!node || !name)
        return NULL;

    TAILQ_INIT(&node->attributes);
    TAILQ_INIT(&node->children);
    TAILQ_INIT(&node->namespaces);
    TAILQ_INIT(&node->known_namespaces);

    node->name = strdup(name);

    if (parent)
        txml_node_add_child(parent, node);
    else
        txml_node_set_path(node, NULL);

    if(value && strlen(value) > 0)
        node->value = strdup(value);
    else
        node->value = (char *)calloc(1, 1);
    return node;
}

void
txml_node_destroy(txml_node_t *node)
{
    txml_attribute_t *attr, *attrtmp;
    txml_node_t *child, *childtmp;
    txml_namespace_t *ns, *nstmp;
    txml_namespace_set_t *item, *itemtmp;

    TAILQ_FOREACH_SAFE(attr, &node->attributes, list, attrtmp) {
        TAILQ_REMOVE(&node->attributes, attr, list);
        if(attr->name)
            free(attr->name);
        if(attr->value)
            free(attr->value);
        free(attr);
    }

    TAILQ_FOREACH_SAFE(child, &node->children, siblings, childtmp) {
        TAILQ_REMOVE(&node->children, child, siblings);
        txml_node_destroy(child);
    }

    TAILQ_FOREACH_SAFE(item, &node->known_namespaces, next, itemtmp) {
        TAILQ_REMOVE(&node->known_namespaces, item, next);
        free(item);
    }

    TAILQ_FOREACH_SAFE(ns, &node->namespaces, list, nstmp) {
        TAILQ_REMOVE(&node->namespaces, ns, list);
        txml_namespace_destroy(ns);
    }

    if(node->name)
        free(node->name);
    if(node->path)
        free(node->path);
    if(node->value)
        free(node->value);
    free(node);
}

txml_err_t
txml_node_set_value(txml_node_t *node, char *val)
{
    if(!val)
        return TXML_BADARGS;

    if(node->value)
        free(node->value);
    node->value = strdup(val);
    return TXML_NOERR;
}

/* quite useless */
char *
txml_node_get_value(txml_node_t *node)
{
    if(!node)
        return NULL;
    return node->value;
}

static void
txml_node_remove_child(txml_node_t *parent, txml_node_t *child)
{
    txml_node_t *p, *tmp;
    TAILQ_FOREACH_SAFE(p, &parent->children, siblings, tmp) {
        if (p == child) {
            TAILQ_REMOVE(&parent->children, p, siblings);
            p->parent = NULL;
            txml_node_set_path(p, NULL);
            break;
        }
    }
}

static inline void
txml_update_known_namespaces(txml_node_t *node)
{
    txml_namespace_t *ns;
    txml_namespace_set_t *new_item;
    
    // first empty actual list
    if (!TAILQ_EMPTY(&node->known_namespaces)) {
        txml_namespace_set_t *old_item;
        while((old_item = TAILQ_FIRST(&node->known_namespaces))) {
            TAILQ_REMOVE(&node->known_namespaces, old_item, next);
            free(old_item);
        }
    }

    // than start populating the list with actual default namespace
    if (node->cns) {
        new_item = (txml_namespace_set_t *)calloc(1, sizeof(txml_namespace_set_t));
        new_item->ns = node->cns;
        TAILQ_INSERT_TAIL(&node->known_namespaces, new_item, next);
    } else if (node->hns) {
        new_item = (txml_namespace_set_t *)calloc(1, sizeof(txml_namespace_set_t));
        new_item->ns = node->hns;
        TAILQ_INSERT_TAIL(&node->known_namespaces, new_item, next);
    }

    // add all namespaces defined by this node
    TAILQ_FOREACH(ns, &node->namespaces, list) {
        if (ns->name) { // skip an eventual default namespace since has been handled earlier
            new_item = (txml_namespace_set_t *)calloc(1, sizeof(txml_namespace_set_t));
            new_item->ns = ns;
            TAILQ_INSERT_TAIL(&node->known_namespaces, new_item, next);
        }
    }

    // and now import namespaces already valid in the scope of our parent
    if (node->parent) {
        if (!TAILQ_EMPTY(&node->parent->known_namespaces)) {
            txml_namespace_set_t *parent_item;
            TAILQ_FOREACH(parent_item, &node->parent->known_namespaces, next) {
                if (parent_item->ns->name) { // skip the default namespace
                    new_item = (txml_namespace_set_t *)calloc(1, sizeof(txml_namespace_set_t));
                    new_item->ns = parent_item->ns;
                    TAILQ_INSERT_TAIL(&node->known_namespaces, new_item, next);
                }
            }
        } else { // this shouldn't happen until known_namespaces is properly kept synchronized
            TAILQ_FOREACH(ns, &node->parent->namespaces, list) {
                if (ns->name) { // skip the default namespace
                    new_item = (txml_namespace_set_t *)calloc(1, sizeof(txml_namespace_set_t));
                    new_item->ns = ns;
                    TAILQ_INSERT_TAIL(&node->known_namespaces, new_item, next);
                }
            }
        }
    }
}

// update the hinerited namespace across a branch.
// This happens if a node (with all its childnodes) is moved across
// 2 different documents. The hinerited namespace must be updated
// accordingly to the new context, so we traverse the branches
// under the moved node to update the the hinerited namespace where
// necessary.
// NOTE: if a node defines a new default itself, it's not necessary
//       to go deeper in that same branch
static inline void
txml_update_branch_namespace(txml_node_t *node, txml_namespace_t *ns)
{
    txml_node_t *child;
    txml_namespace_set_t *nsitem;

    if (node->hns != ns && !node->cns) // skip update if not necessary
        node->hns = ns; 

    txml_update_known_namespaces(node);

    if (node->ns) { // we are bound to a specific ns.... let's see if it's known
        int missing = 1;

        TAILQ_FOREACH(nsitem, &node->known_namespaces, next) 
            if (strcmp(node->ns->uri, nsitem->ns->uri) == 0) 
                if (!(node->ns->name && !nsitem->ns->name) && strcmp(node->ns->name, nsitem->ns->name) == 0)
                    missing = 0;

        if (missing) {
            txml_namespace_t *new_ns;
            txml_namespace_set_t *newitem;
            char *newattr;

            new_ns = txml_node_add_namespace(node, node->ns->name, node->ns->uri);
            node->ns = new_ns;
            newitem = (txml_namespace_set_t *)calloc(1, sizeof(txml_namespace_set_t));
            newitem->ns = new_ns;
            TAILQ_INSERT_TAIL(&node->known_namespaces, newitem, next);
            newattr = malloc(strlen(new_ns->name)+7); // prefix + xmlns + :
            sprintf(newattr, "xmlns:%s", node->ns->name);
            // enforce the definition for our namepsace in the new context
            txml_node_add_attribute(node, newattr, node->ns->uri); 
            free(newattr);
        }
    }

    TAILQ_FOREACH(child, &node->children, siblings) // update our descendants
        txml_update_branch_namespace(child, node->cns?node->cns:node->hns); // recursion here
}

txml_err_t
txml_node_add_child(txml_node_t *parent, txml_node_t *child)
{
    if(!child)
        return TXML_BADARGS;

    // now we can update the parent
    if (child->parent)
        txml_node_remove_child(child->parent, child);

    TAILQ_INSERT_TAIL(&parent->children, child, siblings);
    child->parent = parent;

    // udate/propagate the default namespace (if any) to the newly attached node 
    // (and all its descendants)
    // Also scan for unknown namespaces defined/used in the newly attached branch
    txml_update_branch_namespace(child, parent->cns?parent->cns:parent->hns);
    txml_node_set_path(child, parent);
    return TXML_NOERR;
}

txml_node_t *
txml_node_next_sibling(txml_node_t *node)
{
    return TAILQ_NEXT(node, siblings);
}

txml_node_t *
txml_node_prev_sibling(txml_node_t *node)
{
    return TAILQ_PREV(node, nodelist_head, siblings);
}

txml_err_t
txml_add_root_node(txml_t *xml, txml_node_t *node)
{
    if(!node)
        return TXML_BADARGS;

    if (!TAILQ_EMPTY(&xml->root_elements) && !xml->allow_multiple_root_nodes) {
        return TXML_MROOT_ERR;
    }

    TAILQ_INSERT_TAIL(&xml->root_elements, node, siblings);
    node->context = xml;
    txml_update_known_namespaces(node);
    return TXML_NOERR;
}

txml_err_t
txml_node_add_attribute(txml_node_t *node, char *name, char *val)
{
    txml_attribute_t *attr;

    if(!name || !node)
        return TXML_BADARGS;

    attr = (txml_attribute_t *)calloc(1, sizeof(txml_attribute_t));
    attr->name = strdup(name);
    attr->value = val?strdup(val):strdup("");
    attr->node = node;

    TAILQ_INSERT_TAIL(&node->attributes, attr, list);
    return TXML_NOERR;
}

int
txml_node_remove_attribute(txml_node_t *node, unsigned long index)
{
    txml_attribute_t *attr, *tmp;
    int count = 0;

    TAILQ_FOREACH_SAFE(attr, &node->attributes, list, tmp) {
        if (count++ == index) {
            TAILQ_REMOVE(&node->attributes, attr, list);
            free(attr->name);
            free(attr->value);
            free(attr);
            return TXML_NOERR;
        }
    }
    return TXML_GENERIC_ERR;
}

void
txml_node_clear_attributes(txml_node_t *node)
{
    txml_attribute_t *attr, *tmp;

    TAILQ_FOREACH_SAFE(attr, &node->attributes, list, tmp) {
        TAILQ_REMOVE(&node->attributes, attr, list);
        free(attr->name);
        free(attr->value);
        free(attr);
    }
}

txml_attribute_t
*txml_node_get_attribute_byname(txml_node_t *node, char *name)
{
    txml_attribute_t *attr;
    TAILQ_FOREACH(attr, &node->attributes, list) {
        if (strcmp(attr->name, name) == 0)
            return attr;
    }
    return NULL;
}

txml_attribute_t
*txml_node_get_attribute(txml_node_t *node, unsigned long index)
{
    txml_attribute_t *attr;
    int count = 0;
    TAILQ_FOREACH(attr, &node->attributes, list) {
        if (count++ == index)
            return attr;
    }
    return NULL;
}

static txml_err_t
txml_extra_node_handler(txml_t *xml, char *content, char type)
{
    txml_node_t *new_node = NULL;
    txml_err_t res = TXML_NOERR;
    char fake_name[256];

    sprintf(fake_name, "_fakenode_%d_", type);
    new_node = txml_node_create(fake_name, content, xml->cnode);
    new_node->type = type;
    if(!new_node || !new_node->name) {
        /* XXX - ERROR MESSAGES HERE */
        res = TXML_GENERIC_ERR;
        return res;
    }
    if(xml->cnode) {
        res = txml_node_add_child(xml->cnode, new_node);
        if(res != TXML_NOERR) {
            txml_node_destroy(new_node);
            return res;
        }
    } else {
        res = txml_add_root_node(xml, new_node) ;
        if(res != TXML_NOERR) {
            txml_node_destroy(new_node);
            return res;
        }
    }
    return res;
}

static txml_err_t
txml_start_handler(txml_t *xml, char *element, char **attr_names, char **attr_values)
{
    txml_node_t *new_node = NULL;
    unsigned int offset = 0;
    txml_err_t res = TXML_NOERR;
    char *nodename = NULL;
    char *nssep = NULL;

    if(!element || strlen(element) == 0)
        return TXML_BADARGS;

    // unescape read element to be used as nodename
    nodename = dexmlize(element);
    if (!nodename)
        return TXML_BAD_CHARS;

    if ((nssep = strchr(nodename, ':'))) { // a namespace is defined
        txml_namespace_t *ns = NULL;
        *nssep = 0; // nodename now starts with the null-terminated namespace 
                    // followed by the real name (nssep + 1)
        new_node = txml_node_create(nssep+1, NULL, xml->cnode);
        if (xml->cnode)
            ns = txml_node_get_namespace_byname(xml->cnode, nodename);
        if (!ns) { 
            // TODO - Error condition
        }
        new_node->ns = ns;
    } else {
        new_node = txml_node_create(nodename, NULL, xml->cnode);
    }
    free(nodename);
    if(!new_node || !new_node->name) {
        /* XXX - ERROR MESSAGES HERE */
        return TXML_MEMORY_ERR;
    }
    /* handle attributes if present */
    if(attr_names && attr_values) {
        while(attr_names[offset] != NULL) {
            char *nsp = NULL;
            res = txml_node_add_attribute(new_node, attr_names[offset], attr_values[offset]);
            if(res != TXML_NOERR) {
                txml_node_destroy(new_node);
                return res;
            }
            if ((nsp = txml_strcasestr(attr_names[offset], "xmlns"))) {
                if ((nssep = strchr(nsp, ':'))) {  // declaration of a new namespace
                    *nssep = 0;
                    txml_node_add_namespace(new_node, nssep+1, attr_values[offset]);
                } else { // definition of the default ns
                    new_node->cns = txml_node_add_namespace(new_node, NULL, attr_values[offset]);
                }
            }
            offset++;
        }
    }
    if(xml->cnode) {
        res = txml_node_add_child(xml->cnode, new_node);
        if(res != TXML_NOERR) {
            txml_node_destroy(new_node);
            return res;
        }
    } else {
        res = txml_add_root_node(xml, new_node) ;
        if(res != TXML_NOERR) {
            txml_node_destroy(new_node);
            return res;
        }
    }
    xml->cnode = new_node;

    return res;
}

static txml_err_t
txml_end_handler(txml_t *xml, char *element)
{
    txml_node_t *parent;
    if(xml->cnode) {
        parent = xml->cnode->parent;
        xml->cnode = parent;
        return TXML_NOERR;
    }
    return TXML_GENERIC_ERR;
}

static txml_err_t
txml_value_handler(txml_t *xml, char *text)
{
    char *p;
    if(text) {
        // remove heading blanks
        if (xml->ignore_white_spaces) { // first check if we want to ignore any kind of whitespace between nodes
                                      // (which means : 'no whitespace-only values' and 'any value will be trimmed')
            while((*text == ' ' || *text == '\t' || *text == '\r' || *text == '\n') &&
                   *text != 0)
            {
                text++;
            }
        } else if (xml->ignore_blanks) { // or if perhaps we want to consider pure whitespaces: ' '
                                        // as part of the value. (but we still want to skip newlines and
                                        // tabs, which are assumed to be there to prettify the text layout
            while((*text == '\t' || *text == '\r' || *text == '\n') && 
                   *text != 0)
            {
                text++;
            }
        }

        p = text+strlen(text)-1;

        // remove trailing blanks
        if (xml->ignore_white_spaces) { // XXX - read above
            while((*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') &&
                    p != text)
            {
                *p = 0;
                p--;
            }
        } else if (xml->ignore_blanks) { // XXX = read above
            while((*p == '\t' || *p == '\r' || *p == '\n') &&
                    p != text)
            {
                *p = 0;
                p--;
            }
        }

        if(xml->cnode)  {
            char *rtext = dexmlize(text);
            if (!rtext)
                return TXML_BAD_CHARS;
            txml_node_set_value(xml->cnode, rtext);
            free(rtext);
        } else {
            fprintf(stderr, "ctag == NULL while handling a value!!");
        }
        return TXML_NOERR;
    }
    return TXML_GENERIC_ERR;
}


txml_err_t
txml_parse_buffer(txml_t *xml, char *buf)
{
    txml_err_t err = TXML_NOERR;
    int state = XML_ELEMENT_NONE;
    char *p = buf;
    unsigned int i;
    char *start = NULL;
    char *end = NULL;
    char **attrs = NULL;
    char **values = NULL;
    unsigned int nattrs = 0;
    char *mark = NULL;
    int quote = 0;

    txml_context_reset(xml); // reset the context if we are parsing a new document

    //unsigned int offset = filestat.st_size;

#define XML_FREE_ATTRIBUTES \
    if(nattrs>0) {\
        for(i = 0; i < nattrs; i++) {\
            if(attrs[i]) \
                free(attrs[i]);\
            if(values[i]) \
                free(values[i]);\
        }\
        free(attrs);\
        attrs = NULL;\
        free(values);\
        values = NULL;\
        nattrs = 0;\
    }\

// skip tabs and new-lines
#define SKIP_BLANKS(__p) \
    while((*__p == '\t' || *__p == '\r' || *__p == '\n') && *__p != 0) __p++;

// skip any kind of whitespace
#define SKIP_WHITESPACES(__p) \
    SKIP_BLANKS(__p); \
    while(*__p == ' ') {\
        __p++;\
        SKIP_BLANKS(__p);\
        if(*__p == 0) break;\
    }

#define ADVANCE_ELEMENT(__p) \
    while(*__p != '>' && *__p != ' ' && *__p != '\t' && *__p != '\r' && *__p != '\n' && *__p != 0) __p++;

#define ADVANCE_TO_ATTR_VALUE(__p) \
    while(*__p != '=' && *__p != ' ' && *__p != '\t' && *__p != '\r' && *__p != '\n' && *__p != 0) __p++;\
    SKIP_WHITESPACES(__p);

    while(*p != 0) {
        if (xml->ignore_white_spaces) {
            SKIP_WHITESPACES(p);
        } else if (xml->ignore_blanks) {
            SKIP_BLANKS(p);
        }
        if(*p == '<') { // an xml entity starts here
            p++;
            if(*p == '/') { // check if this is a closing node
                p++;
                SKIP_WHITESPACES(p);
                mark = p;
                while(*p != '>' && *p != 0)
                    p++;
                if(*p == '>') {
                    end = (char *)malloc(p-mark+1);
                    if(!end) {
                        err = TXML_MEMORY_ERR;
                        return err;
                    }
                    strncpy(end, mark, p-mark);
                    end[p-mark] = 0;
                    p++;
                    state = XML_ELEMENT_END;
                    err = txml_end_handler(xml, end);
                    free(end);
                    if(err != TXML_NOERR)
                        return err;
                }
            } else if(strncmp(p, "!ENTITY", 8) == 0) { // XXX - IGNORING !ENTITY NODES
                p += 8;
                mark = p;
                p = strstr(mark, ">");
                if(!p) {
                    fprintf(stderr, "Can't find where the !ENTITY element ends\n");
                    err = TXML_PARSER_GENERIC_ERR;
                    return err;
                }
                p++;
            } else if(strncmp(p, "!NOTATION", 9) == 0) { // XXX - IGNORING !NOTATION NODES
                p += 9;
                mark = p;
                p = strstr(mark, ">");
                if(!p) {
                    fprintf(stderr, "Can't find where the !NOTATION element ends\n");
                    err = TXML_PARSER_GENERIC_ERR;
                    return err;
                }
                p++;
            } else if(strncmp(p, "!ATTLIST", 8) == 0) { // XXX - IGNORING !ATTLIST NODES
                p += 8;
                mark = p;
                p = strstr(mark, ">");
                if(!p) {
                    fprintf(stderr, "Can't find where the !NOTATION element ends\n");
                    err = TXML_PARSER_GENERIC_ERR;
                    return err;
                }
                p++;
            } else if(strncmp(p, "!--", 3) == 0) { /* comment */
                char *comment = NULL;
                p += 3; /* skip !-- */
                mark = p;
                p = strstr(mark, "-->");
                if(!p) {
                    /* XXX - TODO - This error condition must be handled asap */
                }
                comment = (char *)calloc(1, p-mark+1);
                if(!comment) {
                    err = TXML_MEMORY_ERR;
                    return err;
                }
                strncpy(comment, mark, p-mark);
                err = txml_extra_node_handler(xml, comment, TXML_NODETYPE_COMMENT);
                free(comment);
                p+=3;
            } else if(strncmp(p, "![", 2) == 0) {
                mark = p;
                p += 2; /* skip ![ */
                SKIP_WHITESPACES(p);
                //mark = p;
                if(strncmp(p, "CDATA", 5) == 0) {
                    char *cdata = NULL;
                    p+=5;
                    SKIP_WHITESPACES(p);
                    if(*p != '[') {
                        fprintf(stderr, "Unsupported entity type at \"... -->%15s\"", mark);
                        err = TXML_PARSER_GENERIC_ERR;
                        return err;
                    }
                    mark = ++p;
                    p = strstr(mark, "]]>");
                    if(!p) {
                        /* XXX - TODO - This error condition must be handled asap */
                    }
                    cdata = (char *)calloc(1, p-mark+1);
                    if(!cdata) {
                        err = TXML_MEMORY_ERR;
                        return err;
                    }
                    strncpy(cdata, mark, p-mark);
                    err = txml_extra_node_handler(xml, cdata, TXML_NODETYPE_CDATA);
                    free(cdata);
                    p+=3;
                } else {
                    fprintf(stderr, "Unsupported entity type at \"... -->%15s\"", mark);
                    err = TXML_PARSER_GENERIC_ERR;
                    return err;
                }
            } else if(*p =='?') { /* head */
                char *encoding = NULL;
                p++;
                mark = p;
                p = strstr(mark, "?>");
                if(xml->head) // we are going to overwrite existing head (if any)
                    free(xml->head); /* XXX - should notify this behaviour? */
                xml->head = (char *)calloc(1, p-mark+1);
                strncpy(xml->head, mark, p-mark);
                encoding = strstr(xml->head, "encoding=");
                if (encoding) {
                    encoding += 9;
                    if (*encoding == '"' || *encoding == '\'') {
                        int encoding_length = 0;
                        quote = *encoding;
                        encoding++;
                        end = (char *)strchr(encoding, quote);
                        if (!end) {
                            fprintf(stderr, "Unquoted encoding string in the <?xml> section");
                            err = TXML_PARSER_GENERIC_ERR;
                            return err;
                        }
                        encoding_length = end - encoding;
                        if (encoding_length < sizeof(xml->document_encoding)) {
                            strncpy(xml->document_encoding, encoding, encoding_length);
                            // ensure to terminate it, if we are reusing a context we 
                            // could have still the old encoding there possibly with a 
                            // longer name (so poisoning the buffer)
                            xml->document_encoding[encoding_length] = 0; 
                        }
                    }
                } else {
                }
                p+=2;
            } else { /* start tag */
                attrs = NULL;
                values = NULL;
                nattrs = 0;
                state = XML_ELEMENT_START;
                SKIP_WHITESPACES(p);
                mark = p;
                ADVANCE_ELEMENT(p);
                start = (char *)malloc(p-mark+2);
                if(start == NULL)
                    return TXML_MEMORY_ERR;
                strncpy(start, mark, p-mark);

                if(*p == '>' && *(p-1) == '/') {
                    start[p-mark-1] = 0;
                    state = XML_ELEMENT_UNIQUE;
                } else {
                    start[p-mark] = 0;
                }

                SKIP_WHITESPACES(p);
                if(*p == '>' || (*p == '/' && *(p+1) == '>')) {
                    if (*p == '/') {
                        state = XML_ELEMENT_UNIQUE;
                        p++;
                    }
                }
                while(*p != '>' && *p != 0) {
                    mark = p;
                    ADVANCE_TO_ATTR_VALUE(p);
                    if(*p == '=') {
                        char *tmpattr = (char *)malloc(p-mark+1);
                        strncpy(tmpattr, mark, p-mark);
                        tmpattr[p-mark] = 0;
                        p++;
                        SKIP_WHITESPACES(p);
                        if(*p == '"' || *p == '\'') {
                            quote = *p;
                            p++;
                            mark = p;
                            while(*p != 0) {
                                if (*p == quote) {
                                    if (*(p+1) != quote) // handle quote escaping
                                        break;
                                    else
                                        p++;
                                }
                                p++;
                            }
                            if(*p == quote) {
                                char *dexmlized;
                                char *tmpval = (char *)malloc(p-mark+2);
                                int i, j=0;
                                for (i = 0; i < p-mark; i++) {
                                    if ( mark[i] == quote && mark[i+1] == mark[i] )
                                        i++;
                                    tmpval[j++] = mark[i]; 
                                }
                                tmpval[p-mark] = 0;
                                /* add new attribute */
                                nattrs++;
                                attrs = (char **)realloc(attrs, sizeof(char *)*(nattrs+1));
                                attrs[nattrs-1] = tmpattr;
                                attrs[nattrs] = NULL;
                                values = (char **)realloc(values, sizeof(char *)*(nattrs+1));
                                dexmlized = dexmlize(tmpval);
                                free(tmpval);
                                values[nattrs-1] = dexmlized;
                                values[nattrs] = NULL;
                                p++;
                                SKIP_WHITESPACES(p);
                            }
                            else {
                                free(tmpattr);
                            }
                        } /* if(*p == '"' || *p == '\'') */
                        else {
                            free(tmpattr);
                        }
                    } /* if(*p=='=') */
                    if(*p == '/' && *(p+1) == '>') {
                        p++;
                        state = XML_ELEMENT_UNIQUE;
                    }
                } /* while(*p != '>' && *p != 0) */
                err = txml_start_handler(xml, start, attrs, values);
                if(err != TXML_NOERR) {
                    XML_FREE_ATTRIBUTES
                    free(start);
                    return err;
                }
                if(state == XML_ELEMENT_UNIQUE) {
                    err = txml_end_handler(xml, start);
                    if(err != TXML_NOERR) {
                        XML_FREE_ATTRIBUTES
                        free(start);
                        return err;
                    }
                }
                XML_FREE_ATTRIBUTES
                free(start);
                p++;
            } /* end of start tag */
        } /* if(*p == '<') */
        else if(state == XML_ELEMENT_START) {
            state = XML_ELEMENT_VALUE;
            mark = p;
            while(*p != '<' && *p != 0)
                p++;
            if(*p == '<') { // p now points to the beginning of next node
                char *value = (char *)malloc(p-mark+1);
                strncpy(value, mark, p-mark);
                value[p-mark] = 0;
                err = txml_value_handler(xml, value);
                if(value)
                    free(value);
                if(err != TXML_NOERR)
                    return(err);
                //p++;
            }
        }
        else {
            /* XXX */
            p++;
        }
    } // while(*p != 0)

    return err;
}

#ifdef WIN32
//************************************************************************
// BOOL W32LockFile (FILE* filestream)
//
// locks the specific file for exclusive access, nonblocking
//
// returns 0 on success
//************************************************************************
static BOOL
W32LockFile (FILE* filestream)
{
    BOOL res = TRUE;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    unsigned long size = 0;
    int fd = 0;

    // check params
    if (!filestream)
        return FALSE;

    // get handle from stream
    fd = _fileno (filestream);
    hFile = (HANDLE)_get_osfhandle(fd);

    // lock file until access is permitted
    size = GetFileSize(hFile, NULL);
    res = LockFile (hFile, 0, 0, size, 0);
    if (res)
        res = 0;

    return res;
}

//************************************************************************
// BOOL W32UnlockFile (FILE* filestream)
//
// unlocks the specific file locked by W32LockFile
//
// returns 0 on success
//************************************************************************
static BOOL
W32UnlockFile (FILE* filestream)
{
    BOOL res = TRUE;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    unsigned long size = 0;
    int tries = 0;
    int fd = 0;

    // check params
    if (!filestream)
        return FALSE;

    // get handle from stream
    fd = _fileno (filestream);
    hFile = (HANDLE)_get_osfhandle(fd);

    // unlock
    size = GetFileSize(hFile, NULL);
    res = UnlockFile (hFile, 0, 0, size, 0);
    if (res)
        res = 0;

    return res;
}
#endif // #ifdef WIN32

static txml_err_t
txml_file_lock(FILE *file)
{
    int tries = 0;
    if(file) {
#ifdef WIN32
        while(W32LockFile(file) != 0) {
#else
        while(ftrylockfile(file) != 0) {
#endif
    // warning("can't obtain a lock on xml file %s... waiting (%d)", xml_file, tries);
            tries++;
            if(tries>5) {
                fprintf(stderr, "sticky lock on xml file!!!");
                return TXML_GENERIC_ERR;
            }
            sleep(1);
        }
        return TXML_NOERR;
    }
    return TXML_GENERIC_ERR;
}

static txml_err_t
txml_file_unlock(FILE *file)
{
    if(file) {
#ifdef WIN32
        if(W32UnlockFile(file) == 0)
#else
        funlockfile(file);

#endif
        return TXML_NOERR;
    }
    return TXML_GENERIC_ERR;
}

txml_err_t
txml_parse_file(txml_t *xml, char *path)
{
    FILE *infile;
    char *buffer;
    txml_err_t err;
    struct stat filestat;
    int rc = 0;

    infile = NULL;
    err = TXML_NOERR;
    if(!path)
        return TXML_BADARGS;
    rc = stat(path, &filestat);
    if (rc != 0)
        return TXML_BADARGS;
    xml->cnode = NULL;
    if(filestat.st_size>0) {
        infile = fopen(path, "r");
        if(infile) {
#ifdef USE_ICONV
            iconv_t ich;
            char *iconvin, *iconvout;
#endif
            size_t rb, ilen, olen;
            char *encoding_from = NULL;

            if(txml_file_lock(infile) != TXML_NOERR) {
                fprintf(stderr, "Can't lock %s for opening ", path);
                return -1;
            }
            olen = ilen = filestat.st_size;
            buffer = (char *)malloc(ilen+1);
            rb = fread(buffer, 1, ilen, infile);
            if (ilen != rb) {
                fprintf(stderr, "Can't read %s content", path);
                return -1;
            }
            buffer[ilen] = 0;
            switch(detect_encoding(buffer)) {
                case ENCODING_UTF16LE:
                    encoding_from = "UTF-16LE";
                    break;
                case ENCODING_UTF16BE:
                    encoding_from = "UTF-16BE";
                    break;
                case ENCODING_UTF32LE:
                    encoding_from = "UTF-32LE";
                    break;
                case ENCODING_UTF32BE:
                    encoding_from = "UTF-32BE";
                    break;
                case ENCODING_UTF7:
                    encoding_from = "UTF-7";
                    olen = ilen*2; // we need a bigger output buffer
                    break;
            }
            if (encoding_from) {
#ifdef USE_ICONV
                ich = iconv_open ("UTF-8", encoding_from);
                if (ich == (iconv_t)(-1)) {
                    fprintf(stderr, "Can't init iconv: %s\n", strerror(errno));
                    free(buffer);
                    txml_file_unlock(infile);
                    fclose(infile);
                    return -1;
                }
                out = (char *)calloc(1, olen);
                iconvin = buffer;
                iconvout = out;
                cb = iconv(ich, &iconvin, &ilen, &iconvout, &olen);
                if (cb == -1) {
                    fprintf(stderr, "Can't convert encoding: %s\n", strerror(errno));
                    free(buffer);
                    free(out);
                    txml_file_unlock(infile);
                    fclose(infile);
                    return -1;
                }
                free(buffer); // release initial buffer
                buffer = out; // point to the converted buffer
                iconv_close(ich);
#else
                fprintf(stderr, "Iconv missing: can't open file %s encoded in %s. Convert it to utf8 and try again\n",
                        path, encoding_from);
                free(buffer);
                txml_file_unlock(infile);
                fclose(infile);
                return -1;
#endif
            }
            err = txml_parse_buffer(xml, buffer);
            free(buffer); // release either the initial or the converted buffer
            txml_file_unlock(infile);
            fclose(infile);
        } else {
            fprintf(stderr, "Can't open xmlfile %s\n", path);
            return -1;
        }
    } else {
        fprintf(stderr, "Can't stat xmlfile %s\n", path);
        return -1;
    }
    return TXML_NOERR;
}

char *
txml_dump_branch(txml_t *xml, txml_node_t *rnode, unsigned int depth)
{
    unsigned int i, n;
    char *out = NULL;
    int out_offset = 0;
    char *start_tag;
    int start_offset = 0;
    char *end_tag;
    int end_offset = 0;
    char *child_dump;
    int child_offset = 0;
    char *value = NULL;
    int namelen = 0;
    int ns_namelen = 0;
    txml_attribute_t *attr;
    txml_node_t *child;
    unsigned long nattrs;


    if (rnode->value) {
        if (rnode->type == TXML_NODETYPE_SIMPLE) {
            value = xmlize(rnode->value);
        } else {
            value = strdup(rnode->value);
        }
    }

    if(rnode->name)
        namelen = (unsigned int)strlen(rnode->name);
    else
        return NULL;

    /* First check if this is a special node (a comment or a CDATA) */
    if(rnode->type == TXML_NODETYPE_COMMENT) {
        out = malloc(strlen(value)+depth+9);
        *out = 0;
        if (xml->ignore_blanks) {
            for(n = 0; n < depth; n++)
                out[n] = '\t';
            sprintf(out+depth, "<!--%s-->\n", value);
        } else {
            sprintf(out+depth, "<!--%s-->", value);
        }
        return out;
    } else if(rnode->type == TXML_NODETYPE_CDATA) {
        out = malloc(strlen(value)+depth+14);
        *out = 0;
        if (xml->ignore_blanks) {
            for(n = 0; n < depth; n++)
                out[n] = '\t';
            sprintf(out+depth, "<![CDATA[%s]]>\n", value);
        } else {
            sprintf(out+depth, "<![CDATA[%s]]>", value);
        }
        return out;
    }

    child_dump = (char *)calloc(1, 1);

    if (rnode->ns && rnode->ns->name)
        ns_namelen = (unsigned int)strlen(rnode->ns->name)+1;
    start_tag = (char *)calloc(1, depth+namelen+ns_namelen+7); // :/<>\n
    end_tag = (char *)calloc(1, depth+namelen+ns_namelen+7);

    if (xml->ignore_blanks) {
        for(start_offset = 0; start_offset < depth; start_offset++)
            start_tag[start_offset] = '\t';
    }
    start_tag[start_offset++] = '<';
    if (rnode->ns && rnode->ns->name) {
        // TODO - optimize
        strcpy(start_tag + start_offset, rnode->ns->name);
        start_offset += ns_namelen;
        start_tag[start_offset-1] = ':';
    }
    memcpy(start_tag + start_offset, rnode->name, namelen);
    start_offset += namelen;
    nattrs = txml_node_count_attributes(rnode);
    if(nattrs>0) {
        for(i = 0; i < nattrs; i++) {
            attr = txml_node_get_attribute(rnode, i);
            if(attr) {
                int anlen, avlen;
                char *attr_value;

                attr_value = xmlize(attr->value);
                anlen = strlen(attr->name);
                avlen = strlen (attr_value);
                start_tag = (char *)realloc(start_tag, start_offset + anlen + avlen + 8);
                sprintf(start_tag + start_offset, " %s=\"%s\"", attr->name, attr_value);
                start_offset += anlen + avlen + 4;
                if (attr_value)
                    free(attr_value);
            }
        }
    }
    if((value && *value) || !TAILQ_EMPTY(&rnode->children)) {
        if(!TAILQ_EMPTY(&rnode->children)) {
            if (xml->ignore_blanks) {
                strcpy(start_tag + start_offset, ">\n");
                start_offset += 2;
                for(end_offset = 0; end_offset < depth; end_offset++)
                    end_tag[end_offset] = '\t';
            } else {
                start_tag[start_offset++] = '>';
            }
            TAILQ_FOREACH(child, &rnode->children, siblings) {
                char *childbuff = txml_dump_branch(xml, child, depth+1); /* let's recurse */
                if(childbuff) {
                    int child_bufflen = strlen(childbuff);
                    child_dump = (char *)realloc(child_dump, child_offset+child_bufflen+1);
                    // ensure copying the null-terminating byte as well
                    memcpy(child_dump + child_offset, childbuff, child_bufflen + 1);
                    child_offset += child_bufflen;
                    free(childbuff);
                }
            }
        } else {
            // TODO - allow to specify a flag to determine if we want white spaces or not
            start_tag[start_offset++] = '>';
        }
        start_tag[start_offset] = 0; // ensure null-terminating the start-tag
        strcpy(end_tag + end_offset, "</");
        end_offset += 2;
        if (rnode->ns && rnode->ns->name) {
            // TODO - optimize
            strcpy(end_tag + end_offset, rnode->ns->name);
            end_offset += ns_namelen;
            end_tag[end_offset-1] = ':';
        }
        sprintf(end_tag + end_offset, "%s>", rnode->name);
        end_offset += namelen + 1;
        if (xml->ignore_blanks)
            end_tag[end_offset++] = '\n';
        end_tag[end_offset] = 0; // ensure null-terminating
        out = (char *)malloc(depth+strlen(start_tag)+strlen(end_tag)+
            (value?strlen(value)+1:1)+strlen(child_dump)+3);
        strcpy(out, start_tag);
        out_offset += start_offset;
        if(value && *value) { // skip also if value is an empty string (not only if it's a null pointer)
            if(!TAILQ_EMPTY(&rnode->children)) {
                if (xml->ignore_blanks) {
                    for(; out_offset < depth; out_offset++)
                        out[out_offset] = '\t';
                }
                if (value) {
                    sprintf(out + out_offset, "%s", value);
                    out_offset += strlen(value);
                    if (xml->ignore_blanks)
                        out[out_offset++] = '\n';
                }
            }
            else {
                if (value)
                    strcpy(out + out_offset, value);
                    out_offset += strlen(value);
            }
        }
        memcpy(out + out_offset, child_dump, child_offset);
        out_offset += child_offset;
        strcpy(out + out_offset, end_tag);
    }
    else {
        strcpy(start_tag + start_offset, "/>");
        start_offset += 2;
        if (xml->ignore_blanks)
            start_tag[start_offset++] = '\n';
        start_tag[start_offset] = 0;
        out = strdup(start_tag);
    }
    free(start_tag);
    free(end_tag);
    free(child_dump);
    if (value)
        free(value);
    return out;
}

char *
txml_dump(txml_t *xml, int *outlen)
{
    char *dump;
    txml_node_t *rnode;
    char *branch;
#ifdef USE_ICONV
    int do_conversion = 0;
#endif
    char head[256]; // should be enough
    int hlen;
    unsigned int offset;

    memset(head, 0, sizeof(head));
    if (xml->head) {
        int quote;
        char *start, *end, *encoding;
        char *initial = strdup(xml->head);
        start = strstr(initial, "encoding=");
        if (start) {
            *start = 0;
            encoding = start+9;
            if (*encoding == '"' || *encoding == '\'') {
                quote = *encoding;
                encoding++;
                end = (char *)strchr(encoding, quote);
                if (!end) {
                    /* TODO - Error Messages */
                } else if ((end-encoding) >= sizeof(xml->output_encoding)) {
                    /* TODO - Error Messages */
                }
                *end = 0;
                // check if document encoding matches
                if (strncasecmp(encoding, xml->document_encoding, end-encoding) != 0) {
                    /* TODO - Error Messages */
                } 
                if (strncasecmp(encoding, xml->output_encoding, end-encoding) != 0) {
#ifdef USE_ICONV
                    snprintf(head, sizeof(head), "%sencoding=\"%s\"%s",
                        initial, xml->output_encoding, ++end);
                    do_conversion = 1;
#else
                    fprintf(stderr, "Iconv missing: will not convert output to %s\n", xml->output_encoding);
                    snprintf(head, sizeof(head), "%s", xml->head);
#endif
                } else {
                    snprintf(head, sizeof(head), "%s", xml->head);
                }

            }
        } else {
#ifdef USE_ICONV
            if (xml->output_encoding && strcasecmp(xml->output_encoding, "utf-8") != 0) {
                do_conversion = 1;
                fprintf(stderr, "Iconv missing: will not convert output to %s\n", xml->output_encoding);
            }
            snprintf(head, sizeof(head), "xml version=\"1.0\" encoding=\"%s\"", 
                xml->output_encoding?xml->output_encoding:"utf-8");
#else
            if (xml->output_encoding && strcasecmp(xml->output_encoding, "utf-8") != 0) {
                fprintf(stderr, "Iconv missing: will not convert output to %s\n", xml->output_encoding);
            }
            snprintf(head, sizeof(head), "xml version=\"1.0\" encoding=\"utf-8\"");
#endif
        }
        free(initial);
    } else {
#ifdef USE_ICONV
        if (xml->output_encoding && strcasecmp(xml->output_encoding, "utf-8") != 0) {
            do_conversion = 1;
        }
        snprintf(head, sizeof(head), "xml version=\"1.0\" encoding=\"%s\"", 
            xml->output_encoding?xml->output_encoding:"utf-8");
#else
        if (xml->output_encoding && strcasecmp(xml->output_encoding, "utf-8") != 0) {
            fprintf(stderr, "Iconv missing: will not convert output to %s\n", xml->output_encoding);
        }
        snprintf(head, sizeof(head), "xml version=\"1.0\" encoding=\"utf-8\"");
#endif
    }
    hlen = strlen(head);
    dump = malloc(hlen+6);
    sprintf(dump, "<?%s?>\n", head);
    offset = hlen +5;
    TAILQ_FOREACH(rnode, &xml->root_elements, siblings) {
        branch = txml_dump_branch(xml, rnode, 0);
        if(branch) {
            int blen = strlen(branch);
            dump = (char *)realloc(dump, offset + blen + 1);
            // ensure copying the null-terminating byte as well
            memcpy(dump + offset, branch, blen + 1); 
            offset += blen;
            free(branch);
        }
    }
    if (outlen) // check if we need to report the output size
        *outlen = strlen(dump);
#ifdef USE_ICONV
    if (do_conversion) {
        iconv_t ich;
        size_t ilen, olen, cb;
        char *out;
        char *iconvin;
        char *iconvout;
        ilen = strlen(dump);
        // the most expensive conversion would be from ascii to utf-32/ucs-4
        // ( 4 bytes for each char )
        olen = ilen * 4; 
        // we still don't know how big the output buffer is going to be
        // we will update outlen later once iconv tell us the size
        if (outlen) 
            *outlen = olen;
        out = (char *)calloc(1, olen);
        ich = iconv_open (xml->output_encoding, xml->document_encoding);
        if (ich == (iconv_t)(-1)) {
            free(dump);
            free(out);
            fprintf(stderr, "Can't init iconv: %s\n", strerror(errno));
            return NULL;
        }
        iconvin = dump;
        iconvout = out;
        cb = iconv(ich, &iconvin, &ilen, &iconvout, &olen);
        if (cb == -1) {
            free(dump);
            free(out);
            fprintf(stderr, "Error from iconv: %s\n", strerror(errno));
            return NULL;
        }
        iconv_close(ich);
        free(dump); // release the old buffer (in the original encoding)
        dump = out;
        if (outlen) // update the outputsize if we have to
            *outlen -= olen;
    }
#endif
    return(dump);
}

txml_err_t
txml_save(txml_t *xml, char *xml_file)
{
    size_t rb;
    struct stat filestat;
    FILE *save_file = NULL;
    char *dump = NULL;
    int dump_len = 0;
    char *backup = NULL;
    char *backup_path = NULL;
    FILE *backup_file = NULL;


    if (stat(xml_file, &filestat) == 0) {
        if(filestat.st_size>0) { /* backup old profiles */
            save_file = fopen(xml_file, "r");
            if(!save_file) {
                fprintf(stderr, "Can't open %s for reading !!", xml_file);
                return TXML_GENERIC_ERR;
            }
            if(txml_file_lock(save_file) != TXML_NOERR) {
                fprintf(stderr, "Can't lock %s for reading ", xml_file);
                return TXML_GENERIC_ERR;
            }
            backup = (char *)malloc(filestat.st_size+1);
            rb = fread(backup, 1, filestat.st_size, save_file);
            if (rb != filestat.st_size) {
                fprintf(stderr, "Can't read %s content", xml_file);
                return -1;
            }
            backup[filestat.st_size] = 0;
            txml_file_unlock(save_file);
            fclose(save_file);
            backup_path = (char *)malloc(strlen(xml_file)+5);
            sprintf(backup_path, "%s.bck", xml_file);
            backup_file = fopen(backup_path, "w+");
            if(backup_file) {
                if(txml_file_lock(backup_file) != TXML_NOERR) {
                    fprintf(stderr, "Can't lock %s for writing ", backup_path);
                    free(backup_path);
                    free(backup);
                    return TXML_GENERIC_ERR;
                }
                fwrite(backup, 1, filestat.st_size, backup_file);
                txml_file_unlock(backup_file);
                fclose(backup_file);
            }
            else {
                fprintf(stderr, "Can't open backup file (%s) for writing! ", backup_path);
                free(backup_path);
                free(backup);
                return TXML_GENERIC_ERR;
            }
            free(backup_path);
            free(backup);
        } /* end of backup */
    }
    dump = txml_dump(xml, &dump_len);
     if(dump && dump_len) {
        save_file = fopen(xml_file, "w+");
        if(save_file) {
            if(txml_file_lock(save_file) != TXML_NOERR) {
                fprintf(stderr, "Can't lock %s for writing ", xml_file);
                free(dump);
                return TXML_GENERIC_ERR;
            }
            fwrite(dump, 1, dump_len, save_file);
            free(dump);
            txml_file_unlock(save_file);
            fclose(save_file);
        }
        else {
            fprintf(stderr, "Can't open output file %s", xml_file);
            if(!save_file) {
                free(dump);
                return TXML_GENERIC_ERR;
            }
        }
    }
    return TXML_NOERR;
}

unsigned long
txml_node_count_attributes(txml_node_t *node)
{
    txml_attribute_t *attr;
    int cnt = 0;
    TAILQ_FOREACH(attr, &node->attributes, list) 
        cnt++;
    return cnt;
}

unsigned long
txml_node_count_children(txml_node_t *node)
{
    txml_node_t *child;
    int cnt = 0; 
    TAILQ_FOREACH(child, &node->children, siblings)
        cnt++;
    return cnt;
}

unsigned long
txml_count_branches(txml_t *xml)
{
    txml_node_t *node;
    int cnt = 0;
    TAILQ_FOREACH(node, &xml->root_elements, siblings)
        cnt++;
    return cnt;
}

txml_err_t
txml_remove_node(txml_t *xml, char *path)
{
    /* XXX - UNIMPLEMENTED */
    return TXML_GENERIC_ERR;
}

txml_err_t
txml_remove_branch(txml_t *xml, unsigned long index)
{
    int count = 0;
    txml_node_t *branch, *tmp;
    TAILQ_FOREACH_SAFE(branch, &xml->root_elements, siblings, tmp) {
        if (count++ == index) {
            TAILQ_REMOVE(&xml->root_elements, branch, siblings);
            txml_node_destroy(branch);
            return TXML_NOERR;
        }
    }
    return TXML_GENERIC_ERR;
}

txml_node_t
*txml_node_get_child(txml_node_t *node, unsigned long index)
{
    txml_node_t *child;
    int count = 0;
    if(!node)
        return NULL;
    TAILQ_FOREACH(child, &node->children, siblings) {
        if (count++ == index) {
            return child;
            break;
        }
    }
    return NULL;
}

/* XXX - if multiple children shares the same name, only the first is returned */
txml_node_t
*txml_node_get_child_byname(txml_node_t *node, char *name)
{
    txml_node_t *child;
    unsigned int i = 0;
    char *attr_name = NULL;
    char *attr_val = NULL;
    char *node_name = NULL;
    int name_len = 0;
    char *p;

    if(!node)
        return NULL;

    node_name = strdup(name); // make a copy to avoid changing the provided buffer
    name_len = strlen(node_name);

    if (node_name[name_len-1] == ']') {
        p = strchr(node_name, '[');
        *p = 0;
        p++;
        if (sscanf(p, "%d]", &i) == 1) {
            i--;
        } else if (*p == '@') {
            p++;
            p[strlen(p)-1] = 0;
            attr_name = p;
            attr_val = strchr(p, '=');
            if (attr_val) {
                *attr_val = 0;
                attr_val++;
                if (*attr_val == '\'' || *attr_val == '"') {
                    char quote = *attr_val;
                    int n, j=0;
                    // inplace dequoting
                    attr_val++;
                    for (n = 0; attr_val[n] != 0; n++) {
                        if (attr_val[n] == quote) {
                            if (n && attr_val[n-1] == quote) { // quote escaping (XXX - perhaps out of spec)
                                if (j)
                                    j--;
                            } else {
                                attr_val[n] = 0;
                                break;
                            }
                        }
                        if (j != n)
                            attr_val[j] = attr_val[n];
                        j++;
                    }

                }
            }
        }
    }

    TAILQ_FOREACH(child, &node->children, siblings) {
        if(strcmp(child->name, node_name) == 0) {
            if (attr_name) {
                txml_attribute_t *attr = txml_node_get_attribute_byname(child, attr_name);
                if (attr) {
                    if (attr_val) {
                        char *dexmlized = dexmlize(attr_val);
                        if (strcmp(attr->value, dexmlized) != 0) {
                            free(dexmlized);
                            continue; // the attr value doesn't match .. let's skip to next matching node
                        }
                        free(dexmlized);
                    }
                    free(node_name);
                    return child;
                }
            } else if (i == 0) {
                free(node_name);
                return child;
            } else {
                i--;
            }
        }
    }
    free(node_name);
    return NULL;
}

txml_node_t *
txml_get_node(txml_t *xml, char *path)
{
    char *buff, *walk;
    char *tag;
    unsigned long i = 0;
    txml_node_t *cnode = NULL;
    txml_node_t *wnode = NULL;
//#ifndef WIN32
    char *brkb;
//#endif
    if(!path)
        return NULL;

    buff = strdup(path);
    walk = buff;

    // check if we are allowing multiple rootnodes to determine
    // if it's included in the path or not
    if (xml->allow_multiple_root_nodes) {
        /* skip leading slashes '/' */
        while(*walk == '/')
            walk++;

        /* select the root node */
#ifndef WIN32
        tag  = strtok_r(walk, "/", &brkb);
#else
        tag = strtok(walk, "/");
#endif
        if(!tag) {
            free(buff);
            return NULL;
        }

        for(i = 0; i < txml_count_branches(xml); i++) {
            wnode = txml_get_branch(xml, i);
            if(strcmp(wnode->name, tag) == 0) {
                cnode = wnode;
                break;
            }
        }
        /* now cnode points to the root node ... let's find requested node */
#ifndef WIN32
        tag = strtok_r(NULL, "/", &brkb);
#else
        tag = strtok(NULL, "/");
#endif
    } else { // no multiple rootnodes
        cnode = txml_get_branch(xml, 0);
        // TODO - this could be done in a cleaner and more efficient way
        if (*walk != '/') {
            buff = malloc(strlen(walk)+2);
            sprintf(buff, "/%s", walk);
            free(walk);
            walk = buff;
        }
#ifndef WIN32
        tag = strtok_r(walk, "/", &brkb);
#else
        tag = strtok(walk, "/");
#endif
    }

    if(!cnode) {
        free(buff);
        return NULL;
    }

    while(tag) {
        wnode = txml_node_get_child_byname(cnode, tag);
        if(!wnode) {
            free(buff);
            return NULL;
        }
        cnode = wnode; // update current node
#ifndef WIN32
        tag = strtok_r(NULL, "/", &brkb);
#else
        tag = strtok(NULL, "/");
#endif
    }

    free(buff);
    return cnode;
}

txml_node_t
*txml_get_branch(txml_t *xml, unsigned long index)
{
    txml_node_t *node;
    int cnt = 0;
    if(!xml)
        return NULL;
    TAILQ_FOREACH(node, &xml->root_elements, siblings) {
        if (cnt++ == index)
            return node;
    }
    return NULL;
}

txml_err_t
txml_subst_branch(txml_t *xml, unsigned long index, txml_node_t *new_branch)
{
    txml_node_t *branch, *tmp;
    int cnt = 0;
    TAILQ_FOREACH_SAFE(branch, &xml->root_elements, siblings, tmp) {
        if (cnt++ == index) {
            TAILQ_INSERT_BEFORE(branch, new_branch, siblings);
            TAILQ_REMOVE(&xml->root_elements, branch, siblings);
            return TXML_NOERR;
        }
    }
    return TXML_LINKLIST_ERR;
}

txml_namespace_t *
txml_namespace_create(char *ns_name, char *ns_uri) {
    txml_namespace_t *new_ns;
    new_ns = (txml_namespace_t *)calloc(1, sizeof(txml_namespace_t));
    if (ns_name)
        new_ns->name = strdup(ns_name);
    new_ns->uri = strdup(ns_uri);
    return new_ns;
}

void
txml_namespace_destroy(txml_namespace_t *ns)
{
    if (ns) {
        if (ns->name)
            free(ns->name);
        if (ns->uri)
            free(ns->uri);
        free(ns);
    }
}

txml_namespace_t *
txml_node_add_namespace(txml_node_t *node, char *ns_name, char *ns_uri) {
    txml_namespace_t *new_ns = NULL;
    if (!node || !ns_uri)
        return NULL;

    if ((new_ns = txml_namespace_create(ns_name, ns_uri)))
        TAILQ_INSERT_TAIL(&node->namespaces, new_ns, list);
    return new_ns;
}

txml_namespace_t *
txml_node_get_namespace_byname(txml_node_t *node, char *ns_name) {
    txml_namespace_set_t *item;
    // TODO - check if node->known_namespaces needs to be updated
    TAILQ_FOREACH(item, &node->known_namespaces, next) {
        if (item->ns->name && strcmp(item->ns->name, ns_name) == 0)
            return item->ns;
    }
    return NULL;
}

txml_namespace_t *
txml_node_get_namespace_byuri(txml_node_t *node, char *ns_uri) {
    txml_namespace_set_t *item;
    // TODO - check if node->known_namespaces needs to be updated
    TAILQ_FOREACH(item, &node->known_namespaces, next) {
        if (strcmp(item->ns->uri, ns_uri) == 0)
            return item->ns;
    }
    return NULL;
}

txml_namespace_t *
txml_node_get_namespace(txml_node_t *node) {
    txml_node_t *p = node->parent;
    if (node->ns) // my namespace
        return node->ns;
    if (node->hns) // hinerited namespace
        return node->hns;
    // search for a default naspace defined in our hierarchy
    // this should happen only if a node has been moved across 
    // multiple documents and it's hinerited namespace has been lost
    while (p) { 
        if (p->cns)
            return p->cns;
        p = p->parent;
    }
    return NULL;
}

txml_err_t
txml_node_set_cnamespace(txml_node_t *node, txml_namespace_t *ns) {
    if (!node || !ns)
        return TXML_BADARGS;
    
    node->cns = ns;
    return TXML_NOERR;
}

txml_err_t
txml_node_set_namespace(txml_node_t *node, txml_namespace_t *ns) {
    if (!node || !ns)
        return TXML_BADARGS;
    
    node->ns = ns;
    return TXML_NOERR;
}

int
txml_has_iconv()
{
#ifdef USE_ICONV
    return 1;
#else
    return 0;
#endif
}

