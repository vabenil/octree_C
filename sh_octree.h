/*
 * Fast octree library(not sparse octree)
 * Limitations:
 *  - The maximum depth for an octree is 8
*/
#ifndef OCTREE_H
#define OCTREE_H

#if defined(_MSC_VER)
#pragma warning(push)
// Stop MSVC complaining about not inlining functions.
#pragma warning(disable : 4710)
// Stop MSVC complaining about inlining functions!
#pragma warning(disable : 4711)
#elif defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#endif

/* TODO: add used flag in function defs to prevent gcc from complaining */
#if defined(_MSC_VER)
#define OCTREE_USED
#elif defined(__GNUC__)
#define OCTREE_USED __attribute__((used))
#else
#define OCTREE_USED
#endif


#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>


#ifndef OCTREE_INLINE
#define OCTREE_INLINE static inline
#endif /* OCTREE_INLINE */


#ifndef OCTREE_DEF
#define OCTREE_DEF static
#endif /* OCTREE_DEF */


#ifndef OCTREE_LEAF_TYPE
#define OCTREE_LEAF_TYPE uint16_t
#endif /* OCTREE_LEAF_TYPE */


#define LEAVES_INIT(leaf) {leaf, leaf, leaf, leaf, leaf, leaf, leaf, leaf}

#define LEAVES(leaf) ((leaf_t [8]) LEAVES_INIT(leaf))


typedef OCTREE_LEAF_TYPE leaf_t;


typedef struct node_s
{
    union
    {
        struct node_s **childreen;
        leaf_t *leaves;
    };
    bool is_full        : 1;
    bool is_original    : 1;
    uint8_t level       : 3;
    leaf_t dom_leaf;
} node_t;


typedef struct
{
    node_t *root;
    uint8_t depth;
} octree_t;


typedef struct {
    bool is_full        : 1;
    bool is_original    : 1;
    uint8_t level       : 3;
    leaf_t dom_leaf;
} simple_node_t;


OCTREE_DEF
node_t *node_construct(void)
{
    node_t *node = (node_t *)malloc(sizeof(node_t));

    if (node) *node = (node_t) {{NULL}, 0, 0, 0, 0};
    return node;
}


OCTREE_DEF
int node_init_childreen(node_t *node)
{
    int success = 0;
    const uint8_t level = node->level + 1;
    node_t base_node = {{NULL}, 1, 1, level, node->dom_leaf};
    
    node->childreen = (node_t **)calloc(8, sizeof(node_t *));
    node->is_full = 0;

    if (node->childreen) {
        for (int i = 0; i < 8; i++) {
            node_t *child = node->childreen[i] = node_construct();

            if (child) {
                *(child) = base_node;
                success = 1;
            }
        }
    }
    return success;
}


/* Naive function, shouldn't be used */
OCTREE_INLINE
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


/* Get the nearest node to the level */
OCTREE_INLINE
node_t *node_get_nearest(
        node_t *node, uint32_t index, uint8_t level, uint8_t oc_depth)
{
    node_t *l_node = node;

    uint8_t c_level = node->level;
    uint32_t bit = (oc_depth - c_level - 1) * 3;
    for (; c_level < level; c_level++) {
        uint8_t c_index = (index >> bit) & 0x7;

        if (l_node->is_full || !l_node->childreen) break;

        l_node = l_node->childreen[c_index];
        bit -= 3;
    }
    return l_node;
}


/* Get nodes or create them if they don't exist */
OCTREE_DEF
node_t *node_get_or_create(
        node_t *node, uint32_t index, uint8_t level, uint8_t oc_depth)
{
    node_t *l_node = node;

    uint8_t c_level = node->level;
    uint32_t bit = (oc_depth - c_level - 1) * 3;
    for (; c_level < level; c_level++) {
        uint8_t c_index = (index >> bit) & 0x7;

        if (l_node->is_full) {
            node_init_childreen(l_node);
        }

        l_node = l_node->childreen[c_index];
        bit -= 3;
    }
    return l_node;
}


