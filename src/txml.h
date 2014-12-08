/*
 *  Imported from tinyxml.h - Created by xant on 2/17/06.
 */

#ifndef __TINYXML_H__
#define __TINYXML_H__

#ifdef __cplusplus
extern "C" {
#endif

#define txml_err_t int
#define TXML_NOERR 0
#define TXML_GENERIC_ERR -1
#define TXML_BADARGS -2
#define TXML_UPDATE_ERR -2
#define TXML_OPEN_FILE_ERR -3
#define TXML_PARSER_GENERIC_ERR -4
#define TXML_MEMORY_ERR -5
#define TXML_LINKLIST_ERR -6
#define TXML_BAD_CHARS -7
#define TXML_MROOT_ERR -8

struct __txml_node_s;
struct __txml_s;

typedef struct __txml_namespace_s {
    char *name;
    char *uri;
    TAILQ_ENTRY(__txml_namespace_s) list;
} txml_namespace_t;

/**
    @brief One attribute associated to an element 
*/
typedef struct __txml_attribute_s {
    char *name; ///< the attribute name
    char *value; ///< the attribute value
    struct __txml_node_s *node;
    TAILQ_ENTRY(__txml_attribute_s) list;
} txml_attribute_t;

typedef struct __txml_namespace_set_s {
    txml_namespace_t *ns;
    TAILQ_ENTRY(__txml_namespace_set_s) next;
} txml_namespace_set_t;

typedef struct __txml_node_s {
    char *path;
    char *name;
    struct __txml_node_s *parent;
    char *value;
    TAILQ_HEAD(,__txml_node_s) children;
    TAILQ_HEAD(,__txml_attribute_s) attributes;
#define TXML_NODETYPE_SIMPLE 0
#define TXML_NODETYPE_COMMENT 1
#define TXML_NODETYPE_CDATA 2
    char type;
    txml_namespace_t *ns;  // namespace of this node (if any)
    txml_namespace_t *cns; // new default namespace defined by this node
    txml_namespace_t *hns; // hinerited namespace (if any)
    // all namespaces valid in this scope ( implicit namespaces )
    TAILQ_HEAD(,__txml_namespace_set_s) known_namespaces; 
    // storage for newly defined namespaces 
    // (needed keep track of allocated txml_namespace_t structures for later release)
    TAILQ_HEAD(,__txml_namespace_s) namespaces; 
    TAILQ_ENTRY(__txml_node_s) siblings;
    struct __txml_s *context; // set only if rootnode (otherwise it's always NULL)
} txml_node_t;

TAILQ_HEAD(nodelist_head, __txml_node_s);

typedef struct __txml_s {
    txml_node_t *cnode;
    TAILQ_HEAD(,__txml_node_s) root_elements;
    char *head;
    char output_encoding[64];  /* XXX probably oversized, 24 or 32 should be enough */
    char document_encoding[64];
    int use_namespaces;
    int allow_multiple_root_nodes;
    int ignore_white_spaces;
    int ignore_blanks;
} txml_t;

/***
    @brief Create a new xml context
    @return a point to a valid xml context
*/

txml_t *txml_context_create();

/***
    @brief Resets/cleans an existing context
    @arg pointer to a valid xml context
*/
void txml_context_reset(txml_t *xml);

/***
    @brief release all resources associated to an xml context
    @arg pointer to a valid xml context
*/
void txml_context_destroy(txml_t *xml);

/***
    @brief parse a string buffer containing an xml profile and fills internal structures appropriately
    @arg the null terminated string buffer containing the xml profile
    @return true if buffer is parsed successfully , false otherwise)
*/
txml_err_t txml_parse_buffer(txml_t *xml,char *buf);

/***
    @brief parse an xml file containing the profile and fills internal structures appropriately
    @arg a null terminating string representing the path to the xml file
    @return an txml_err_t error status (XML_NOERR if buffer was parsed successfully)
*/
txml_err_t txml_parse_file(txml_t *xml,char *path);

/***
    @brief dump the entire xml configuration tree that reflects the status of internal structures
    @arg pointer to a valid xml context
    @arg if not NULL, here will be stored the bytelength of the returned buffer
    @return a null terminated string containing the xml representation of the configuration tree.
    The memory allocated for the dump-string must be freed by the user when no more needed
*/
char *txml_dump(txml_t *xml, int *outlen);

void txml_set_output_encoding(txml_t *xml, char *encoding);
/***
    @brief allocates memory for an txml_node_t. In case of errors NULL is returned 
    @arg name of the new node
    @arg value associated to the new node (can be NULL and specified later through XmlSetNodeValue function)
    @arg parent of the new node if present, NULL if this will be a root node
    @return the newly created node 
 */

/***
    @brief dump the entire xml branch starting from a given node
    @arg pointer to a valid xml context
    @arg pointer to a valid xml node
    @arg *TODO: DOCUMENT*
    @return a null terminated string containing the xml representation of the configuration tree.
    The memory allocated for the dump-string must be freed by the user when no more needed
*/
char *txml_dump_branch(txml_t *xml,txml_node_t *node,unsigned int depth);

/***
    @brief Makes txml_node_t *node a root node in context represented by txml_t *xml 
    @arg the xml context pointer
    @arg the new root node
    @return XML_NOERR on success, error code otherwise
 */
txml_err_t txml_add_root_node(txml_t *xml,txml_node_t *node);

txml_node_t *txml_node_create(char *name,char *val,txml_node_t *parent);

/*** 
    @brief associate a value to txml_node_t *node. XML_NOERR is returned if no error occurs 
    @arg the node we want to modify
    @arg the value we want to set for node
    @return XML_NOERR if success , error code otherwise
 */
txml_err_t txml_node_set_value(txml_node_t *node,char *val);

/***
    @brief get value for an txml_node_t
    @arg the txml_node_t containing the value we want to access.
    @return returns value associated to txml_node_t *node 
 */
char *txml_node_get_value(txml_node_t *node);

/****
    @brief free resources for txml_node_t *node and all its subnodes 
    @arg the txml_node_t we want to destroy
 */
void txml_node_destroy(txml_node_t *node);

/*** 
    @brief Adds txml_node_t *child to the children list of txml_node_t *node 
    @arg the parent node
    @arg the new child
    @return return XML_NOERR on success, error code otherwise 
*/
txml_err_t txml_node_add_child(txml_node_t *parent,txml_node_t *child);

/***
    @brief access next sibling of a node (if any)
    @arg pointer to a valid txml_node_t structure
    @return pointer to next sibling if existing, NULL otherwise
*/
txml_node_t *txml_node_next_sibling(txml_node_t *node);

/***
    @brief access previous sibling of a node (if any)
    @arg pointer to a valid txml_node_t structure
    @return pointer to previous sibling if existing, NULL otherwise
*/
txml_node_t *txml_node_prev_sibling(txml_node_t *node);


/***
    @brief add an attribute to txml_node_t *node 
    @arg the txml_node_t that we want to set attributes to 
    @arg the name of the new attribute
    @arg the value of the new attribute
    @return XML_NOERR on success, error code otherwise
 */
txml_err_t txml_node_add_attribute(txml_node_t *node,char *name,char *val);

/***
    @brief substitute an existing branch with a new one
    @arg the xml context pointer
    @arg the index of the branch we want to substitute
    @arg the root of the new branch
    @reurn XML_NOERR on success, error code otherwise
 */
txml_err_t txml_subst_branch(txml_t *xml,unsigned long index, txml_node_t *newBranch);

/***
    @brief Remove a specific node from the xml structure
 */
txml_err_t txml_remove_node(txml_t *xml,char *path);

/***
 */
txml_err_t txml_remove_branch(txml_t *xml,unsigned long index);

/***
    @brief Returns the number of root nodes in the xml context 
    @arg the xml context pointer
    @return the number of root nodes found in the xml context
 */
unsigned long txml_count_branches(txml_t *xml);

/***
    @brief Returns the number of children of the given txml_node_t
    @arg the node we want to query
    @return the number of children of queried node
 */
unsigned long txml_node_count_children(txml_node_t *node);

/***
    @brief Returns the number of attributes of the given txml_node_t
    @arg the node we want to query
    @return the number of attributes that are set for queried node
 */
unsigned long txml_node_count_attributes(txml_node_t *node);

/***
    @brief Returns the txml_node_t at specified path
    @arg the xml context pointer
    @arg the path that references requested node. 
        This must be of formatted as a slash '/' separated list
        of node names ( ex. "tag_A/tag_B/tag_C" )
    @return the node at specified path
 */
txml_node_t *txml_get_node(txml_t *xml, char *path);

/***
    @brief get the root node at a specific index
    @arg the xml context pointer
    @arg the index of the requested root node
    @return the root node at requested index

 */
txml_node_t *txml_get_branch(txml_t *xml,unsigned long index);

/***
    @brief get the child at a specific index inside a node
    @arg the node 
    @arg the index of the child we are interested in
    @return the selected child node 
 */
txml_node_t *txml_node_get_child(txml_node_t *node,unsigned long index);
/***
    @brief get the first child of an txml_node_t whose name is 'name'
    @arg the parent node
    @arg the name of the desired child node
    @return the requested child node
 */
txml_node_t *txml_node_get_child_byname(txml_node_t *node,char *name);


/***
    @brief get node attribute at specified index
    @arg pointer to a valid txml_node_t strucutre
    @arg of the the attribute we want to access (starting by 1)
    @return a pointer to a valid txml_attribute_t structure if found at
    the specified offset, NULL otherwise
*/
txml_attribute_t *txml_node_get_attribute(txml_node_t *node,unsigned long index);

/***
    @brief get node attribute with specified name
    @arg pointer to a valid txml_node_t strucutre
    @arg the name of the desired attribute
    @return a pointer to a valid txml_attribute_t structure if found, NULL otherwise
*/
txml_attribute_t *txml_node_get_attribute_byname(txml_node_t *node, char *name);

/***
    @brief remove attribute at specified index
    @arg pointer to a valid txml_node_t strucutre
    @arg of the the attribute we want to access (starting by 1)
    @return XML_NOERR on success, XML_GENERIC_ERR otherwise
*/
int txml_node_remove_attribute(txml_node_t *node, unsigned long index);

/***
    @brief remove all attributes of a node
    @arg pointer to a valid txml_node_t structure
*/
void txml_node_clear_attributes(txml_node_t *node);


/***
    @brief save the configuration stored in the xml file containing the current profile
           the xml file name is obtained appending '.xml' to the category name . The xml file is stored 
           in the repository directory specified during object construction.
    @arg pointer to a valid xml context
    @arg the path where to save the file
    @return an txml_err_t error status (XML_NOERR if buffer was parsed successfully)
*/
txml_err_t txml_save(txml_t *xml,char *path);

/***
    @brief allocate resources for a new namespace 
    @arg the shortname of the new namespace
    @arg the complete uri of the new namspace
    @return a valid txml_namespace_t pointer on success, NULL otherwise
*/
txml_namespace_t *txml_namespace_create(char *nsName, char *nsUri);

/***
 
*/
void txml_namespace_destroy(txml_namespace_t *ns);

/***
    @brief search for a specific namespace defined within the current document
    @arg pointer to a valid txml_node_t structure
    @arg the shortname of the new namespace
*/
txml_namespace_t *txml_node_get_namespace_byname(txml_node_t *node, char *nsName);

/***
    @brief search for a specific namespace defined within the current document
    @arg pointer to a valid txml_node_t structure
    @arg the complete uri of the new namspace
    @return a valid txml_namespace_t pointer if found, NULL otherwise
*/
txml_namespace_t *txml_node_get_namespace_byuri(txml_node_t *node, char *nsUri);

/***
    @brief create a new namespace and link it to current document/context
    @arg pointer to a valid txml_node_t structure
    @arg the shortname of the new namespace
    @arg the complete uri of the new namspace
    @return a valid txml_namespace_t pointer if found, NULL otherwise
*/
txml_namespace_t *txml_node_add_namespace(txml_node_t *node, char *nsName, char *nsUri);

/***
    @brief get the namespace of a node , if any
    @arg pointer to a valid txml_node_t strucutre
    @return a pointer to the txml_namespace_t of the node if defined or inherited, NULL otherwise
*/
txml_namespace_t *txml_node_get_namespace(txml_node_t *node);

/***
    @brief set the namespace of a node
    @arg pointer to a valid txml_node_t structure
    @return XML_NOERR on success, any other xml error code otherwise
*/ 
txml_err_t txml_node_set_namespace(txml_node_t *node, txml_namespace_t *ns);

/***
    @brief set the default namespace of a node 
           (which will be inherited by all descendant, unless overridden)
    @arg pointer to a valid txml_node_t structure
    @arg pointer to a valid txml_namespace_t structure
    @return XML_NOERR on success, any other xml error code otherwise
*/ 
txml_err_t txml_node_set_cnamespace(txml_node_t *node, txml_namespace_t *ns);

int txml_has_iconv();

#ifdef __cplusplus
}
#endif

#endif
