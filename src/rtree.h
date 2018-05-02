/*
 * See Licensing and Copyright notice in naev.h
 */


#ifndef RTREE_H
#  define RTREE_H

#include "pilot.h"

struct rtree;
struct rtree_iter;

struct rtree *rtree_create();
void rtree_free(struct rtree *tree);
void rtree_insert(struct rtree *tree, Pilot *pilot);
struct rtree_iter *rtree_begin(struct rtree *tree);
void rtree_iter_free(struct rtree_iter *iter);
Pilot* rtree_find(struct rtree_iter *iter, double x1, double x2, double y1, double y2);
void rtree_draw(struct rtree *tree, double res);

#endif
