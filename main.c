/*
 * ============================================================================
 *  main.c -- Interactive demo driver for the avl_set library
 * ============================================================================
 *  All state (set A, set B, the last computed result, an imported dataset
 *  table) lives in local variables inside main(), passed to helper functions
 *  by pointer as needed. Nothing here is a global variable.
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "avl_set.h"

/* ---------------------------------------------------------------------- *
 *  Small input helpers
 * ---------------------------------------------------------------------- */

static void flush_stdin_line(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { /* discard */ }
}

static int read_int(const char *prompt) {
    int value;
    printf("%s", prompt);
    while (scanf("%d", &value) != 1) {
        flush_stdin_line();
        printf("Invalid input, enter an integer: ");
    }
    flush_stdin_line();
    return value;
}

static void read_line(const char *prompt, char *buffer, size_t buffer_size) {
    printf("%s", prompt);
    if (fgets(buffer, (int)buffer_size, stdin) == NULL) {
        buffer[0] = '\0';
        return;
    }
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n')
        buffer[len - 1] = '\0';
}

/* ---------------------------------------------------------------------- *
 *  Menu
 * ---------------------------------------------------------------------- */

static void print_menu(void) {
    printf("\n===================== AVL SET OPERATIONS =====================\n");
    printf(" 1. Insert into A              2. Insert into B\n");
    printf(" 3. Delete from A              4. Delete from B\n");
    printf(" 5. Search in A                6. Search in B\n");
    printf(" 7. Print A                    8. Print B\n");
    printf(" 9. Union            (A U B)  10. Intersection   (A ^ B)\n");
    printf("11. Difference       (A - B)  12. Difference     (B - A)\n");
    printf("13. Symmetric Diff   (A (+) B)\n");
    printf("14. Is A subset of B ?        15. Are A and B equal ?\n");
    printf("16. Save A to file            17. Save B to file\n");
    printf("18. Load A from file          19. Load B from file\n");
    printf("20. Export last result to file\n");
    printf("21. Import multiple dataset files\n");
    printf(" 0. Exit\n");
    printf("================================================================\n");
}

/* Runs a binary set operation, prints the result, and stores it as
 * `*last_result` (freeing whatever was there before) so it can be exported
 * afterwards with option 20. */
static void run_binary_op(AVLSet *(*op)(const AVLSet *, const AVLSet *),
                           const AVLSet *a, const AVLSet *b,
                           const char *label,
                           AVLSet **last_result) {
    AVLSet *result = op(a, b);
    if (!result) {
        printf("Operation failed (allocation error).\n");
        return;
    }
    avlset_print(result, label);

    avlset_destroy(*last_result);
    *last_result = result;
}

