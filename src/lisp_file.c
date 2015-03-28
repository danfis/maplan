#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include <boruvka/alloc.h>

#include "lisp_file.h"

#define IS_WS(c) ((c) == ' ' || (c) == '\n' || (c) == '\r' || (c) == '\t')
#define IS_ALPHA(c) (!IS_WS(c) && (c) != ')' && (c) != '(' && (c) != ';')

static plan_lisp_node_t *lispNodeNew(void)
{
    plan_lisp_node_t *n;

    n = BOR_ALLOC(plan_lisp_node_t);
    n->value = NULL;
    borListInit(&n->children);
    borListInit(&n->list);
    return n;
}

static void lispNodeDel(plan_lisp_node_t *n)
{
    bor_list_t *item, *tmp;
    plan_lisp_node_t *m;

    BOR_LIST_FOR_EACH_SAFE(&n->children, item, tmp){
        m = BOR_LIST_ENTRY(item, plan_lisp_node_t, list);
        lispNodeDel(m);
    }

    BOR_FREE(n);
}

static plan_lisp_node_t *parseExp(char *data, int from, int size, int *cont)
{
    plan_lisp_node_t *exp, *sub;
    int i = from;
    char c;

    if (i >= size)
        return NULL;

    exp = lispNodeNew();
    c = data[i];
    while (i < size){
        if (IS_WS(c)){
            // Skip whitespace
            c = data[++i];
            continue;

        }else if (c == ';'){
            // Skip comments
            for (++i; i < size && data[i] != '\n'; ++i);
            if (i < size)
                c = data[i];
            continue;

        }else if (c == '('){
            // Parse subexpression
            sub = parseExp(data, i + 1, size, &i);
            if (sub == NULL){
                lispNodeDel(exp);
                return NULL;
            }

            borListAppend(&exp->children, &sub->list);
            c = data[i];
            continue;

        }else if (c == ')'){
            // Finalize expression
            if (cont != NULL)
                *cont = i + 1;
            return exp;

        }else{
            sub = lispNodeNew();
            borListAppend(&exp->children, &sub->list);
            sub->value = data + i;
            for (; i < size && IS_ALPHA(data[i]); ++i);
            if (i == size){
                lispNodeDel(exp);
                return NULL;
            }

            c = data[i];
            data[i] = 0x0;
        }
    }

    lispNodeDel(exp);
    return NULL;
}

plan_lisp_file_t *planLispFileParse(const char *fn)
{
    int fd, i;
    struct stat st;
    char *data;
    plan_lisp_file_t *lisp;
    plan_lisp_node_t *root = NULL;

    fd = open(fn, O_RDONLY);
    if (fd == -1){
        fprintf(stderr, "Error: Could not open `%s'.\n", fn);
        fflush(stderr);
        return NULL;
    }

    if (fstat(fd, &st) != 0){
        fprintf(stderr, "Error: Could fstat `%s'.\n", fn);
        fflush(stderr);
        close(fd);
        return NULL;
    }

    data = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED){
        fprintf(stderr, "Error: Could not mmap `%s' to memory.\n", fn);
        fflush(stderr);
        close(fd);
        return NULL;
    }

    for (i = 0; i < (int)st.st_size && data[i] != '('; ++i);
    root = parseExp(data, i + 1, st.st_size, NULL);
    if (root == NULL){
        fprintf(stderr, "Error: Could not parse file `%s'.\n", fn);
        fflush(stderr);
        munmap((void *)data, st.st_size);
        close(fd);
        return NULL;
    }

    lisp = BOR_ALLOC(plan_lisp_file_t);
    lisp->fd = fd;
    lisp->data = data;
    lisp->size = st.st_size;
    lisp->root = root;

    return lisp;
}

void planLispFileDel(plan_lisp_file_t *lisp)
{
    if (lisp->data)
        munmap((void *)lisp->data, lisp->size);
    if (lisp->fd >= 0)
        close(lisp->fd);
    if (lisp->root)
        lispNodeDel(lisp->root);
    BOR_FREE(lisp);
}

static void nodeDump(const plan_lisp_node_t *node, FILE *fout, int prefix)
{
    bor_list_t *item;
    const plan_lisp_node_t *m;
    int i;

    if (node->value != NULL){
        for (i = 0; i < prefix; ++i)
            fprintf(fout, " ");
        fprintf(fout, "%s\n", node->value);
    }else{
        for (i = 0; i < prefix; ++i)
            fprintf(fout, " ");
        fprintf(fout, "(\n");

        BOR_LIST_FOR_EACH(&node->children, item){
            m = BOR_LIST_ENTRY(item, plan_lisp_node_t, list);
            nodeDump(m, fout, prefix + 4);
        }

        for (i = 0; i < prefix; ++i)
            fprintf(fout, " ");
        fprintf(fout, ")\n");
    }
}

void planLispFileDump(const plan_lisp_file_t *lisp, FILE *fout)
{
    nodeDump(lisp->root, fout, 0);
}
