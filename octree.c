#include "octree.h"
#include <stdlib.h>
#include <string.h>


#define REPEAT8(x) {x; x; x; x; x; x; x; x;}

/* extern inline uint32_t _uvec3_index_to_index( */
/*         uint32_t uvec3_index, uint8_t oc_depth); */
node_t *node_construct()
{
    node_t *node = malloc(sizeof(node_t)); 

    node->is_full = 0;
    node->dom_leaf = 0;
    node->is_original = 0;
    node->level = 0;
    node->childreen = NULL;
    return node;
}


void node_create_childreen(node_t *node)
{
    node->is_full = false;

    node_t base_node = {
        .childreen=NULL,
        .is_full=true,
        .level=node->level + 1,
        .dom_leaf = node->dom_leaf
    };

    node->childreen = calloc(8, sizeof(node_t *));
    for (int i = 0; i < 8; i++) {
        node_t *child = node->childreen[i] = node_construct();
        *(child) = base_node;
    }
}


void node_r_initialize(node_t *node, uint8_t depth)
{
    int level = node->level;
    bool is_last_node = (level == depth - 1);

    if (is_last_node) {
        node->leaves = calloc(8, sizeof(leaf_t));
        memset(node->leaves, 0, sizeof(leaf_t) * 8);
        return ;
    }

    node->childreen = calloc(8, sizeof(node_t *));
    for (int i = 0; i < 8; i++) {
        node_t *child_node = node->childreen[i] = node_construct();

        child_node->level = level + 1;
        node_r_initialize(child_node, depth);
    }
}


node_t *node_get(node_t *node, uint32_t index, uint8_t level, uint8_t oc_depth)
{
    node_t *l_node = node;
    uint8_t c_level = node->level;
    uint32_t bit = (oc_depth - c_level - 1) * 3;

    for (; c_level < level; c_level++) {
        uint8_t c_index = (index >> bit) & 0x7;

        l_node = l_node->childreen[c_index];
        bit -= 3;
    }
    return l_node;
}


node_t *node_get_nearest(
        node_t *node, uint32_t index, uint8_t level, uint8_t oc_depth)
{
    node_t *l_node = node;

    uint8_t c_level = node->level;
    uint32_t bit = (oc_depth - c_level - 1) * 3;
    for (; c_level < level; c_level++) {
        uint8_t c_index = (index >> bit) & 0x7;

        if (l_node->is_full) break;
        if (!l_node->childreen) break;

        l_node = l_node->childreen[c_index];
        bit -= 3;
    }
    return l_node;
}


node_t *node_get_or_create(
        node_t *node, uint32_t index, uint8_t level, uint8_t oc_depth)
{
    node_t *l_node = node;

    uint8_t c_level = node->level;
    uint32_t bit = (oc_depth - c_level - 1) * 3;
    for (; c_level < level; c_level++) {
        uint8_t c_index = (index >> bit) & 0x7;

        if (l_node->is_full) {
            node_create_childreen(l_node);
        }

        l_node = l_node->childreen[c_index];
        bit -= 3;
    }
    return l_node;
}


void node_print(node_t *node, const char *name)
{
    printf("*(%s) = {\n"
            "\t.childreen = %p\n"
            "\t.is_full = %u,\n"
            "\t.is_original = %u,\n"
            "\t.level = %u,\n"
            "\t.dom_leaf = %u,\n"
            "}\n",
            name,
            node->childreen,
            node->is_full,
            node->is_original,
            node->level,
            node->dom_leaf);
}


void leaves_print(leaf_t *leaves, const char *name)
{
    printf("%s = {\n", name); 
    for (int i = 0; i < 8; i++) {
        printf((!(i % 4)) ? "\t" : "");
        printf("%d", leaves[i]);
        printf((!((i + 1) % 4)) ? "\n" : ", ");
    }
    printf("}\n");
}


int leaf_get(node_t *node, uint32_t index, uint8_t oc_depth)
{
    node_t *l_node = node_get_nearest(node, index, oc_depth - 1, oc_depth);

    int leaf = 
        (l_node->is_full)
            ? l_node->dom_leaf
            : l_node->leaves[index & 0x7];

    return leaf;
}


