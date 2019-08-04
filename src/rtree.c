#define NODE_LENGTH 5

#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include "rtree.h"
#include "physics.h"
#include "opengl_render.h"

struct bounding_rectangle {
   double x1, x2, y1, y2;
};

const glColour cInternal = { .r=1., .g=0., .b=0., .a=.25 };
const glColour cLeaf = { .r=0., .g=1., .b=0., .a=.25 };
const glColour cPilot = { .r=1., .g=1., .b=1., .a=1. };

struct rtree_entry {
   struct bounding_rectangle mbr;
   union {
      Pilot *pilot;
      struct rtree_node *node;
   } value;
};

struct rtree_node {
   enum {
      INTERNAL_NODE,
      LEAF_NODE
   } type;
   int length;
   struct bounding_rectangle mbr;
   struct rtree_entry entries[NODE_LENGTH];
};

struct rtree {
   struct rtree_node *root;
   int height;
};

struct rtree_iter_item {
   struct rtree_node *node;
   int index;
};

struct rtree_iter {
   struct rtree_iter_item *items;
   int count;
};

struct rtree *rtree_create() {
   struct rtree_node *root;
   struct rtree *tree;
      
   root = malloc(sizeof(struct rtree_node));
   root->type = LEAF_NODE;
   root->length = 0;

   tree = malloc(sizeof(struct rtree));
   tree->root = root;
   tree->height = 0;

   return tree;
}

static void rtree_free_node(struct rtree_node *node) {
   int i;
   if (node->type == INTERNAL_NODE) {
      for (i=0; i < node->length; i++)
         rtree_free_node(node->entries[i].value.node);
   }

   free(node);
}

void rtree_free(struct rtree *tree) {
   rtree_free_node(tree->root);
   free(tree);
}

static struct bounding_rectangle mbr_add(struct bounding_rectangle mbr1, struct bounding_rectangle mbr2) {
   struct bounding_rectangle mbr;
   mbr.x1 = fmin(mbr1.x1, mbr2.x1);
   mbr.x2 = fmax(mbr1.x2, mbr2.x2);
   mbr.y1 = fmin(mbr1.y1, mbr2.y1);
   mbr.y2 = fmax(mbr1.y2, mbr2.y2);
   return mbr;
}

static double mbr_area(struct bounding_rectangle mbr) {
   return (mbr.x2 - mbr.x1) * (mbr.y2 - mbr.y1);
}

static double mbr_distance(struct bounding_rectangle mbr1, struct bounding_rectangle mbr2) {
   return hypot((mbr1.x1 + mbr1.x2) / 2 - (mbr2.x1 + mbr2.x2) / 2,
                (mbr1.y1 + mbr1.y2) / 2 - (mbr2.y1 + mbr2.y2) / 2);
}

static int mbr_interesect(struct bounding_rectangle mbr1, struct bounding_rectangle mbr2) {
   return !(mbr1.x2 < mbr2.x1 || mbr2.x2 < mbr1.x1 ||
            mbr1.y2 < mbr2.y1 || mbr2.y2 < mbr1.y1);
}

static struct rtree_node *rtree_node_split(struct rtree_node *node, struct rtree_entry entry) {
   int i, j, distance, best_distance, best_i, best_j;
   struct rtree_node *new_node;
   struct bounding_rectangle mbr1, mbr2;
   struct rtree_entry tmp_entry;

   assert(node->length == NODE_LENGTH);

   new_node = malloc(sizeof(struct rtree_node));
   new_node->type = node->type;
   
   // TODO handle ties
   best_distance = -1;
   for (i = -1; i < NODE_LENGTH - 1; i++) {
      for (j = i+1; j < NODE_LENGTH; j++) {
         /* Use -1 to represent the value being inserted */
         if (i == -1)
            distance = mbr_distance(entry.mbr, node->entries[j].mbr);
         else
            distance = mbr_distance(node->entries[i].mbr, node->entries[j].mbr);
         if (distance > best_distance) {
            best_i = i;
            best_j = j;
            best_distance = distance;
         }
      }
   }

   if (best_j == -1) {
      new_node->entries[0] = entry;
   } else {
      new_node->entries[0] = node->entries[best_j];

      node->entries[best_j] = entry;

      if (best_i == -1)
         best_i = best_j;
   }

   /* swap node->entries[0] and node->entries[best_i] */
   tmp_entry = node->entries[0];
   node->entries[0] = node->entries[best_i];
   node->entries[best_i] = tmp_entry;

   new_node->mbr = new_node->entries[0].mbr;
   new_node->length = 1;
   node->mbr = node->entries[0].mbr;
   node->length = 1;

   for (i = 1; i < NODE_LENGTH; i++) {
      mbr1 = mbr_add(node->mbr, node->entries[i].mbr);
      mbr2 = mbr_add(new_node->mbr, node->entries[i].mbr);
      if (mbr_area(mbr2) - mbr_area(new_node->mbr) < mbr_area(mbr1) - mbr_area(node->mbr)) {
         new_node->entries[new_node->length++] = node->entries[i];
         new_node->mbr = mbr2;
      } else {
         node->entries[node->length++] = node->entries[i];
         node->mbr = mbr1;
      }
   }

