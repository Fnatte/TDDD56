/*
 * stack.c
 *
 *  Created on: 18 Oct 2011
 *  Copyright 2011 Nicolas Melot
 *
 * This file is part of TDDD56.
 *
 *     TDDD56 is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     TDDD56 is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with TDDD56. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef DEBUG
#define NDEBUG
#endif

#include <assert.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdatomic.h>

#include "stack.h"
#include "non_blocking.h"

#if NON_BLOCKING == 0
#warning Stacks are synchronized through locks
#else
#if NON_BLOCKING == 1
#warning Stacks are synchronized through hardware CAS
#else
#warning Stacks are synchronized through lock-based CAS
#endif
#endif

static stack_item_t pool[MAX_PUSH_POP];
_Atomic static size_t cur;

void
stack_init()
{
	cur = 0;
}

void
stack_check(stack_t* stack)
{
	// Do not perform any sanity check if performance is bein measured
#if MEASURE == 0
	// Use assert() to check if your stack is in a state that makes sens
	// This test should always pass
	assert(1 == 1);

	// This test fails if the task is not allocated or if the allocation failed
	assert(stack != NULL);
#endif
}

void stack_push(stack_t* stack, int val) {
	stack_item_t* new_item = &pool[cur++];
	new_item->val = val;

#if NON_BLOCKING == 0

	pthread_mutex_lock(&stack->lock);
	new_item->prev = stack->head;
	stack->head = new_item;
	pthread_mutex_unlock(&stack->lock);

#elif NON_BLOCKING == 1

	do {
		new_item->prev = stack->head;
	} while (!__sync_bool_compare_and_swap(&stack->head, new_item->prev, new_item));

#else
  /*** Optional ***/
  // Implement a software CAS-based stack
#endif

  // Debug practice: you can check if this operation results in a stack in a consistent check
  // It doesn't harm performance as sanity check are disabled at measurement time
  // This is to be updated as your implementation progresses
  // stack_check((stack_t*)1);
}

int stack_pop(stack_t* stack) {
	stack_item_t* item;
#if NON_BLOCKING == 0


	pthread_mutex_lock(&stack->lock);
	item = stack->head;
	stack->head = item->prev;
	pthread_mutex_unlock(&stack->lock);

#elif NON_BLOCKING == 1

	stack_item_t* old_head;
	do {
		old_head = stack->head;
		item = (stack_item_t*)cas((size_t*)&stack->head, (size_t)old_head, (size_t)old_head->prev);
	}	while(item != old_head);

#else
  /*** Optional ***/
  // Implement a software CAS-based stack
#endif

  return item->val;
}
