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
#include <stdlib.h>
#include <string.h>

/*
 * returns managed memory by instantiating a garbage collector for each
 * request memory size.
 *
 * frees all garbage collector if size equals 0
 *
 * puts the newly allocated object into the root set if root equals
 * non-zero
 */

void* gc_malloc(size_t size, int root) {
     struct GCNode {
          size_t size;
          GarbageCollector gc;
          struct GCNode* next;
     } * gc_head = NULL;

     struct GCNode * cur = NULL;
     struct GCNode * next = NULL;
     
     /* if size == 0 free all garbage collectors */
     if (size == 0) {
          for (cur = gc_head; cur ; cur = next) {
               next = cur->next;
               gc_free(&(cur->gc));
               free(cur);
          }
          gc_head = NULL; /* avoid having a dangling pointer */
     }
     
     /* check whether there is a fitting garbage collector already */
     for (cur = gc_head; cur ; cur = cur->next)
          if (cur->size == size)
               break;

     /* create a new garbage collector if necessary */
     if (cur == NULL) {
          cur = malloc(sizeof(*cur));

          cur->size = size;
          cur->gc   = gc_create(1024,size,NULL,NULL,NULL);
          cur->next = gc_head;
          gc_head = cur;
     }

     void* result = gc_alloc(cur->gc);

     if (root)
          gc_root(cur->gc,result);

     return result;
}

/* duplicate a string, result is garbage collected */
char* gc_strdup(const char* src, int root) {
     if (!src)
          return NULL;

     char* dest = gc_malloc(strlen(src) + 1, root);

     if (dest)
          strcpy(dest,src);

     return dest;
}

int main(int argc, char** argv) {
     int i;
     char* survives = gc_strdup("Hello world1",1);
     
     for (i = 0; i < 2000; i++)
          gc_strdup("Hello world0",0);

     printf("%s\n",survives);
     
     gc_malloc(0,0);
     return 0;
}