/* Recrusively free the last level */
/* TODO: write a none recursive versino of this function */
OCTREE_DEF
void node_r_free_last(node_t *node, uint8_t last_level, uint8_t depth)
{
    uint8_t level = node->level;
    bool is_local_last = (level == last_level - 1);
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
            node_r_free_last(node->childreen[i], last_level, depth);
        }
    }
}


OCTREE_DEF
void node_r_free(node_t *node, uint8_t depth)
{
    // Free all childreen nodes
    for (int i = depth; i > node->level; i--) {
        node_r_free_last(node, i, depth);
    }

    free(node);
}


OCTREE_DEF
int node_save_buffer(node_t *node, uint8_t oc_depth, char *buff)
{
    node_t *cnode = node;
    uint32_t i = 0, c = 0,
             bits_written = 0, ofs = 0,
             max_i = 1 << (oc_depth * 3);

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
        uint32_t increment = (is_full || is_last) << (levels * 3);
        uint32_t prev_i = i, nl;

        memcpy(buff + ofs, &snode, sizeof(snode));
        ofs += sizeof(snode);

        bits_written += sizeof(snode);

        if (!is_full) {
            if (is_last) {
                memcpy(buff + ofs, cnode->leaves, sizeof(leaf_t [8]));
                ofs += sizeof(leaf_t [8]);

                bits_written += sizeof(leaf_t [8]);
            }
        }
        i+= increment;

        /* Next level to write */
        if (increment == 0) {
            nl = snode.level + 1;
        }
        else {
            uint32_t diff = 1 ^ prev_i;
            nl = 1;
            for (; nl < snode.level; nl++)
                if ((diff >> ((oc_depth - nl) * 3)) & 0x7) break;
        }
        cnode = node_get_nearest(node, i, nl, oc_depth);
        c++;
    }

    return bits_written;
}


OCTREE_DEF
int node_load_buffer(node_t *node, uint8_t oc_depth, const char *buff)
{
    node_t *cnode = node;
    uint32_t i = 0, c = 0,
             bits_read = 0, ofs = 0,
             max_i = 1 << (oc_depth * 3);

    while (i < max_i) {
        simple_node_t snode;
        uint32_t levels, increment, depth;
        bool is_full, is_last;

        memcpy(&snode, buff + ofs, sizeof(snode));

        ofs += sizeof(snode);
        bits_read += sizeof(snode);

        depth = oc_depth - snode.level;
        is_last = (snode.level == oc_depth - 1);
        levels = depth - (!(snode.is_full || is_last));

        cnode->is_full = snode.is_full;
        cnode->is_original = snode.is_original;
        cnode->level = snode.level;
        cnode->dom_leaf = snode.dom_leaf;

        increment = (snode.is_full || is_last) << (levels * 3);
        
        if (!snode.is_full) {
            if (is_last) {
                cnode->leaves = (leaf_t *)calloc(8, sizeof(leaf_t));

                if (cnode->leaves == NULL) return -1;

                memcpy(cnode->leaves, buff + ofs, sizeof(leaf_t [8]));

                ofs += sizeof(leaf_t [8]);
                bits_read += sizeof(leaf_t [8]);
            }
            else {
                if (!node_init_childreen(cnode)) return -1;
            }
        }
        i += increment;

        cnode = node_get_nearest(node, i, oc_depth - 1, oc_depth);
        c++;
    }
    return bits_read;
}


OCTREE_INLINE
bool leaves_full(leaf_t *leaves, leaf_t leaf)
{
    return memcmp(leaves, LEAVES(leaf), sizeof(leaf_t) * 8) == 0;
}


