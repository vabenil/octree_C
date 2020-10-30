#include "octree.h"


node_t *node_construct(void)
{
    node_t *node = (node_t *)malloc(sizeof(node_t));

    if (node) *node = (node_t) {{NULL}, 0, 0, 0, 0};
    return node;
}


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


/* Get nodes or create them if they don't exist */
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


void node_r_free(node_t *node, uint8_t depth)
{
    // Free all childreen nodes
    for (int i = depth; i > node->level; i--) {
        node_r_free_last(node, i, depth);
    }

    free(node);
}


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
        bool is_last;

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


octree_t *octree_construct(uint8_t depth)
{
    octree_t *octree = (octree_t *)malloc(sizeof(octree_t));

    if (octree) {
        octree->root = node_construct();
        octree->depth = depth;
    }
    return octree;
}


void octree_r_free(octree_t *octree)
{
    node_r_free(octree->root, octree->depth);
    free(octree);
}


/* octree_load_buffer
 * params:
 *      * octree - octree to write to.
 *      * buff - buffer containing the raw octree data.
 * description:
 *      * Attempt to load raw data into octree. On failure -1  is returned and
 *      the octree's root node is freed. Otherwise the bits read is returned
 */
int octree_load_buffer(octree_t *octree, const char *buff)
{
    return node_load_buffer(octree->root, octree->depth, buff);
}


int octree_save_buffer(octree_t *octree, char *buff)
{
    return node_save_buffer(octree->root, octree->depth, buff);
}
