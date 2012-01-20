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
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "gc.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* managed strings */
typedef struct string * String;
struct string {
     size_t len;
     char*  data;
};

/* free a string's data */
void string_free(void* _str) {
     if (!_str)
          return;
     free(((String)_str)->data);
}

/* create a new string */
String string(GarbageCollector gc, char* data) {
     if (!gc)
          return NULL;

     String str = gc_alloc(gc);
     if (!str) { /* ran out of objects */
          gc_add(gc,1); /* let the garbage collector manage one more object */
          str = gc_alloc(gc);
     }
     str->len = strlen(data);
     str->data = malloc(str->len + 1); /* + 1 for null byte */
     strcpy(str->data,data); /* also copies the null byte from source */

     return str;
}

/* convert String -> char* */
const char* string_cstr(String str) {
     if (str)
          return str->data;
}

/* create a new string by concatenating two other strings */
String string_concat(GarbageCollector gc, String a, String b) {
     size_t buflen = a->len + b->len;
     char* buf = malloc(buflen + 1); /* + 1 for null byte */
     memset(buf,0,buflen+1);
     
     strcpy(buf,a->data);
     strcat(buf,b->data);

     String result = gc_alloc(gc);
     if (!result) {      /* ran out of objects */
          gc_add(gc,1);  /* let the garbage collector manage one more object */
          result = gc_alloc(gc);
     }
     result->data = buf;
     result->len = buflen;

     return result;
}

int main(int argc, char** argv) {
     GarbageCollector string_gc = gc_create(10,                   /* manage 10 strings */
                                            sizeof(struct string),
                                            NULL, /* do nothing on mark */
                                            string_free, /* on collect */
                                            string_free); /* on destroy */

     String result = NULL;

     /* protect whatever result is pointing to */
     gc_protect(string_gc,&result);
     
     /* create a new string */
     String str = string(string_gc,"hello, world"); 

     /* prevent the string from being collected */
     gc_root(string_gc,str);

     /* collect garbage */
     gc_collect(string_gc);

     /* check that the string is still valid */
     printf("%s\n",string_cstr(str));

     /* double the string */
     result = string_concat(string_gc,str,str);

     /* collect garbage */
     gc_collect(string_gc);

     /* check that result is still valid */
     printf("%s\n",string_cstr(result));

     /* clean up everything */
     gc_free(&string_gc);

     return 0;
}
