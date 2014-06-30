
#define NEW_FN planListLazyRBTreeNew
#define TREE_T bor_rbtree_int_t
#define TREE_NODE_T bor_rbtree_int_node_t
#define TREE_FOR_EACH_SAFE BOR_RBTREE_INT_FOR_EACH_SAFE
#define TREE_INIT borRBTreeIntInit
#define TREE_FREE borRBTreeIntFree
#define TREE_INSERT borRBTreeIntInsert
#define TREE_EMPTY borRBTreeIntEmpty
#define TREE_MIN borRBTreeIntMin
#define TREE_REMOVE borRBTreeIntRemove


#include "_list_lazy_tree.c"
