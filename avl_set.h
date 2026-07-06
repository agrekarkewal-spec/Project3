/*
 * ============================================================================
 *  avl_set.h -- Public API for an AVL-tree-backed Set ADT
 * ============================================================================
 *  Design notes:
 *   - The AVLSet type is OPAQUE. Callers only ever see a pointer to it; the
 *     internal tree (nodes, rotations, height bookkeeping) lives entirely in
 *     avl_set.c. This keeps the tree invariants safe from outside tampering
 *     and means there is nothing here for a caller to get wrong by poking at
 *     internals.
 *   - There are NO global variables anywhere in this library. Every function
 *     receives whatever state it needs as an explicit argument (an AVLSet*,
 *     a value, a file path, ...). Two completely independent programs could
 *     link against this library and use it concurrently for unrelated sets
 *     without any shared mutable state between them.
 *   - Every function that allocates memory documents who is responsible for
 *     freeing it. As a rule of thumb:
 *       * Anything returned as `AVLSet *`      -> free with avlset_destroy()
 *       * Anything returned as `AVLSet **`     -> free with avlset_free_multiple()
 *       * Anything returned as a raw `int *`   -> free with plain free()
 *   - Naming convention: every public symbol is prefixed `avlset_` (functions)
 *     or `AVLSet` / `AVLSET_` (types, enum constants). snake_case for
 *     functions and variables, PascalCase only for the opaque typedef.
 * ============================================================================
 */

#ifndef AVL_SET_H
#define AVL_SET_H

#include <stddef.h> /* size_t */

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------------- *
 *  Opaque type
 * ---------------------------------------------------------------------- */

typedef struct AVLSet AVLSet;

/* ---------------------------------------------------------------------- *
 *  Status codes
 *
 *  Any function that can fail returns one of these (or writes one into an
 *  `AVLSetStatus *out_status` argument) instead of relying on errno or a
 *  global "last error" flag.
 * ---------------------------------------------------------------------- */

typedef enum AVLSetStatus {
    AVLSET_OK = 0,
    AVLSET_ERR_NULL_ARG,        /* a required pointer argument was NULL   */
    AVLSET_ERR_ALLOC,           /* malloc/realloc failed                 */
    AVLSET_ERR_FILE_OPEN,       /* fopen() failed (bad path/permissions) */
    AVLSET_ERR_FILE_FORMAT,     /* file exists but is not valid AVLSET   */
    AVLSET_ERR_NOT_FOUND        /* value not present (for remove/search) */
} AVLSetStatus;

/* Human-readable description of a status code. Never returns NULL. */
const char *avlset_status_string(AVLSetStatus status);

/* ---------------------------------------------------------------------- *
 *  Lifecycle
 * ---------------------------------------------------------------------- */

/* Creates an empty set. Returns NULL only on allocation failure. */
AVLSet *avlset_create(void);

/* Frees every node in the set plus the set handle itself. Safe to pass NULL. */
void avlset_destroy(AVLSet *set);

/* Deep-copies a set (independent tree, independent memory). Returns NULL on
 * allocation failure or if `set` is NULL. */
AVLSet *avlset_clone(const AVLSet *set);

/* ---------------------------------------------------------------------- *
 *  Core AVL-backed operations
 * ---------------------------------------------------------------------- */

/* Inserts `value`. If `was_inserted` is non-NULL, it is set to 1 when the
 * value was newly added or 0 when it was already present (sets never store
 * duplicates, so this is how you tell insert-vs-noop apart). */
AVLSetStatus avlset_insert(AVLSet *set, int value, int *was_inserted);

/* Removes `value`. If `was_removed` is non-NULL, it is set to 1 if the value
 * was found and removed, or 0 if it was not present. */
AVLSetStatus avlset_remove(AVLSet *set, int value, int *was_removed);

/* Returns 1 if `value` is in the set, 0 otherwise (also 0 if set is NULL). */
int avlset_contains(const AVLSet *set, int value);

/* Number of elements currently stored. */
size_t avlset_size(const AVLSet *set);

/* Convenience wrapper around avlset_size(set) == 0. */
int avlset_is_empty(const AVLSet *set);

/* Removes every element, leaving an empty-but-still-valid set handle. */
void avlset_clear(AVLSet *set);

