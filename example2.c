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
