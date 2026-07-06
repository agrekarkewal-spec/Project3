/*
 * ============================================================================
 *  avl_set.c -- Implementation of the AVL-tree-backed Set ADT
 * ============================================================================
 *  Everything below is either `static` (file-local, invisible outside this
 *  translation unit) or part of the public API declared in avl_set.h. There
 *  are no global/static mutable variables: all state lives inside heap
 *  objects reached only through the pointers the caller holds.
 * ============================================================================
 */

#include "avl_set.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------------------------------------------------------------------- *
 *  Internal node type (hidden from callers via the opaque AVLSet handle)
 * ---------------------------------------------------------------------- */

typedef struct AVLNode {
    int data;
    struct AVLNode *left;
    struct AVLNode *right;
    int height;
} AVLNode;

/* The opaque AVLSet just wraps a root pointer plus a cached element count,
 * so avlset_size() is O(1) instead of an O(n) tree walk. */
struct AVLSet {
    AVLNode *root;
    size_t count;
};

/* ---------------------------------------------------------------------- *
 *  Small internal helpers
 * ---------------------------------------------------------------------- */

static int avlset_max_int(int a, int b) {
    return (a > b) ? a : b;
}

static int node_height(const AVLNode *n) {
    return n ? n->height : 0;
}

static int node_balance_factor(const AVLNode *n) {
    return n ? node_height(n->left) - node_height(n->right) : 0;
}

static AVLNode *node_create(int data) {
    AVLNode *n = (AVLNode *)malloc(sizeof(AVLNode));
    if (!n) return NULL;
    n->data = data;
    n->left = n->right = NULL;
    n->height = 1;
    return n;
}

/* ---------------------------------------------------------------------- *
 *  Rotations
 * ---------------------------------------------------------------------- */

static AVLNode *rotate_right(AVLNode *y) {
    AVLNode *x = y->left;
    AVLNode *t2 = x->right;

    x->right = y;
    y->left = t2;

    y->height = avlset_max_int(node_height(y->left), node_height(y->right)) + 1;
    x->height = avlset_max_int(node_height(x->left), node_height(x->right)) + 1;

    return x;
}

static AVLNode *rotate_left(AVLNode *x) {
    AVLNode *y = x->right;
    AVLNode *t2 = y->left;

    y->left = x;
    x->right = t2;

    x->height = avlset_max_int(node_height(x->left), node_height(x->right)) + 1;
    y->height = avlset_max_int(node_height(y->left), node_height(y->right)) + 1;

    return y;
}

static AVLNode *node_rebalance(AVLNode *node) {
    node->height = avlset_max_int(node_height(node->left), node_height(node->right)) + 1;
    int bf = node_balance_factor(node);

    if (bf > 1) {
        if (node_balance_factor(node->left) < 0)
            node->left = rotate_left(node->left);
        return rotate_right(node);
    }
    if (bf < -1) {
        if (node_balance_factor(node->right) > 0)
            node->right = rotate_right(node->right);
        return rotate_left(node);
    }
    return node;
}

/* ---------------------------------------------------------------------- *
 *  Internal insert / delete / search on raw AVLNode trees.
 *  `ok` is set to 0 if a required allocation failed partway through, so the
 *  public wrapper can report AVLSET_ERR_ALLOC instead of silently losing the
 *  failure.
 * ---------------------------------------------------------------------- */

static AVLNode *node_insert(AVLNode *root, int data, int *changed, int *ok) {
    if (root == NULL) {
        AVLNode *n = node_create(data);
        if (!n) {
            *ok = 0;
            return NULL;
        }
        *changed = 1;
        return n;
    }

    if (data < root->data) {
        AVLNode *new_left = node_insert(root->left, data, changed, ok);
        if (!*ok) return root;
        root->left = new_left;
    } else if (data > root->data) {
        AVLNode *new_right = node_insert(root->right, data, changed, ok);
        if (!*ok) return root;
        root->right = new_right;
    } else {
        *changed = 0; /* duplicate: set already has this element */
        return root;
    }

    return node_rebalance(root);
}

static const AVLNode *node_search(const AVLNode *root, int data) {
    if (root == NULL) return NULL;
    if (data == root->data) return root;
    return (data < root->data) ? node_search(root->left, data)
                                : node_search(root->right, data);
}

static const AVLNode *node_min(const AVLNode *node) {
    const AVLNode *cur = node;
    while (cur->left != NULL)
        cur = cur->left;
    return cur;
}