void leaf_set(node_t *node, uint32_t index, uint8_t oc_depth, leaf_t leaf)
{
    node_t *l_node = node_get_nearest(node, index, oc_depth - 1, oc_depth);
    bool is_last = (l_node->level == oc_depth - 1);

    if (l_node->is_full) {
        if (l_node->dom_leaf == leaf) return;

        l_node = (is_last)
            ? l_node
            : node_get_or_create(node, index, oc_depth - 1, oc_depth);

        l_node->is_full = false;
        l_node->leaves = calloc(8, sizeof(leaf_t));
        LEAVES_FILL(l_node->leaves, l_node->dom_leaf);
    }

    l_node->leaves[index & 0x7] = leaf;

    if (LEAVES_FULL_OF_LEAF(l_node->leaves, leaf)) {
        printf("IS FULL");
        free(l_node->leaves);
        l_node->leaves = NULL;
        l_node->is_full = true;
        l_node->dom_leaf = leaf;
    }
}


/* NOTE:
 * Recursive slow. Rewrite this function later.
 */
void node_r_free_last(node_t *node, uint8_t last, uint8_t depth)
{
    uint8_t level = node->level;
    bool is_local_last = (level == last - 1);
    bool is_last_node = (level == depth - 1);

    if (node->is_full) return;

    if (is_local_last) {
        if (is_last_node) {
            free(node->leaves);
            node->leaves = NULL;
        }
        else {
            for (int i = 0; i < 8; i++) {
                free(node->childreen[i]);
            }
            free(node->childreen);
            node->childreen = NULL;
        }
    }
    else {
        for (int i = 0; i < 8; i++) {
            node_r_free_last(node->childreen[i], last, depth);
        }
    }
}


void node_r_free_childreen(node_t *node, uint8_t depth)
{
    int l = node->level;
    for (int i = depth; i > l; i--) {
        node_r_free_last(node, i, depth);
    }
}


octree_t *octree_construct(uint8_t depth)
{
    octree_t *octree = malloc(sizeof(octree_t));

    octree->root = node_construct();
    octree->depth = depth;
    return octree;
}


void octree_r_initialize(octree_t *octree)
{
    node_r_initialize(octree->root, octree->depth);
}


void octree_r_free_childreen(octree_t *octree)
{
    node_r_free_childreen(octree->root, octree->depth);
}


node_t *octree_node_get(octree_t *octree, uint32_t index, uint8_t level)
{
    return node_get(octree->root, index, level, octree->depth);
}


node_t *octree_node_get_nearest(
        octree_t *octree, uint32_t index, uint8_t level)
{
    return node_get_nearest(octree->root, index, level, octree->depth);
}


node_t *octree_node_get_or_create(
        octree_t *octree, uint32_t index, uint8_t level)
{
    return node_get_or_create(octree->root, index, level, octree->depth);
}


int octree_leaf_get(octree_t *octree, uint32_t index)
{
    return leaf_get(octree->root, index, octree->depth);
}


void octree_leaf_set(octree_t *octree, uint32_t index, leaf_t leaf)
{
    leaf_set(octree->root, index, octree->depth, leaf);
}


void octree_free(octree_t *octree)
{
    if (octree->root) free(octree->root);
    if (octree) free(octree);
}


void node_r_save_f(node_t *node, uint8_t oc_depth, FILE *fp)
{
    simple_node_t snode = {
        .is_full = node->is_full,
        .is_original = node->is_original,
        .level = node->level,
        .dom_leaf = node->dom_leaf
    };

    bool is_last = (snode.level == oc_depth - 1);

    if (!fwrite(&snode, sizeof(snode), 1, fp)) printf("STRUCT NOT WRITTEN\n");
    node_print(node, "node");

    if (snode.is_full) return;

    if (is_last) {
        if (!fwrite(node->leaves, sizeof(leaf_t), 8, fp)) {
            printf("ARRAY NOT WRITTEN\n");
        }
        leaves_print(node->leaves, "node->leaves");
    }
    else {
        for (int i = 0; i < 8; i++) {
            node_r_save_f(node->childreen[i], oc_depth, fp);
        }
    }
}


