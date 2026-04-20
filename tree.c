#include "tree.h"
#include "index.h"
#include "pes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int tree_from_index(ObjectID *tree_id_out) {
    Index index;

    // Load index internally (IMPORTANT)
    if (index_load(&index) != 0) {
        fprintf(stderr, "error: failed to load index\n");
        return -1;
    }

    if (index.count == 0) {
        fprintf(stderr, "error: index empty\n");
        return -1;
    }

    char buffer[16384];
    int offset = 0;

    for (int i = 0; i < index.count; i++) {
        char hash_hex[HASH_HEX_SIZE + 1];
        hash_to_hex(&index.entries[i].hash, hash_hex);

        int written = snprintf(buffer + offset,
                               sizeof(buffer) - offset,
                               "%o %s %s\n",
                               index.entries[i].mode,
                               hash_hex,
                               index.entries[i].path);

        if (written < 0 || offset + written >= (int)sizeof(buffer)) {
            fprintf(stderr, "error: tree buffer overflow\n");
            return -1;
        }

        offset += written;
    }

    if (object_write(OBJ_TREE, buffer, offset, tree_id_out) != 0) {
        fprintf(stderr, "error: failed to write tree\n");
        return -1;
    }

    return 0;
}
