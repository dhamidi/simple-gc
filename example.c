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
