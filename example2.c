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
#include <stdio.h>

/* list of numbers */
typedef struct node * Node;
struct node {
     int value;
     Node next;
};

/* when marking a list node also mark it's successors */
void node_mark(void* object) {
     if (!object)
          return;

     Node n = (Node)object;
     
     gc_mark(node_mark,n->next);
}

/* create a new list node */
Node node(GarbageCollector gc, Node next, int value) {
     if (!gc)
          return NULL;

     Node n = gc_alloc(gc);
     if (!n) /* ran out of objects */
          return NULL;

     n->next = next;
     n->value = value;

     return n;
}

int main(int argc, char** argv) {
     GarbageCollector gc = gc_create(10,sizeof(struct node),node_mark,NULL,NULL);

     Node head = NULL;
     gc_protect(gc,&head); /* protect the list from garbage collection */

     /* populate the list */
     head = node(gc,head,1);
     head = node(gc,head,2);
     head = node(gc,head,3);

     /* force collect */
     gc_collect(gc);

     /* check whether list still exists */
     Node cur;
     for (cur = head; cur ; cur = cur->next)
          printf("%d\n",cur->value);

     gc_free(&gc);
     
     return 0;
}