int main(void) {
    AVLSet *set_a = avlset_create();
    AVLSet *set_b = avlset_create();
    AVLSet *last_result = NULL; /* result of the most recent set-algebra op */

    if (!set_a || !set_b) {
        fprintf(stderr, "Fatal: could not allocate initial sets.\n");
        avlset_destroy(set_a);
        avlset_destroy(set_b);
        return EXIT_FAILURE;
    }

    printf("AVL-Backed Set ADT -- Library Demo Driver\n");

    int choice;
    do {
        print_menu();
        choice = read_int("Enter choice: ");

        int value, flag;
        char path[256];
        char label[128];
        AVLSetStatus status;

        switch (choice) {
            case 1:
                value = read_int("Value to insert into A: ");
                status = avlset_insert(set_a, value, &flag);
                if (status != AVLSET_OK) printf("Error: %s\n", avlset_status_string(status));
                else printf(flag ? "Inserted %d into A.\n" : "%d already in A (ignored).\n", value);
                break;

            case 2:
                value = read_int("Value to insert into B: ");
                status = avlset_insert(set_b, value, &flag);
                if (status != AVLSET_OK) printf("Error: %s\n", avlset_status_string(status));
                else printf(flag ? "Inserted %d into B.\n" : "%d already in B (ignored).\n", value);
                break;

            case 3:
                value = read_int("Value to delete from A: ");
                status = avlset_remove(set_a, value, &flag);
                printf(flag ? "Deleted %d from A.\n" : "%d not found in A.\n", value);
                break;

            case 4:
                value = read_int("Value to delete from B: ");
                status = avlset_remove(set_b, value, &flag);
                printf(flag ? "Deleted %d from B.\n" : "%d not found in B.\n", value);
                break;

            case 5:
                value = read_int("Value to search in A: ");
                printf("%d %s in A.\n", value, avlset_contains(set_a, value) ? "IS present" : "is NOT present");
                break;

            case 6:
                value = read_int("Value to search in B: ");
                printf("%d %s in B.\n", value, avlset_contains(set_b, value) ? "IS present" : "is NOT present");
                break;

            case 7:
                avlset_print(set_a, "A");
                break;

            case 8:
                avlset_print(set_b, "B");
                break;

            case 9:
                run_binary_op(avlset_union, set_a, set_b, "A U B", &last_result);
                break;

            case 10:
                run_binary_op(avlset_intersection, set_a, set_b, "A ^ B", &last_result);
                break;

            case 11:
                run_binary_op(avlset_difference, set_a, set_b, "A - B", &last_result);
                break;

            case 12:
                run_binary_op(avlset_difference, set_b, set_a, "B - A", &last_result);
                break;

            case 13:
                run_binary_op(avlset_symmetric_difference, set_a, set_b, "A (+) B", &last_result);
                break;

            case 14:
                printf("A is %sa subset of B.\n", avlset_is_subset(set_a, set_b) ? "" : "NOT ");
                break;

            case 15:
                printf("Sets A and B are %sequal.\n", avlset_is_equal(set_a, set_b) ? "" : "NOT ");
                break;

            case 16:
                read_line("Filepath to save A to: ", path, sizeof(path));
                status = avlset_save_to_file(set_a, path);
                printf(status == AVLSET_OK ? "Saved A to %s\n" : "Failed to save A: %s\n",
                       status == AVLSET_OK ? path : avlset_status_string(status));
                break;

            case 17:
                read_line("Filepath to save B to: ", path, sizeof(path));
                status = avlset_save_to_file(set_b, path);
                printf(status == AVLSET_OK ? "Saved B to %s\n" : "Failed to save B: %s\n",
                       status == AVLSET_OK ? path : avlset_status_string(status));
                break;

            case 18: {
                read_line("Filepath to load into A: ", path, sizeof(path));
                AVLSetStatus load_status;
                AVLSet *loaded = avlset_load_from_file(path, &load_status);
                if (loaded) {
                    avlset_destroy(set_a);
                    set_a = loaded;
                    printf("Loaded A from %s (size = %zu).\n", path, avlset_size(set_a));
                } else {
                    printf("Failed to load A: %s\n", avlset_status_string(load_status));
                }
                break;
            }

            case 19: {
                read_line("Filepath to load into B: ", path, sizeof(path));
                AVLSetStatus load_status;
                AVLSet *loaded = avlset_load_from_file(path, &load_status);
                if (loaded) {
                    avlset_destroy(set_b);
                    set_b = loaded;
                    printf("Loaded B from %s (size = %zu).\n", path, avlset_size(set_b));
                } else {
                    printf("Failed to load B: %s\n", avlset_status_string(load_status));
                }
                break;
            }

            case 20:
                if (!last_result) {
                    printf("No result yet -- run a set operation (9-13) first.\n");
                    break;
                }
                read_line("Filepath to export last result to: ", path, sizeof(path));
                read_line("Label for this export: ", label, sizeof(label));
                status = avlset_export_result(last_result, path, label);
                printf(status == AVLSET_OK ? "Exported last result to %s\n" : "Export failed: %s\n",
                       status == AVLSET_OK ? path : avlset_status_string(status));
                break;

            case 21: {
                int n = read_int("How many dataset files to import? ");
                if (n <= 0) {
                    printf("Nothing to import.\n");
                    break;
                }

                const char **paths = (const char **)malloc(sizeof(char *) * (size_t)n);
                char **owned = (char **)malloc(sizeof(char *) * (size_t)n);
                if (!paths || !owned) {
                    printf("Allocation failed.\n");
                    free(paths);
                    free(owned);
                    break;
                }

                for (int i = 0; i < n; i++) {
                    char buf[256];
                    char prompt[64];
                    snprintf(prompt, sizeof(prompt), "  File %d path: ", i + 1);
                    read_line(prompt, buf, sizeof(buf));
                    owned[i] = (char *)malloc(strlen(buf) + 1);
                    if (owned[i]) strcpy(owned[i], buf);
                    paths[i] = owned[i];
                }

                AVLSet **sets = NULL;
                size_t loaded_count = 0;
                AVLSetStatus import_status = avlset_import_multiple(paths, (size_t)n, &sets, &loaded_count);

                if (loaded_count > 0) {
                    printf("Successfully imported %zu of %d dataset(s):\n", loaded_count, n);
                    for (size_t i = 0; i < loaded_count; i++) {
                        char lbl[32];
                        snprintf(lbl, sizeof(lbl), "dataset_%zu", i + 1);
                        avlset_print(sets[i], lbl);
                    }
                    printf("(These are demo-only; use load A/B, option 18/19, to work with a specific file.)\n");
                } else {
                    printf("No datasets could be imported: %s\n", avlset_status_string(import_status));
                }

                avlset_free_multiple(sets, loaded_count);
                for (int i = 0; i < n; i++) free(owned[i]);
                free(owned);
                free(paths);
                break;
            }

            case 0:
                printf("Freeing memory and exiting...\n");
                break;

            default:
                printf("Invalid choice, try again.\n");
        }

    } while (choice != 0);

    avlset_destroy(set_a);
    avlset_destroy(set_b);
    avlset_destroy(last_result);

    return EXIT_SUCCESS;
}