/* ---------------------------------------------------------------------- *
 *  Export to a plain array (sorted ascending)
 *
 *  On success (*out_array, *out_size) describes a freshly malloc'd array
 *  that the CALLER must free() (a plain free(), not avlset_destroy -- this
 *  is a raw int array, not a set). If the set is empty, *out_array is set
 *  to NULL and *out_size to 0, which is not an error.
 * ---------------------------------------------------------------------- */
AVLSetStatus avlset_to_array(const AVLSet *set, int **out_array, size_t *out_size);

/* ---------------------------------------------------------------------- *
 *  Set algebra -- each returns a brand-new, independent AVLSet*.
 *  The inputs are never modified. Returns NULL on allocation failure or if
 *  either input pointer is NULL.
 * ---------------------------------------------------------------------- */

AVLSet *avlset_union(const AVLSet *a, const AVLSet *b);
AVLSet *avlset_intersection(const AVLSet *a, const AVLSet *b);
AVLSet *avlset_difference(const AVLSet *a, const AVLSet *b);            /* a - b */
AVLSet *avlset_symmetric_difference(const AVLSet *a, const AVLSet *b);  /* (a-b) U (b-a) */

/* 1 if every element of `a` is in `b` (empty set is a subset of anything). */
int avlset_is_subset(const AVLSet *a, const AVLSet *b);

/* 1 if `a` and `b` contain exactly the same elements. */
int avlset_is_equal(const AVLSet *a, const AVLSet *b);

/* ---------------------------------------------------------------------- *
 *  File I/O
 *
 *  File format ("AVLSET v1"): a small human-readable text format --
 *
 *      # AVLSET v1
 *      # label: <free-form text, no newlines>
 *      # count: <n>
 *      <value_1>
 *      <value_2>
 *      ...
 *      <value_n>
 *
 *  Lines starting with '#' are metadata/comments and are ignored by the
 *  parser except for validating the format tag. This single format is used
 *  both for plain save/load (avlset_save_to_file / avlset_load_from_file)
 *  and for labeled result exports (avlset_export_result), so any exported
 *  result can also be reloaded later with avlset_load_from_file().
 * ---------------------------------------------------------------------- */

/* Saves `set` to `filepath` with the default label "set". */
AVLSetStatus avlset_save_to_file(const AVLSet *set, const char *filepath);

/* Saves `set` to `filepath` tagged with a caller-supplied label, e.g.
 * avlset_export_result(result, "union_A_B.avlset", "A union B"). */
AVLSetStatus avlset_export_result(const AVLSet *set, const char *filepath, const char *label);

/* Loads a set previously written by avlset_save_to_file / avlset_export_result.
 * Returns a newly allocated AVLSet* (caller must avlset_destroy it) on
 * success, or NULL on failure. If `out_status` is non-NULL, it receives the
 * precise reason for failure. */
AVLSet *avlset_load_from_file(const char *filepath, AVLSetStatus *out_status);

/* Loads several dataset files at once.
 *   filepaths       - array of `count` C-string paths
 *   out_sets        - on success, points at a freshly malloc'd array of
 *                      AVLSet* of length *out_loaded_count (caller frees
 *                      with avlset_free_multiple)
 *   out_loaded_count- number of files successfully loaded (<= count)
 *
 *  Individual file failures do not abort the whole batch: a file that fails
 *  to load is simply skipped, so callers can import a directory of datasets
 *  where a few files might be missing or malformed and still get the rest.
 *  Returns AVLSET_OK if at least one file loaded, or the status of the last
 *  failure if none did. */
AVLSetStatus avlset_import_multiple(const char **filepaths,
                                     size_t count,
                                     AVLSet ***out_sets,
                                     size_t *out_loaded_count);

/* Frees an array of AVLSet* returned by avlset_import_multiple, including
 * every set inside it and the array itself. Safe to pass NULL. */
void avlset_free_multiple(AVLSet **sets, size_t count);

/* ---------------------------------------------------------------------- *
 *  Diagnostics
 * ---------------------------------------------------------------------- */

/* Prints "label = { e1, e2, ... }  (size = n)" to stdout. Purely a debug /
 * demo convenience -- not part of the file I/O path. */
void avlset_print(const AVLSet *set, const char *label);

#ifdef __cplusplus
}
#endif

#endif /* AVL_SET_H */
