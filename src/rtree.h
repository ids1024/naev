/*
 * See Licensing and Copyright notice in naev.h
 */


#ifndef RTREE_H
#  define RTREE_H

#include "pilot.h"

struct rtree;

struct rtree *rtree_create();
void rtree_free(struct rtree *tree);
void rtree_insert(struct rtree *tree, Pilot *pilot);

#endif