OCTREE_INLINE
void leaves_fill(leaf_t *leaves, leaf_t leaf)
{
    leaves[0] = leaf; leaves[1] = leaf;
    leaves[2] = leaf; leaves[3] = leaf;
    leaves[4] = leaf; leaves[5] = leaf;
    leaves[6] = leaf; leaves[7] = leaf;
}


OCTREE_INLINE
void node_leaves_init(node_t *node, leaf_t leaf)
{
    node->is_full = false;
    node->leaves = (leaf_t *)calloc(8, sizeof(leaf_t));

    if (node->leaves) leaves_fill(node->leaves, node->dom_leaf);
}


OCTREE_INLINE
int leaf_get(node_t *node, uint32_t index, uint8_t oc_depth)
{
    /* Get node at the last level*/
    node_t *l_node = node_get_nearest(node, index, oc_depth - 1, oc_depth);

    int leaf =
        (l_node->is_full)
            ? l_node->dom_leaf
            : l_node->leaves[index & 0x7];

    return leaf;
}


OCTREE_INLINE
int leaf_set(node_t *node, uint32_t index, uint8_t oc_depth, leaf_t leaf)
{
    int success = 0;
    node_t *l_node = node_get_nearest(node, index, oc_depth - 1, oc_depth);
    bool is_last = (l_node->level == oc_depth - 1);

    if (l_node->is_full) {
        /* Nothing to be done */
        if (l_node->dom_leaf == leaf) return 1;

        l_node = (is_last)
            ? l_node
            : node_get_or_create(node, index, oc_depth - 1, oc_depth);

        node_leaves_init(l_node, l_node->dom_leaf);
    }

    if (l_node->leaves) {
        l_node->leaves[index & 0x7] = leaf;
        sucess = 1;

        /* TODO: Add node_optimize function */
        if (leaves_full(l_node->leaves, leaf)) {
            free(l_node->leaves);
            l_node->leaves = NULL;
            l_node->is_full = true;
            l_node->dom_leaf = leaf;
        }
    }

    return success;
}



OCTREE_DEF
octree_t *octree_construct(uint8_t depth)
{
    octree_t *octree = (octree_t *)malloc(sizeof(octree_t));

    if (octree) {
        octree->root = node_construct();
        octree->depth = depth;
    }
    return octree;
}


OCTREE_DEF
void octree_r_free(octree_t *octree)
{
    node_r_free(octree->root, octree->depth);
    free(octree);
}


OCTREE_INLINE
node_t *octree_node_get(octree_t *octree, uint32_t index, uint8_t level)
{
    return node_get(octree->root, index, level, octree->depth);
}


OCTREE_INLINE
node_t *octree_node_get_nearest(octree_t *octree, uint32_t index, uint8_t level)
{
    return node_get_nearest(octree->root, index, level, octree->depth);
}


OCTREE_INLINE
node_t *octree_node_get_or_create(
        octree_t *octree, uint32_t index, uint8_t level)
{
    return node_get_or_create(octree->root, index, level, octree->depth);

}


/* octree_load_buffer
 * params:
 *      * octree - octree to write to.
 *      * buff - buffer containing the raw octree data.
 * description:
 *      * Attempt to load raw data into octree. On failure -1  is returned and
 *      the octree's root node is freed. Otherwise the bits read is returned
 */
OCTREE_DEF
int octree_load_buffer(octree_t *octree, const char *buff)
{
    return node_load_buffer(octree->root, octree->depth, buff);
}


OCTREE_DEF
itn octree_save_buffer(octree_t *octree, char *buff)
{
    return node_save_buffer(octree->root, octree->depth, buff);
}


OCTREE_INLINE
leaf_t octree_leaf_get(octree_t *octree, uint32_t index)
{
    return leaf_get(octree->root, index, octree->depth);
}


OCTREE_INLINE
void octree_leaf_set(octree_t *octree, uint32_t index, leaf_t leaf)
{
    leaf_set(octree->root, index, octree->depth, leaf);
}
#endif /* OCTREE_H */
