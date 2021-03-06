/*
 * Fast octree library(not sparse octree)
 * Limitations:
 *  - The maximum depth for an octree is 10
 * Important remark:
 *  - Indices are 4 byte unsigned integers containing a 3-bit number represeing
 *  the location of each node relative to it's parent.
 *
 *  Example:
 *      location in octree - (1, 2, 0)
 *
 *      root node:
 *          node:   <- position relative to parent 0
 *              node:
 *                  ...
 *              node:
 *                  ...
 *              node:   <- position relative to parent 2
 *                   leaf
 *                   leaf   <- position relative to parent 1
 *                  <repeat 6x>
 *              node:
 *                  ...
 *              <repeat 4x>
 *          <repeat 7x>
 *
 *      this index could be defined as:
 *      uint32_t index = 1 << 0 |  2 << 3 | 0 << 6;
 *
 *      this makes iterating an octree as easy as doing index++
 *
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
#define OCTREE_DEF extern
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
    uint8_t level       : 4;
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
    uint8_t level       : 4;
    leaf_t dom_leaf;
} simple_node_t;


OCTREE_DEF
node_t *node_construct(void);


OCTREE_DEF
int node_init_childreen(node_t *node);


/* Get nodes or create them if they don't exist */
OCTREE_DEF
node_t *node_get_or_create(
        node_t *node, uint32_t index, uint8_t level, uint8_t oc_depth);


/* Recrusively free the last level */
/* TODO: write a none recursive versino of this function */
OCTREE_DEF
void node_r_free_last(node_t *node, uint8_t last_level, uint8_t depth);


OCTREE_DEF
void node_r_free(node_t *node, uint8_t depth);


OCTREE_DEF
int node_save_buffer(node_t *node, uint8_t oc_depth, char *buff);


OCTREE_DEF
int node_load_buffer(node_t *node, uint8_t oc_depth, const char *buff);


OCTREE_DEF
octree_t *octree_construct(uint8_t depth);


OCTREE_DEF
void octree_r_free(octree_t *octree);


/* octree_load_buffer
 * params:
 *      * octree - octree to write to.
 *      * buff - buffer containing the raw octree data.
 * description:
 *      * Attempt to load raw data into octree. On failure -1  is returned and
 *      the octree's root node is freed. Otherwise the bits read is returned
 */
OCTREE_DEF
int octree_load_buffer(octree_t *octree, const char *buff);


OCTREE_DEF
int octree_save_buffer(octree_t *octree, char *buff);


/* TODO: rename this function to something better */
OCTREE_INLINE
uint32_t _octree_i3d_to_uint(uint32_t x)
{
    return ((x) & 0x1) | (((x) & 0x2) << 9) | (((x) & 0x4) << 18);
}


/* TODO: rename this function to something better */
OCTREE_INLINE
uint32_t _octree_uint_to_i3d(uint32_t x) {
    return ((x) & 0x1) | (((x) >> 9) & 0x2) | (((x) >> 18) & 0x4);
}


OCTREE_INLINE
uint32_t octree_pack_pos(int p[3])
{
    return (((uint32_t)p[0] & 0x3FF) |
            ((uint32_t)p[1] & 0x3FF) << 10 |
            ((uint32_t)p[2] & 0x3FF) << 20);
}


OCTREE_INLINE
void octree_unpack_pos(uint32_t x, int pos[3])
{
    pos[0] = (x & 0x3FF);
    pos[1] = (x & 0xFFC00) >> 10;
    pos[2] = (x & 0x3FF00000) >> 20;
}


OCTREE_INLINE
uint32_t octree_packed_pos_to_index(uint32_t packed, uint8_t oc_depth)
{
    uint32_t index = 0,
             bit = 0;
    for (int i = 0; i < oc_depth; i++) {
        index += _octree_uint_to_i3d(packed >> i) << bit;
        bit += 3;
    }
    return index;
}


OCTREE_INLINE
uint32_t octree_index_to_packed_pos(uint32_t index, uint8_t oc_depth)
{
    uint32_t packed = 0,
             bit = 0;
    for (int i = 0; i < oc_depth; i++) {
        packed += _octree_i3d_to_uint(index >> bit) << i;
        bit += 3;
    }
    return packed;
}


OCTREE_INLINE
uint32_t octree_pos_to_index(int pos[3], uint8_t oc_depth)
{
    uint32_t packed = octree_pack_pos(pos);

    return octree_packed_pos_to_index(packed, oc_depth);
}


OCTREE_INLINE
void octree_index_to_pos(uint32_t index, int pos[3], uint8_t oc_depth)
{
    uint32_t packed = octree_index_to_packed_pos(index, oc_depth);

    octree_unpack_pos(packed, pos);
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


OCTREE_INLINE
bool leaves_full(leaf_t *leaves, leaf_t leaf)
{
    return memcmp(leaves, LEAVES(leaf), sizeof(leaf_t) * 8) == 0;
}


OCTREE_INLINE
void leaves_fill(leaf_t *leaves, leaf_t leaf)
{
    leaves[0] = leaves[1] =
    leaves[2] = leaves[3] =
    leaves[4] = leaves[5] =
    leaves[6] = leaves[7] = leaf;
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
void node_leaves_init(node_t *node, leaf_t leaf)
{
    node->is_full = false;
    node->leaves = (leaf_t *)calloc(8, sizeof(leaf_t));

    if (node->leaves) leaves_fill(node->leaves, node->dom_leaf);
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
        success = 1;

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
int octree_leaf_set(octree_t *octree, uint32_t index, leaf_t leaf)
{
    return leaf_set(octree->root, index, octree->depth, leaf);
}

#endif /* OCTREE_H */
