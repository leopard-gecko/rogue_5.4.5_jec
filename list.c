/*
 * Functions for dealing with linked lists of goodies
 *
 * @(#)list.c	4.12 (Berkeley) 02/05/99
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <stdlib.h>
#include <string.h>
#include <curses.h>
#include "rogue.h"

#ifdef MASTER
int total = 0;			/* total dynamic memory bytes */
#endif

/*
 * detach:
 *	takes an item out of whatever linked list it might be in
 */

void
_detach(THING **list, THING *item)
{
    if (*list == item)
	*list = next(item);
    if (prev(item) != NULL)
	item->l_prev->l_next = next(item);
    if (next(item) != NULL)
	item->l_next->l_prev = prev(item);
    item->l_next = NULL;
    item->l_prev = NULL;
}

/*
 * _attach:
 *	add an item to the head of a list
 */

void
_attach(THING **list, THING *item)
{
    if (*list != NULL)
    {
	item->l_next = *list;
	(*list)->l_prev = item;
	item->l_prev = NULL;
    }
    else
    {
	item->l_next = NULL;
	item->l_prev = NULL;
    }
    *list = item;
}

/*
 * _free_list:
 *	Throw the whole blamed thing away
 */

void
_free_list(THING **ptr)
{
    THING *item;

    while (*ptr != NULL)
    {
	item = *ptr;
	*ptr = next(item);
	discard(item);
    }
}

/*
 * _free_object_list:
 *     Release an object list, including per-object call names.  THING is
 *     also used for monsters, so the original _free_list() deliberately
 *     remains a plain node-list destructor.
 */
void
_free_object_list(THING **ptr)
{
    THING *obj;

    while (*ptr != NULL)
    {
        obj = *ptr;
        *ptr = next(obj);
        discard_object(obj);
    }
}

/*
 * discard_object:
 *     Every object owns its o_label, when present.  Monster nodes are
 *     freed with discard(), never with discard_object().
 */
void
discard_object(THING *obj)
{
    if (obj == NULL)
        return;
    /* The 'a' repeat-command bookkeeping can still refer to an item
     * that has just been consumed or removed from the level. */
    if (last_pick == obj)
        last_pick = NULL;
    if (l_last_pick == obj)
        l_last_pick = NULL;
    free(obj->o_label);
    obj->o_label = NULL;
    discard(obj);
}

/*
 * copy_object:
 *     A split-off stack member must own a separate copy of its call name.
 *     Copy first; on allocation failure the source object is untouched.
 */
THING *
copy_object(const THING *obj)
{
    THING *copy;

    if (obj == NULL)
        return NULL;
    copy = new_item();
    if (copy == NULL)
        return NULL;

    *copy = *obj;
    copy->l_next = NULL;
    copy->l_prev = NULL;
    copy->o_label = NULL;
    if (obj->o_label != NULL)
    {
        size_t length = strlen(obj->o_label) + 1;
        copy->o_label = malloc(length);
        if (copy->o_label == NULL)
        {
            discard(copy);
            return NULL;
        }
        memcpy(copy->o_label, obj->o_label, length);
    }
    return copy;
}

/*
 * discard:
 *     Release a THING node without interpreting its monster/object union.
 *     Use discard_object() when the node is known to be an object.
 */

void
discard(THING *item)
{
#ifdef MASTER
    total--;
#endif
    free(item);
}

/*
 * new_item
 *	Get a new item with a specified size
 */
THING *
new_item(void)
{
    THING *item;

	if ((item = calloc(1, sizeof *item)) == NULL) {
#ifdef MASTER
		msg("ran out of memory after %d items", total);
#endif
		return NULL;
	}

#ifdef MASTER
	total++;
#endif

	item->l_next = NULL;
    item->l_prev = NULL;
    return item;
}
