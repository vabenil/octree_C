/*
 * Fast OCTREE_DEF
Octree library(not sparse octree)
 * Limitations:
 *  - The maximum depth for an octree is 8
*/
#ifndef OCTREE_H
#define OCTREE_H

#include <string.h>
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


OCTREE_DEF
node_t *node_construct(void)
{
    node_t *node = (node_t *)malloc(sizeof(node_t));

    *node = (node_t) {{NULL}, 0, 0, 0, 0};
    return node;
}


OCTREE_DEF
void node_init_childreen(node_t *node)
{
    node->is_full = 0;

    const uint8_t level = node->level + 1;
    node_t base_node = {{NULL}, 1, 1, level, node->dom_leaf};
    
    node->childreen = (node_t **)calloc(8, sizeof(node_t *));
    for (int i = 0; i < 8; i++) {
        node_t *child = node->childreen[i] = node_construct();
        *(child) = base_node;
    }
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
    leaves_fill(node->leaves, node->dom_leaf);
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
void leaf_set(node_t *node, uint32_t index, uint8_t oc_depth, leaf_t leaf)
{
    node_t *l_node = node_get_nearest(node, index, oc_depth - 1, oc_depth);
    bool is_last = (l_node->level == oc_depth - 1);

    if (l_node->is_full) {
        /* Nothing to be done */
        if (l_node->dom_leaf == leaf) return;

        l_node = (is_last)
            ? l_node
            : node_get_or_create(node, index, oc_depth - 1, oc_depth);

        node_leaves_init(l_node, l_node->dom_leaf);
    }

    l_node->leaves[index & 0x7] = leaf;

    /* This may be better to be done by seperate in a function called
     * node_optimize or something like that, along with optimization to nodes
    */
    if (leaves_full(l_node->leaves, leaf)) {
        free(l_node->leaves);
        l_node->leaves = NULL;
        l_node->is_full = true;
        l_node->dom_leaf = leaf;
    }
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
octree_t *octree_construct(uint8_t depth)
{
    octree_t *octree = (octree_t *)malloc(sizeof(octree_t));

    octree->root = node_construct();
    octree->depth = depth;
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
