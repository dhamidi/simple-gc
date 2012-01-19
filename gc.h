#ifndef GC_H
#define GC_H

#include <stddef.h>

typedef struct garbage_collector * GarbageCollector;
typedef void (*gc_event_fn)(void* object);

GarbageCollector gc_create(size_t nobjects,
                           size_t size,
                           gc_event_fn on_mark,
                           gc_event_fn on_collect,
                           gc_event_fn on_destroy);

void  gc_add     (GarbageCollector gc , size_t nobjects);
void* gc_alloc   (GarbageCollector gc);
void  gc_root    (GarbageCollector gc , void* object);
void  gc_unroot  (GarbageCollector gc , void* object);
void  gc_protect (GarbageCollector gc , void** object);
void  gc_expose  (GarbageCollector gc , size_t n);
void  gc_free    (GarbageCollector* gc);
void  gc_mark    (gc_event_fn cont, void* object);
void  gc_collect (GarbageCollector gc);
#endif