static AVLNode *node_delete(AVLNode *root, int data, int *changed) {
    if (root == NULL) {
        *changed = 0;
        return NULL;
    }

    if (data < root->data) {
        root->left = node_delete(root->left, data, changed);
    } else if (data > root->data) {
        root->right = node_delete(root->right, data, changed);
    } else {
        *changed = 1;

        if (root->left == NULL || root->right == NULL) {
            AVLNode *child = root->left ? root->left : root->right;
            free(root);
            return child; /* NULL if there was no child either */
        } else {
            const AVLNode *succ = node_min(root->right);
            root->data = succ->data;
            int dummy;
            root->right = node_delete(root->right, succ->data, &dummy);
        }
    }

    if (root == NULL) return NULL;
    return node_rebalance(root);
}

static void node_free_all(AVLNode *root) {
    if (root == NULL) return;
    node_free_all(root->left);
    node_free_all(root->right);
    free(root);
}

static void node_inorder_fill(const AVLNode *root, int *arr, size_t *idx) {
    if (root == NULL) return;
    node_inorder_fill(root->left, arr, idx);
    arr[(*idx)++] = root->data;
    node_inorder_fill(root->right, arr, idx);
}

/* ---------------------------------------------------------------------- *
 *  Status strings
 * ---------------------------------------------------------------------- */

const char *avlset_status_string(AVLSetStatus status) {
    switch (status) {
        case AVLSET_OK:               return "OK";
        case AVLSET_ERR_NULL_ARG:      return "Null argument supplied";
        case AVLSET_ERR_ALLOC:        return "Memory allocation failed";
        case AVLSET_ERR_FILE_OPEN:     return "Could not open file";
        case AVLSET_ERR_FILE_FORMAT:   return "File is not a valid AVLSET file";
        case AVLSET_ERR_NOT_FOUND:     return "Value not found";
        default:                       return "Unknown status";
    }
}

/* ---------------------------------------------------------------------- *
 *  Lifecycle
 * ---------------------------------------------------------------------- */

AVLSet *avlset_create(void) {
    AVLSet *set = (AVLSet *)malloc(sizeof(AVLSet));
    if (!set) return NULL;
    set->root = NULL;
    set->count = 0;
    return set;
}

void avlset_destroy(AVLSet *set) {
    if (!set) return;
    node_free_all(set->root);
    free(set);
}

void avlset_clear(AVLSet *set) {
    if (!set) return;
    node_free_all(set->root);
    set->root = NULL;
    set->count = 0;
}

AVLSet *avlset_clone(const AVLSet *set) {
    if (!set) return NULL;

    AVLSet *clone = avlset_create();
    if (!clone) return NULL;

    int *arr = NULL;
    size_t n = 0;
    if (avlset_to_array(set, &arr, &n) != AVLSET_OK) {
        avlset_destroy(clone);
        return NULL;
    }

    for (size_t i = 0; i < n; i++) {
        int changed, ok = 1;
        clone->root = node_insert(clone->root, arr[i], &changed, &ok);
        if (!ok) {
            free(arr);
            avlset_destroy(clone);
            return NULL;
        }
        if (changed) clone->count++;
    }

    free(arr);
    return clone;
}

/* ---------------------------------------------------------------------- *
 *  Core operations
 * ---------------------------------------------------------------------- */

AVLSetStatus avlset_insert(AVLSet *set, int value, int *was_inserted) {
    if (!set) return AVLSET_ERR_NULL_ARG;

    int changed = 0;
    int ok = 1;
    set->root = node_insert(set->root, value, &changed, &ok);

    if (!ok) return AVLSET_ERR_ALLOC;
    if (changed) set->count++;
    if (was_inserted) *was_inserted = changed;
    return AVLSET_OK;
}

AVLSetStatus avlset_remove(AVLSet *set, int value, int *was_removed) {
    if (!set) return AVLSET_ERR_NULL_ARG;

    int changed = 0;
    set->root = node_delete(set->root, value, &changed);

    if (changed) set->count--;
    if (was_removed) *was_removed = changed;
    return changed ? AVLSET_OK : AVLSET_ERR_NOT_FOUND;
}

int avlset_contains(const AVLSet *set, int value) {
    if (!set) return 0;
    return node_search(set->root, value) != NULL;
}

size_t avlset_size(const AVLSet *set) {
    return set ? set->count : 0;
}

int avlset_is_empty(const AVLSet *set) {
    return avlset_size(set) == 0;
}

/* ---------------------------------------------------------------------- *
 *  Array export
 * ---------------------------------------------------------------------- */

AVLSetStatus avlset_to_array(const AVLSet *set, int **out_array, size_t *out_size) {
    if (!set || !out_array || !out_size) return AVLSET_ERR_NULL_ARG;

    size_t n = set->count;
    if (n == 0) {
        *out_array = NULL;
        *out_size = 0;
        return AVLSET_OK;
    }

    int *arr = (int *)malloc(sizeof(int) * n);
    if (!arr) return AVLSET_ERR_ALLOC;

    size_t idx = 0;
    node_inorder_fill(set->root, arr, &idx);

    *out_array = arr;
    *out_size = n;
    return AVLSET_OK;
}