   return new_node;
}

static struct rtree_node *rtree_node_insert(struct rtree_node *node, struct bounding_rectangle mbr, Pilot *pilot) {
   int best_fit, i;
   double best_fit_increase, increase;
   struct rtree_node *new_node;

   struct rtree_entry entry;
   entry.mbr = mbr;
   entry.value.pilot = pilot;

   if (node->type == LEAF_NODE) {
      if (node->length < NODE_LENGTH) {
         node->entries[node->length] = entry;
         node->length++;
         node->mbr = mbr_add(node->mbr, mbr);
         return NULL;
      } else {
         return rtree_node_split(node, entry);
      }
   } else {
      best_fit_increase = INFINITY;
      // TODO handle ties
      for (i=0; i < node->length; i++) {
         increase = mbr_area(mbr_add(node->entries[i].mbr, mbr)) - mbr_area(node->entries[i].mbr);
         if (increase < best_fit_increase) {
            best_fit_increase = increase;
            best_fit = i;
         }
      }
      new_node = rtree_node_insert(node->entries[best_fit].value.node, mbr, pilot);
      node->entries[best_fit].mbr = node->entries[best_fit].value.node->mbr;

      if (new_node != NULL) {
         if (node->length < NODE_LENGTH) {
            node->entries[node->length].mbr = new_node->mbr;
            node->entries[node->length].value.node = new_node;
            node->length++;
         } else {
            struct rtree_entry entry;
            entry.mbr = mbr;
            entry.value.node = new_node;
            return rtree_node_split(node, entry);
         }
      }

      node->mbr = mbr_add(node->mbr, mbr);
      return NULL;
   }
}

void rtree_insert(struct rtree *tree, Pilot *pilot) {
   struct bounding_rectangle mbr;
   glTexture *gfx_space;
   struct rtree_node *new_node, *new_root;

   gfx_space = pilot->ship->gfx_space;
   mbr.x1 = VX(pilot->solid->pos) - (gfx_space->sw / 2);
   mbr.x2 = VX(pilot->solid->pos) + (gfx_space->sw / 2);
   mbr.y1 = VY(pilot->solid->pos) - (gfx_space->sh / 2);
   mbr.y2 = VY(pilot->solid->pos) + (gfx_space->sh / 2);

   new_node = rtree_node_insert(tree->root, mbr, pilot);
   if (new_node != NULL) {
      new_root = malloc(sizeof(struct rtree_node));
      new_root->type = INTERNAL_NODE;
      new_root->length = 2;

      new_root->entries[0].value.node = tree->root;
      new_root->entries[0].mbr = tree->root->mbr;
      new_root->entries[1].value.node = new_node;
      new_root->entries[1].mbr = new_node->mbr;

      tree->root = new_root;
      tree->height++;
   }
}

struct rtree_iter *rtree_begin(struct rtree *tree) {
   struct rtree_iter *iter = malloc(sizeof(struct rtree_iter));
   iter->count = tree->height;
   iter->items = malloc((tree->height + 1) * sizeof(struct rtree_iter_item));
   iter->items[0].node = tree->root;
   iter->items[0].index = -1;
   return iter;
}

void rtree_iter_free(struct rtree_iter *iter) {
   free(iter->items);
   free(iter);
}

Pilot* rtree_find(struct rtree_iter *iter, double x1, double x2, double y1, double y2) {
   int level, i;
   struct rtree_node *node;
   struct bounding_rectangle mbr = {x1, x2, y1, y2};

   /* Start of iteration */
   if (iter->items[0].index == -1)
      level = 0;
   else
      level = iter->count - 1;

OUTER:
   while (level >= 0) {
      i = iter->items[level].index + 1;
      node = iter->items[level].node;

      for (; i < node->length; i++) {
         if (mbr_interesect(mbr, node->entries[i].mbr)) {
            iter->items[level].index = i;
            if (node->type == LEAF_NODE) {
               return node->entries[i].value.pilot;
            } else {
               level++;
               iter->items[level].node = node->entries[i].value.node;
               iter->items[level].index = -1;
               goto OUTER;
            }
         }
      }

      level--;
   }

   return NULL;
}

static void mbr_draw(struct bounding_rectangle mbr, double res, const glColour *c) {
   gl_renderRectEmpty(mbr.x1 / res + SCREEN_W / 2,
		      mbr.y1 / res + SCREEN_H / 2,
		      (mbr.x2 - mbr.x1) / res,
		      (mbr.y2 - mbr.y1) / res,
		      c);
}

static void rtree_node_draw(struct rtree_node *node, double res) {
   int i;

   if (node->type == INTERNAL_NODE)
      mbr_draw(node->mbr, res, &cInternal);
   else
      mbr_draw(node->mbr, res, &cLeaf);

   for (i = 0; i < node->length; i++) {
      if (node->type == INTERNAL_NODE) {
         rtree_node_draw(node->entries[i].value.node, res);
      } else {
         mbr_draw(node->entries[i].mbr, res, &cPilot);
      }
   }
}

void rtree_draw(struct rtree *tree, double res) {
   rtree_node_draw(tree->root, res);
}
