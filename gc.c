/*
This file is part of simple-gc.

Copyright (c) 2012, Dario Hamidi <dario.hamidi@gmail.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the author nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL DARIO HAMIDI BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "gc.h"

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void* xmalloc(size_t n) {
     void* result = malloc(n);
     if (!result) {
          fputs("gc: out of memory.\n",stderr);
          abort(); /* all hope is lost */
     }

     memset(result,0,n);
     return result;
}

/* header for each object */
typedef struct node * Node;
struct node {
     Node next; /* TODO: collapse both fields into one using pointer masking */
     char mark;
};

static Node node(size_t size, Node next) {
     Node n = xmalloc(size + sizeof *n); /* header + object size */

     n->next = next;
     n->mark = 0;

     return n;
}

/* list node for rooted/protected objects */
typedef struct root * Root;
struct root {
     Root next;
     void* data;
};

static Root root(Root next, void* data) {
     Root r = xmalloc(sizeof *r);

     r->next = next;
     r->data = data;

     return r;
}

struct garbage_collector {
     Node free;              /* list of free objects */
     Node active;            /* list of used objects */
     Root used_roots;        /* active root nodes */
     Root free_roots;        /* free root nodes */
     Root used_protected;    /* active protected nodes */
     Root free_protected;    /* free protected nodes */
     gc_event_fn on_mark;    /* callback when marking an object */
     gc_event_fn on_collect; /* callback when collecting an object */
     gc_event_fn on_destroy; /* callback when destroying an object */
     size_t size;            /* size of a single object */
};

GarbageCollector gc_create(size_t nobjects,
                           size_t size,
                           gc_event_fn on_mark,
                           gc_event_fn on_collect,
                           gc_event_fn on_destroy) {
     GarbageCollector gc = xmalloc(sizeof *gc);

     /* initialise struct */
     gc->on_mark = on_mark;
     gc->on_collect = on_collect;
     gc->on_destroy = on_destroy;

     gc->size = size;

     /* create and put objects on free list */
     size_t i;
     for (i = 0; i < nobjects; i++)
          gc->free = node(size,gc->free);

     return gc;
}

static void destroy_nodes(Node head, gc_event_fn on_destroy) {
     Node cur = NULL;
     Node next = NULL;
     for (cur = head; cur ; cur = next) {
          next = cur->next;
          if (on_destroy)
               on_destroy(cur+1); /* call handler with address of object, not header */
          free(cur);
     }
}

static void destroy_roots(Root head) {
     Root cur,next;
     
     for (cur = head; cur ; cur = next) {
          next = cur->next;
          free(cur);
     }
}

void  gc_add(GarbageCollector gc, size_t nobjects) {
     assert(gc);

     /* create objects and put them on the free list */
     size_t i;
     for (i = 0; i < nobjects; i++)
          gc->free = node(gc->size,gc->free);
}

void  gc_root(GarbageCollector gc , void* object) {
     assert(gc);
     Root r = NULL;
     if (gc->free_roots) { /* reuse already allocated root node */
          r = gc->free_roots;
          r->next = gc->used_roots;
          gc->free_roots = gc->free_roots->next;
     }
     else /* create a new one */
          r = root(gc->used_roots,object);

     gc->used_roots = r;
}
void  gc_unroot(GarbageCollector gc , void* object) {
     assert(gc);

     Root cur,prev;
     /* find object in root list */
     for (cur = gc->used_roots, prev = NULL;
          cur; cur = cur->next) {
          if (cur->data == object) {
               if (prev)
                    prev->next = cur->next;
               else
                    gc->used_roots = cur->next;

               /* put it on free root list */
               cur->data = NULL;
               cur->next = gc->free_roots;
               gc->free_roots = cur;
          }

          prev = cur;
     }
               
}
void  gc_free(GarbageCollector* gc) {
     if (!gc)
          return;
     if (!*gc)
          return;

     destroy_nodes((*gc)->free,(*gc)->on_destroy);
     destroy_nodes((*gc)->active,(*gc)->on_destroy);
     destroy_roots((*gc)->used_roots);
     destroy_roots((*gc)->free_roots);
     destroy_roots((*gc)->used_protected);
     destroy_roots((*gc)->free_protected);

     free(*gc);
     
     *gc = NULL;
}
void  gc_protect(GarbageCollector gc , void** object) {
     assert(gc);
     Root r = NULL;
     if (gc->free_protected) { /* reuse existing node */
          r = gc->free_protected;
          r->next = gc->used_protected;
          gc->free_protected = gc->free_protected->next;
     }
     else /* create a new one */
          r = root(gc->used_protected,(void*)object);

     gc->used_protected = r;
}

void  gc_expose(GarbageCollector gc , size_t n) {
     assert(gc);

     while (n-->0 && gc->used_protected) {
          /* move nodes from used_protected list to free_protected list */
          Root r = gc->used_protected;
          gc->used_protected = r->next;
          r->next = gc->free_protected;
          gc->free_protected = r;
     }
}

static inline int is_marked(void* object) {
     Node n = (Node)(((char*)object) - sizeof(*n));
     return n->mark;
}

static inline void set_mark(void* object) {
     Node n = (Node)(((char*)object) - sizeof(*n));
     n->mark = 1;
}
static inline void unset_mark(void* object) {
     Node n = (Node)(((char*)object) - sizeof(*n));
     n->mark = 0;
}

void  gc_mark(gc_event_fn cont, void* object) {
     if (!object || is_marked(object))
          return;

     set_mark(object);
     if (cont)
          cont(object);
}

/* collect a single object */
static inline void collect(GarbageCollector gc, Node n, Node prev) {
     assert(gc);
     assert(n);
     
     if (gc->on_collect) /* call handler if necessary */
          gc->on_collect(n+1); /* with address of object, not header */
     if (prev)
          prev->next = n->next;
     else /* update list head if no predecessor given */
          gc->active = n->next;
     n->next = gc->free;
     gc->free = n;
}

void gc_collect(GarbageCollector gc) {
     assert(gc);

     Root cur;
     /* mark roots */
     for (cur = gc->used_roots; cur ; cur = cur->next)
          gc_mark(gc->on_mark,cur->data);

     /* mark protected objects */
     for (cur = gc->used_protected; cur; cur = cur->next)
          gc_mark(gc->on_mark,*(void**)cur->data);
     
     Node n,next,prev;
     prev = NULL;

     /* sweep */
     for (n = gc->active; n ; n = next){
          next = n->next;
          if (n->mark) {
               n->mark = 0;
               prev = n;
          }
          else
               collect(gc,n,prev);

     }
}

void* gc_alloc(GarbageCollector gc) {
     assert(gc);

     if (!gc->free) gc_collect(gc); /* try to reclaim unused objects */
     if (!gc->free) return NULL; /* no objects available */

     /* pop object from free list */
     Node n = gc->free;
     gc->free = gc->free->next;

     /* push object to active list */
     n->next = gc->active;
     gc->active = n;

     n->mark = 0;

     return (n+1); /* address of object without header */
}