/* ---------------------------------------------------------------------- *
 *  Set algebra
 * ---------------------------------------------------------------------- */

AVLSet *avlset_union(const AVLSet *a, const AVLSet *b) {
    if (!a || !b) return NULL;

    AVLSet *result = avlset_clone(a);
    if (!result) return NULL;

    int *arr_b = NULL;
    size_t n_b = 0;
    if (avlset_to_array(b, &arr_b, &n_b) != AVLSET_OK) {
        avlset_destroy(result);
        return NULL;
    }

    for (size_t i = 0; i < n_b; i++) {
        if (avlset_insert(result, arr_b[i], NULL) != AVLSET_OK) {
            free(arr_b);
            avlset_destroy(result);
            return NULL;
        }
    }

    free(arr_b);
    return result;
}

AVLSet *avlset_intersection(const AVLSet *a, const AVLSet *b) {
    if (!a || !b) return NULL;

    AVLSet *result = avlset_create();
    if (!result) return NULL;

    int *arr_a = NULL;
    size_t n_a = 0;
    if (avlset_to_array(a, &arr_a, &n_a) != AVLSET_OK) {
        avlset_destroy(result);
        return NULL;
    }

    for (size_t i = 0; i < n_a; i++) {
        if (avlset_contains(b, arr_a[i])) {
            if (avlset_insert(result, arr_a[i], NULL) != AVLSET_OK) {
                free(arr_a);
                avlset_destroy(result);
                return NULL;
            }
        }
    }

    free(arr_a);
    return result;
}

AVLSet *avlset_difference(const AVLSet *a, const AVLSet *b) {
    if (!a || !b) return NULL;

    AVLSet *result = avlset_create();
    if (!result) return NULL;

    int *arr_a = NULL;
    size_t n_a = 0;
    if (avlset_to_array(a, &arr_a, &n_a) != AVLSET_OK) {
        avlset_destroy(result);
        return NULL;
    }

    for (size_t i = 0; i < n_a; i++) {
        if (!avlset_contains(b, arr_a[i])) {
            if (avlset_insert(result, arr_a[i], NULL) != AVLSET_OK) {
                free(arr_a);
                avlset_destroy(result);
                return NULL;
            }
        }
    }

    free(arr_a);
    return result;
}

AVLSet *avlset_symmetric_difference(const AVLSet *a, const AVLSet *b) {
    if (!a || !b) return NULL;

    AVLSet *a_minus_b = avlset_difference(a, b);
    AVLSet *b_minus_a = avlset_difference(b, a);
    if (!a_minus_b || !b_minus_a) {
        avlset_destroy(a_minus_b);
        avlset_destroy(b_minus_a);
        return NULL;
    }

    AVLSet *result = avlset_union(a_minus_b, b_minus_a);

    avlset_destroy(a_minus_b);
    avlset_destroy(b_minus_a);
    return result;
}

int avlset_is_subset(const AVLSet *a, const AVLSet *b) {
    if (!a) return 0;
    if (avlset_is_empty(a)) return 1;

    int *arr_a = NULL;
    size_t n_a = 0;
    if (avlset_to_array(a, &arr_a, &n_a) != AVLSET_OK) return 0;

    int result = 1;
    for (size_t i = 0; i < n_a; i++) {
        if (!avlset_contains(b, arr_a[i])) {
            result = 0;
            break;
        }
    }

    free(arr_a);
    return result;
}

int avlset_is_equal(const AVLSet *a, const AVLSet *b) {
    if (!a || !b) return 0;
    if (avlset_size(a) != avlset_size(b)) return 0;
    return avlset_is_subset(a, b) && avlset_is_subset(b, a);
}

/* ---------------------------------------------------------------------- *
 *  File I/O
 * ---------------------------------------------------------------------- */

#define AVLSET_FILE_MAGIC "# AVLSET v1"
#define AVLSET_MAX_LINE 512

static AVLSetStatus write_set_to_stream(FILE *fp, const AVLSet *set, const char *label) {
    int *arr = NULL;
    size_t n = 0;
    AVLSetStatus st = avlset_to_array(set, &arr, &n);
    if (st != AVLSET_OK) return st;

    fprintf(fp, "%s\n", AVLSET_FILE_MAGIC);
    fprintf(fp, "# label: %s\n", (label && label[0]) ? label : "set");
    fprintf(fp, "# count: %zu\n", n);

    for (size_t i = 0; i < n; i++)
        fprintf(fp, "%d\n", arr[i]);

    free(arr);
    return AVLSET_OK;
}

