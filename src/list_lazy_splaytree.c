/***
 * maplan
 * -------
 * Copyright (c)2015 Daniel Fiser <danfis@danfis.cz>,
 * Agent Technology Center, Department of Computer Science,
 * Faculty of Electrical Engineering, Czech Technical University in Prague.
 * All rights reserved.
 *
 * This file is part of maplan.
 *
 * Distributed under the OSI-approved BSD License (the "License");
 * see accompanying file BDS-LICENSE for details or see
 * <http://www.opensource.org/licenses/bsd-license.php>.
 *
 * This software is distributed WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the License for more information.
 */


#define NEW_FN planListLazySplayTreeNew
#define TREE_T bor_splaytree_int_t
#define TREE_NODE_T bor_splaytree_int_node_t
#define TREE_FOR_EACH_SAFE BOR_SPLAYTREE_INT_FOR_EACH_SAFE
#define TREE_INIT borSplayTreeIntInit
#define TREE_FREE borSplayTreeIntFree
#define TREE_INSERT borSplayTreeIntInsert
#define TREE_EMPTY borSplayTreeIntEmpty
#define TREE_MIN borSplayTreeIntMin
#define TREE_REMOVE borSplayTreeIntRemove


#include "_list_lazy_tree.c"

