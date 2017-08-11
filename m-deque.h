/*
 * M*LIB - DEQUE module
 *
 * Copyright (c) 2017, Patrick Pelissier
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * + Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * + Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef __M_DEQUE_H
#define __M_DEQUE_H

#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "m-core.h"
#include "m-i-list.h"

/* Define a deque of a given type.
   USAGE: DEQUE_DEF(name, type [, oplist_of_the_type]) */
#define DEQUE_DEF(name, ...)                                            \
  M_IF_NARGS_EQ1(__VA_ARGS__)                                           \
  (DEQUEI_DEF2(name, __VA_ARGS__, M_DEFAULT_OPLIST, M_C(name,_t), M_C(name,_it_t), M_C(name, _node_t) ), \
   DEQUEI_DEF2(name, __VA_ARGS__,                   M_C(name,_t), M_C(name,_it_t), M_C(name, _node_t) ))

/********************************** INTERNAL ************************************/

#define DEQUEUI_DEFAULT_SIZE  8

#define DEQUEI_CONTRACT(d) do {						\
    assert ((d) != NULL);						\
    assert ((d)->default_size >= DEQUEUI_DEFAULT_SIZE);			\
    assert ((d)->front->node != NULL);					\
    assert ((d)->front->index <= (d)->front->node->size);		\
    assert ((d)->back->node != NULL);					\
    assert ((d)->back->index <= (d)->back->node->size);			\
    assert ((d)->front->node != (d)->back->node ||			\
	    (d)->front->index <= (d)->back->index);			\
    assert ((d)->front->node != (d)->back->node ||			\
	    (d)->back->index - (d)->front->index == (d)->count);	\
  } while (0)

