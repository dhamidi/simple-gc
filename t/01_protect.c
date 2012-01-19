#include "../gc.h"
#include "../test.h"

int main(int argc, char** argv) {
     GarbageCollector gc = gc_create(1,sizeof(int),NULL,NULL,NULL);
     int* a = NULL;
     gc_protect(gc,&a);
     a = gc_alloc(gc);

     ok(a,"allocating object");
     gc_collect(gc);

     ok(a,"protecting object");
     gc_expose(gc,1);
     
     gc_collect(gc);

     a = gc_alloc(gc);
     ok(a,"exposing object");

     gc_free(&gc);

     finish();
     
     return 0;
}
