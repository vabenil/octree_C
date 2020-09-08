#ifndef OCTREE_H
#define OCTREE_H

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>


#define LEAVES_FILL(leaves, val) {\
    leaves[0] = val; leaves[1] = val;\
    leaves[2] = val; leaves[3] = val;\
    leaves[4] = val; leaves[5] = val;\
    leaves[6] = val; leaves[7] = val;\
}


#define LEAVES_FROM_LEAF_INIT(leaf) {\
    leaf, leaf, leaf, leaf, leaf, leaf, leaf, leaf\
}

#define LEAVES_FROM_LEAF(leaf) ((leaf_t [8]) LEAVES_FROM_LEAF_INIT(leaf))


#define LEAVES_FULL_OF_LEAF(leaves, leaf) \
    (memcmp(leaves, LEAVES_FROM_LEAF(leaf), sizeof(leaf_t) * 8) == 0)

#define LEAVES_FULL(leaves) LEAVES_FULL_OF_LEAF(leaves, leaves[0])


typedef uint16_t leaf_t;

typedef struct node_struct {
    union {
        struct node_struct **childreen;
        leaf_t *leaves;
    };
    bool is_full     : 1;
    bool is_original : 1;
    uint8_t level    : 3;
    leaf_t dom_leaf  : 10;
} node_t;

typedef struct {
    node_t *root;
    uint8_t depth;
} octree_t;


typedef struct {
    uint8_t depth   : 3;
    uint32_t size;
} simple_octree_t;

typedef struct {
    bool is_full        : 1;
    bool is_original    : 1;
    uint8_t level       : 3;
    leaf_t dom_leaf     : 10;
} simple_node_t;


extern node_t *node_construct(void);

/* extern void node_r_initialize(node_t *node, uint8_t depth); */
 
/* naive function, get_nearest or get_or_create should be used instead */
extern node_t *node_get(
        node_t *node, uint32_t index, uint8_t level, uint8_t oc_depth);

extern node_t *node_get_nearest(
        node_t *node, uint32_t index, uint8_t level, uint8_t oc_depth);

extern node_t *node_get_or_create(
        node_t *node, uint32_t index, uint8_t level, uint8_t oc_depth);

extern void node_r_free_last(node_t *node, uint8_t last, uint8_t depth);

extern void node_r_free_childreen(node_t *node, uint8_t depth);


extern int leaf_get(node_t *node, uint32_t index, uint8_t oc_depth);

extern void leaf_set(
        node_t *node, uint32_t index, uint8_t oc_depth, leaf_t leaf);

extern octree_t *octree_construct(uint8_t depth);

extern void octree_r_initialize(octree_t *octree);

extern node_t *octree_node_get(
        octree_t *octree, uint32_t index, uint8_t level);

extern node_t *octree_node_get_nearest(
        octree_t *octree, uint32_t index, uint8_t level);

extern node_t *octree_node_get_or_create(
        octree_t *octree, uint32_t index, uint8_t level);

extern int octree_leaf_get(octree_t *octree, uint32_t index);

extern void octree_leaf_set(octree_t *octree, uint32_t index, leaf_t leaf);

extern void octree_r_free_childreen(octree_t *octree);
extern void octree_free(octree_t *octree);

extern uint32_t octree_load_f(octree_t *octree, FILE *fp);

extern uint32_t octree_save_f(octree_t *octree, FILE *fp);


#endif /* OCTREE_H */