AVLSetStatus avlset_save_to_file(const AVLSet *set, const char *filepath) {
    return avlset_export_result(set, filepath, "set");
}

AVLSetStatus avlset_export_result(const AVLSet *set, const char *filepath, const char *label) {
    if (!set || !filepath) return AVLSET_ERR_NULL_ARG;

    FILE *fp = fopen(filepath, "w");
    if (!fp) return AVLSET_ERR_FILE_OPEN;

    AVLSetStatus st = write_set_to_stream(fp, set, label);
    fclose(fp);
    return st;
}

AVLSet *avlset_load_from_file(const char *filepath, AVLSetStatus *out_status) {
    AVLSetStatus local_status = AVLSET_OK;
    if (!out_status) out_status = &local_status;

    if (!filepath) {
        *out_status = AVLSET_ERR_NULL_ARG;
        return NULL;
    }

    FILE *fp = fopen(filepath, "r");
    if (!fp) {
        *out_status = AVLSET_ERR_FILE_OPEN;
        return NULL;
    }

    char line[AVLSET_MAX_LINE];

    /* First non-empty line must be the magic header. */
    if (!fgets(line, sizeof(line), fp) ||
        strncmp(line, AVLSET_FILE_MAGIC, strlen(AVLSET_FILE_MAGIC)) != 0) {
        fclose(fp);
        *out_status = AVLSET_ERR_FILE_FORMAT;
        return NULL;
    }

    AVLSet *set = avlset_create();
    if (!set) {
        fclose(fp);
        *out_status = AVLSET_ERR_ALLOC;
        return NULL;
    }

    while (fgets(line, sizeof(line), fp)) {
        /* Skip metadata/comment lines and blank lines. */
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
            continue;

        int value;
        if (sscanf(line, "%d", &value) == 1) {
            if (avlset_insert(set, value, NULL) != AVLSET_OK) {
                fclose(fp);
                avlset_destroy(set);
                *out_status = AVLSET_ERR_ALLOC;
                return NULL;
            }
        }
    }

    fclose(fp);
    *out_status = AVLSET_OK;
    return set;
}

AVLSetStatus avlset_import_multiple(const char **filepaths,
                                     size_t count,
                                     AVLSet ***out_sets,
                                     size_t *out_loaded_count) {
    if (!filepaths || !out_sets || !out_loaded_count) return AVLSET_ERR_NULL_ARG;

    *out_sets = NULL;
    *out_loaded_count = 0;

    if (count == 0) return AVLSET_OK;

    AVLSet **loaded = (AVLSet **)malloc(sizeof(AVLSet *) * count);
    if (!loaded) return AVLSET_ERR_ALLOC;

    size_t loaded_count = 0;
    AVLSetStatus last_status = AVLSET_OK;

    for (size_t i = 0; i < count; i++) {
        AVLSetStatus st;
        AVLSet *s = avlset_load_from_file(filepaths[i], &st);
        if (s) {
            loaded[loaded_count++] = s;
        } else {
            last_status = st; /* remember why this one failed, keep going */
        }
    }

    if (loaded_count == 0) {
        free(loaded);
        *out_sets = NULL;
        *out_loaded_count = 0;
        return last_status;
    }

    /* Shrink the array down to exactly how many actually loaded. realloc to a
     * smaller size cannot fail in a way that loses the original block, but we
     * guard anyway and fall back to the original (over-sized) block. */
    AVLSet **shrunk = (AVLSet **)realloc(loaded, sizeof(AVLSet *) * loaded_count);
    if (shrunk) loaded = shrunk;

    *out_sets = loaded;
    *out_loaded_count = loaded_count;
    return AVLSET_OK;
}

void avlset_free_multiple(AVLSet **sets, size_t count) {
    if (!sets) return;
    for (size_t i = 0; i < count; i++)
        avlset_destroy(sets[i]);
    free(sets);
}

/* ---------------------------------------------------------------------- *
 *  Diagnostics
 * ---------------------------------------------------------------------- */

void avlset_print(const AVLSet *set, const char *label) {
    int *arr = NULL;
    size_t n = 0;

    if (avlset_to_array(set, &arr, &n) != AVLSET_OK) {
        printf("%s = <error reading set>\n", label ? label : "set");
        return;
    }

    printf("%s = { ", label ? label : "set");
    for (size_t i = 0; i < n; i++)
        printf("%d%s", arr[i], (i == n - 1) ? "" : ", ");
    printf(" }  (size = %zu)\n", n);

    free(arr);
}