/* Currently doesn't work */
uint32_t node_save_f(node_t *node, uint8_t oc_depth, FILE *fp)
{
    node_t *cnode = node;

    uint32_t i = 0, c = 0, bits_written = 0;
    uint32_t max_i = 1 << (oc_depth * 3);

    while (i < max_i) {
        simple_node_t snode = {
            .is_full = cnode->is_full,
            .is_original = cnode->is_original,
            .level = cnode->level,
            .dom_leaf = cnode->dom_leaf
        };

        bool is_last = (snode.level == oc_depth - 1);
        bool is_full = snode.is_full;
        uint8_t depth = oc_depth - snode.level;
        uint8_t levels = depth - (!(is_full || is_last));
        uint32_t increment = (is_full || is_last) * 1 << (levels * 3);
        uint32_t prev_i = i, nl;

        if (!fwrite(&snode, sizeof(snode), 1, fp)) {
            printf("Error: Couldn't write to file\n");
            break;
        }

        node_print(cnode, "cnode");
        bits_written += sizeof(snode);

        if (!is_full) {
            if (is_last) {
                if (!fwrite(cnode->leaves, sizeof(leaf_t), 8, fp)) {
                    printf("Error: Couldn't write to file\n");
                    break;
                }
                leaves_print(cnode->leaves, "cnode->leaves");
                bits_written += sizeof(leaf_t) * 8;
            }
        }
        i += increment;

        /* Next level to write to disk*/
        if (increment == 0) {
            nl = snode.level + 1;
        }
        else {
            uint32_t diff = i ^ prev_i;
            nl = 1;
            for (; nl < snode.level; nl++)
                if ((diff >> ((oc_depth - nl) * 3)) & 0x7)
                    break;
        }

        printf("increment %u | ni %u | i %u\n",
                increment, nl, i);
        cnode = node_get_nearest(node, i, nl, oc_depth);
        c++;
    }
    return bits_written;
}


/* NOTE:
 * This recursive functions aren't efficient
 * rewrite later
 */
void node_r_load_f(node_t *node, uint8_t oc_depth, FILE *fp)
{
    simple_node_t snode;

    if (!fread(&snode, sizeof(simple_node_t), 1, fp)) {
        printf("Error: Couldn't read\n");
        return;
    }
    node_print(node, "node");

    node->is_full = snode.is_full;
    node->is_original = snode.is_original;
    node->level = snode.level;
    node->dom_leaf = snode.dom_leaf;
    node->childreen = NULL;

    bool is_last = (oc_depth == snode.level + 1);

    if (snode.is_full) return;

    if (is_last) {
        node->leaves = calloc(8, sizeof(leaf_t));
        fread(node->leaves, sizeof(leaf_t), 8, fp);
        leaves_print(node->leaves, "node->leaves");
    }
    else {
        node_create_childreen(node);
        for (int i = 0; i < 8; i++) {
            node_r_load_f(node->childreen[i], oc_depth, fp);
        }
    }
}


uint32_t node_load_f(node_t *node, uint8_t oc_depth, FILE *fp)
{
    node_t *cnode = node;
    uint32_t i = 0, c = 0, bits_read = 0;

    uint32_t max_i = 1 << (oc_depth * 3);

    while (i < max_i) {
        simple_node_t snode;
        uint32_t levels, increment, depth;
        bool is_full, is_last;

        if (!fread(&snode, sizeof(snode), 1, fp)) {
            printf("Error: Couldn't read from file\n");
            break;
        }

        depth = oc_depth - snode.level;
        is_full = snode.is_full;
        is_last = (snode.level == oc_depth - 1);
        levels  = depth - (!(is_full || is_last));

        cnode->is_full = snode.is_full;
        cnode->is_original = snode.is_original;
        cnode->level = snode.level;
        cnode->dom_leaf = snode.dom_leaf;

        increment = (is_full || is_last) * 1 << (levels * 3);
        bits_read += sizeof(snode);

        if (!is_full) {
            if (is_last) {
                cnode->leaves = calloc(8, sizeof(leaf_t));

                if (!fread(cnode->leaves, sizeof(leaf_t), 8, fp)) {
                    printf("Couldn't read from file");
                    break;
                }
                bits_read += sizeof(leaf_t) * 8;
            }
            else {
                node_create_childreen(cnode);
            }
        }
        i += increment;

        cnode = node_get_nearest(node, i, oc_depth - 1, oc_depth);
        c++;
    }
    return bits_read;
}


uint32_t octree_load_f(octree_t *octree, FILE *fp)
{
    return node_load_f(octree->root, octree->depth, fp);
}


void octree_r_load_f(octree_t *octree, FILE *fp)
{
    node_r_load_f(octree->root, octree->depth, fp);
}


void octree_r_save_f(octree_t *octree, FILE *fp)
{
    node_r_save_f(octree->root, octree->depth, fp); 
}


uint32_t octree_save_f(octree_t *octree, FILE *fp)
{
    return node_save_f(octree->root, octree->depth, fp);
}
