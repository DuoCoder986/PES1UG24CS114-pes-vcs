#include "tree.h"
#include "index.h"
#include "pes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ─────────────────────────────────────────
// Comparator for sorting entries by name
// ─────────────────────────────────────────
static int cmp_tree_entries(const void *a, const void *b) {
    const TreeEntry *e1 = (const TreeEntry *)a;
    const TreeEntry *e2 = (const TreeEntry *)b;
    return strcmp(e1->name, e2->name);
}

// ─────────────────────────────────────────
// Serialize Tree → raw bytes
// MUST be sorted
// ─────────────────────────────────────────
int tree_serialize(const Tree *tree, void **data_out, size_t *len_out) {
    if (!tree || !data_out || !len_out) return -1;

    TreeEntry temp[MAX_TREE_ENTRIES];
    memcpy(temp, tree->entries, tree->count * sizeof(TreeEntry));

    // sort entries
    qsort(temp, tree->count, sizeof(TreeEntry), cmp_tree_entries);

    char *buffer = malloc(16384);
    int offset = 0;

    for (int i = 0; i < tree->count; i++) {
        char hash_hex[HASH_HEX_SIZE + 1];
        hash_to_hex(&temp[i].hash, hash_hex);

        int written = snprintf(buffer + offset,
                               16384 - offset,
                               "%o %s %s\n",
                               temp[i].mode,
                               hash_hex,
                               temp[i].name);

        if (written < 0 || offset + written >= 16384) {
            free(buffer);
            return -1;
        }

        offset += written;
    }

    *data_out = buffer;
    *len_out = offset;
    return 0;
}

// ─────────────────────────────────────────
// Parse raw bytes → Tree
// ─────────────────────────────────────────
int tree_parse(const void *data, size_t len, Tree *tree_out) {
    if (!data || !tree_out) return -1;

    char *copy = malloc(len + 1);
    memcpy(copy, data, len);
    copy[len] = '\0';

    tree_out->count = 0;

    char *line = strtok(copy, "\n");
    while (line) {
        TreeEntry *e = &tree_out->entries[tree_out->count];

        char hash_hex[HASH_HEX_SIZE + 1];

        sscanf(line, "%o %64s %255s",
               &e->mode,
               hash_hex,
               e->name);

        hex_to_hash(hash_hex, &e->hash);

        tree_out->count++;
        line = strtok(NULL, "\n");
    }

    free(copy);
    return 0;
}

// ─────────────────────────────────────────
// Build tree from index
// (matches your tree.h)
// ─────────────────────────────────────────
int tree_from_index(ObjectID *tree_id_out) {
    Index index;

    if (index_load(&index) != 0) {
        fprintf(stderr, "error: failed to load index\n");
        return -1;
    }

    if (index.count == 0) {
        fprintf(stderr, "error: index empty\n");
        return -1;
    }

    Tree tree;
    tree.count = 0;

    for (int i = 0; i < index.count; i++) {
        TreeEntry *e = &tree.entries[tree.count++];

        e->mode = index.entries[i].mode;
        e->hash = index.entries[i].hash;

        // IMPORTANT: only filename (not full path)
        const char *name = strrchr(index.entries[i].path, '/');
        if (name) name++;
        else name = index.entries[i].path;

        strcpy(e->name, name);
    }

    void *data;
    size_t len;

    if (tree_serialize(&tree, &data, &len) != 0) {
        return -1;
    }

    if (object_write(OBJ_TREE, data, len, tree_id_out) != 0) {
        free(data);
        return -1;
    }

    free(data);
    return 0;
}