#define DEQUEI_DEF2(name, type, oplist, deque_t, it_t, node_t)		\
									\
  typedef struct M_C(name, _node_s) {					\
    ILIST_INTERFACE(M_C(name, _node_list), struct M_C(name, _node_s));	\
    size_t size;							\
    type  data[];							\
  } node_t;								\
									\
  ILIST_DEF(M_C(name, _node_list), node_t, (DEL(M_GET_FREE oplist)) )	\
  									\
  typedef struct M_C(name, _it2_s) {					\
    node_t *node;							\
    size_t        index;						\
  } M_C(name, _it2_t)[1];						\
  									\
  typedef struct M_C(name, _s) {					\
    M_C(name, _node_list_t) list;					\
    M_C(name, _it2_t)       front;					\
    M_C(name, _it2_t)       back;					\
    size_t                  default_size;				\
    size_t                  count;					\
  } deque_t[1];								\
		 							\
  typedef struct M_C(name, _it_s) {					\
    node_t *node;							\
    size_t        index;						\
    const struct M_C(name, _s) *deque;					\
  } it_t[1];								\
  									\
  									\
  static inline deque_node_t*						\
  M_C(name, _int_new_node)(deque_t d)					\
  {									\
    const size_t def = d->default_size;					\
    deque_node_t*n = (deque_node_t*) (void*)				\
      M_GET_REALLOC oplist (char, NULL,					\
			    sizeof(deque_node_t)+def * sizeof(type) );	\
    if (n==NULL) {							\
      M_MEMORY_FULL(sizeof(deque_node_t)+def * sizeof(type));		\
      return NULL;							\
    }									\
    n->size = def;							\
    deque_node_list_init_field(n);					\
    /* Increase default size for next allocation */			\
    d->default_size += def/2;						\
    return n;								\
  }									\
									\
  static inline void							\
  M_C(name, _init)(deque_t d)						\
  {									\
    M_C(name, _node_list_init)(d->list);				\
    d->default_size = DEQUEUI_DEFAULT_SIZE;				\
    d->count        = 0;						\
    deque_node_t *n = M_C(name, _int_new_node)(d);			\
    if (n == NULL) return;						\
    deque_node_list_push_back(d->list, n);				\
    d->front->node  = n;						\
    d->front->index = DEQUEUI_DEFAULT_SIZE/2;				\
    d->back->node   = n;						\
    d->back->index  = DEQUEUI_DEFAULT_SIZE/2;				\
    DEQUEI_CONTRACT(d);							\
  }									\
									\
  static inline void							\
  M_C(name, _clear)(deque_t d)						\
  {									\
    DEQUEI_CONTRACT(d);							\
    /* We registered the delete operator to clear all objects	*/	\
    M_C(name, _node_list_clear)(d->list);				\
    /* It is safer to clean some variables*/				\
    d->front->node  = NULL;						\
    d->back->node   = NULL;						\
    d->count = 0;							\
  }									\
									\
  static inline void							\
  M_C(name, _push_back)(deque_t d, type const x)			\
  {									\
    DEQUEI_CONTRACT(d);							\
    deque_node_t *n = d->back->node;					\
    size_t index = d->back->index;					\
    if (M_UNLIKELY (n->size <= index)) {				\
      n = M_C(name, _node_list_next_obj)(d->list, n);			\
      if (n == NULL) {							\
	n = M_C(name, _int_new_node)(d);				\
	if (n == NULL) return;						\
	M_C(name, _node_list_push_back)(d->list, n);			\
      }									\
      d->back->node = n;						\
      index = 0;							\
    }									\
    M_GET_INIT_SET oplist (n->data[index], x);				\
    index++;								\
    d->count ++;							\
    d->back->index = index;						\
    DEQUEI_CONTRACT(d);							\
  }									\
									\
  static inline void							\
  M_C(name, _push_front)(deque_t d, type const x)			\
  {									\
    DEQUEI_CONTRACT(d);							\
    deque_node_t *n = d->front->node;					\
    size_t index = d->front->index;					\
    index --;								\
    /* If overflow */							\
    if (M_UNLIKELY (n->size <= index)) {				\
      n = M_C(name, _node_list_previous_obj)(d->list, n);		\
      if (n == NULL) {							\
	n = M_C(name, _int_new_node)(d);				\
	if (n == NULL) return;						\
	M_C(name, _node_list_push_front)(d->list, n);			\
      }									\
      d->front->node = n;						\
      index = n->size -1;						\
    }									\
    M_GET_INIT_SET oplist (n->data[index], x);				\
    d->count ++;							\
    d->front->index = index;						\
    DEQUEI_CONTRACT(d);							\
  }									\
									\
  static inline void							\
  M_C(name, _pop_back)(type *ptr, deque_t d)				\
  {									\
    DEQUEI_CONTRACT(d);							\
    assert(d->count > 0);						\
    deque_node_t *n = d->back->node;					\
    size_t index = d->back->index;					\
    index --;								\
    if (M_UNLIKELY (n->size <= index)) {				\
      n = deque_node_list_previous_obj(d->list, n);			\
      assert (n != NULL);						\
      d->back->node = n;						\
      index = n->size-1;						\
    }									\
    if (ptr != NULL)							\
      M_IF_METHOD(MOVE, oplist) (                                       \
      M_GET_MOVE oplist(*ptr, n->data[index]); else                     \
      ,                                                                 \
      M_GET_SET oplist(*ptr, n->data[index]);				\
      )                                                                 \
    M_GET_CLEAR oplist (n->data[index]);				\
    d->count --;							\
    d->back->index = index;						\
    DEQUEI_CONTRACT(d);							\
  }									\
									\
  static inline void							\
  M_C(name, _pop_front)(type *ptr, deque_t d)				\
  {									\
    DEQUEI_CONTRACT(d);							\
    assert(d->count > 0);						\
    deque_node_t *n = d->front->node;					\
    size_t index = d->front->index;					\
    if (M_UNLIKELY (n->size <= index)) {				\
      n = deque_node_list_next_obj(d->list, n);				\
      assert (n != NULL);						\
      d->front->node = n;						\
      index = 0;							\
    }									\
    if (ptr != NULL)							\
      M_IF_METHOD(MOVE, oplist) (                                       \
      M_GET_MOVE oplist(*ptr, n->data[index]); else                     \
      ,                                                                 \
      M_GET_SET oplist(*ptr, n->data[index]);				\
      )                                                                 \
    M_GET_CLEAR oplist (n->data[index]);				\
    index++;								\
    d->count --;							\
    d->front->index = index;						\
  }									\
									\
  static inline void							\
  M_C(name, _it)(it_t it, const deque_t d)				\
  {									\
    DEQUEI_CONTRACT(d);							\
    assert (it != NULL);						\
    it->node  = d->front->node;						\
    it->index = d->front->index;					\
    it->deque = d;							\
  }									\
									\
  static inline void							\
  M_C(name, _it_last)(it_t it, const deque_t d)				\
  {									\
    DEQUEI_CONTRACT(d);							\
    assert (it != NULL);						\
    it->node  = d->back->node;						\
    it->index = d->back->index - 1;					\
    it->deque = d;							\
    if (M_UNLIKELY (it->index >= it->node->size)) {			\
      it->node = deque_node_list_previous_obj(d->list, it->node);	\
      assert (it->node != NULL);					\
      it->index = it->node->size-1;					\
    }									\
  }									\
									\
  static inline void							\
  M_C(name, _it_end)(it_t it, const deque_t d)				\
  {									\
    DEQUEI_CONTRACT(d);							\
    assert (it != NULL);						\
    it->node  = d->back->node;						\
    it->index = d->back->index;						\
    it->deque = d;							\
  }									\
									\
  static inline void							\
  M_C(name, _it_set)(it_t it1, const it_t it2)				\
  {									\
    assert (it1 != NULL);						\
    assert (it2 != NULL);						\
    it1->node  = it2->node;						\
    it1->index = it2->index;						\
    it1->deque = it2->deque;						\
  }									\
									\
  static inline bool							\
  M_C(name, _end_p)(it_t it)						\
  {									\
    assert (it != NULL);						\
    return it->node == it->deque->back->node				\
      && it->index >= it->deque->back->index;				\
  }									\
									\
  static inline void							\
  M_C(name, _next)(it_t it)						\
  {									\
    assert (it != NULL);						\
    deque_node_t *n = it->node;						\
    it->index ++;							\
    if (M_UNLIKELY (it->index >= n->size)) {				\
      n = deque_node_list_next_obj(it->deque->list, n);			\
      if (M_UNLIKELY (n == NULL)) return;				\
      it->node = n;							\
      it->index = 0;							\
    }									\
  }									\
									\
  static inline void							\
  M_C(name, _previous)(it_t it)						\
  {									\
    assert (it != NULL);						\
    deque_node_t *n = it->node;						\
    it->index --;							\
    if (M_UNLIKELY (it->index >= n->size)) {				\
      n = deque_node_list_previous_obj(it->deque->list, n);		\
      if (M_UNLIKELY (n == NULL)) {					\
	/* Point to 'end' (can't undo it) */				\
	it->node  = it->deque->back->node;				\
	it->index = it->deque->back->node->size;			\
	return;								\
      }									\
      it->node = n;							\
      it->index = 0;							\
    }									\
  }									\
									\
  static inline bool							\
  M_C(name, _last_p)(it_t it)						\
  {									\
    assert (it != NULL);						\
    it_t it2;								\
    M_C(name, _it_set)(it2, it);					\
    M_C(name, _next)(it2);						\
    return M_C(name, _end_p)(it2);					\
  }									\
									\
  static inline bool							\
  M_C(name, _it_equal_p)(it_t it1, const it_t it2)			\
  {									\
    assert (it1 != NULL);						\
    assert (it2 != NULL);						\
    return it1->deque == it2->deque					\
      && it1->node == it2->node						\
      && it1->index == it2->index;					\
  }									\
  
// TODO: init_set, set, move, init_move, equal_p, _hash,
// _get, _cget, _clean, _front, _back, _set_at, _push_back_raw,
// _pop_front_raw, _push_back_new, _push_front_new, _empty_p, _size,
// _swap, _swap_at, IO [like array]


#endif