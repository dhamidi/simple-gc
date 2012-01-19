#include "../gc.h"
#include "../test.h"

int main(int argc, char** argv) {
     GarbageCollector gc = gc_create(1,sizeof (int),NULL,NULL,NULL);

     ok(gc,"allocating garbage collector");
     int* o = gc_alloc(gc);
     ok(o,"allocating object");
     gc_root(gc,o);

     int* b = gc_alloc(gc);
     ok(b == NULL,"rooting object");
     gc_unroot(gc,o);
     b = gc_alloc(gc);

     ok(b,"unrooting object");

     gc_free(&gc);
     ok(gc == NULL, "freeing garbage collector");

     finish();
     
     return 0;
}
