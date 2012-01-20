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
          abort();
     }

     memset(result,0,n);
     return result;
}

typedef struct node * Node;
struct node {
     Node next;
     char mark;
};

static Node node(size_t size, Node next) {
     Node n = xmalloc(size + sizeof *n);

     n->next = next;
     n->mark = 0;

     return n;
}

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
     Node free;
     Node active;
     Root used_roots;
     Root free_roots;
     Root used_protected;
     Root free_protected;
     gc_event_fn on_mark;
     gc_event_fn on_collect;
     gc_event_fn on_destroy;
     size_t size;
};

GarbageCollector gc_create(size_t nobjects,
                           size_t size,
                           gc_event_fn on_mark,
                           gc_event_fn on_collect,
                           gc_event_fn on_destroy) {
     GarbageCollector gc = xmalloc(sizeof *gc);

     gc->free = NULL;
     gc->active = NULL;
     gc->used_roots = NULL;
     gc->free_roots = NULL;
     gc->used_protected = NULL;
     gc->free_protected = NULL;
     
     gc->on_mark = on_mark;
     gc->on_collect = on_collect;
     gc->on_destroy = on_destroy;

     gc->size = size;

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
               on_destroy(cur+1);
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

     size_t i;
     for (i = 0; i < nobjects; i++)
          gc->free = node(gc->size,gc->free);
}

void  gc_root(GarbageCollector gc , void* object) {
     assert(gc);
     Root r = NULL;
     if (gc->free_roots) {
          r = gc->free_roots;
          r->next = gc->used_roots;
          gc->free_roots = gc->free_roots->next;
     }
     else 
          r = root(gc->used_roots,object);

     gc->used_roots = r;
}
void  gc_unroot(GarbageCollector gc , void* object) {
     assert(gc);

     Root cur,prev;
     for (cur = gc->used_roots, prev = NULL;
          cur; cur = cur->next) {
          if (cur->data == object) {
               if (prev)
                    prev->next = cur->next;
               else
                    gc->used_roots = cur->next;

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
     if (gc->free_protected) {
          r = gc->free_protected;
          r->next = gc->used_protected;
          gc->free_protected = gc->free_protected->next;
     }
     else 
          r = root(gc->used_protected,(void*)object);

     gc->used_protected = r;
}

void  gc_expose(GarbageCollector gc , size_t n) {
     assert(gc);

     while (n-->0 && gc->used_protected) {
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

static inline void collect(GarbageCollector gc, Node n, Node prev) {
     assert(gc);
     assert(n);
     
     if (gc->on_collect)
          gc->on_collect(n+1);
     if (prev)
          prev->next = n->next;
     else
          gc->active = n->next;
     n->next = gc->free;
     gc->free = n;
}

void gc_collect(GarbageCollector gc) {
     assert(gc);

     Root cur;
     for (cur = gc->used_roots; cur ; cur = cur->next)
          gc_mark(gc->on_mark,cur->data);

     for (cur = gc->used_protected; cur; cur = cur->next)
          gc_mark(gc->on_mark,*(void**)cur->data);
     
     Node n,next,prev;
     prev = NULL;
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

     if (!gc->free) gc_collect(gc);
     if (!gc->free) return NULL;

     Node n = gc->free;
     gc->free = gc->free->next;

     n->next = gc->active;
     gc->active = n;

     n->mark = 0;

     return (n+1);
}
