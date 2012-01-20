#ifndef SIMPLE_GC_H
#define SIMPLE_GC_H

#include <stddef.h>

/* opaque handle to garbage collector */
typedef struct garbage_collector * GarbageCollector;

/* callback for garbage collection events */
typedef void (*gc_event_fn)(void* object);


/* Create a new garbage collector.
 *
 * Arguments:
 * - nobjects: number of objects to manage initially
 * - size: size of the managed objects
 * - on_mark: callback executed when marking an object
 * - on_collect: callback executed when collecting an object
 * - on_destroy: callback executed for each object when destroying the garbage collector
 * 
 * Returns:
 * A valid pointer to a garbage collector
 */

GarbageCollector gc_create(size_t nobjects,
                           size_t size,
                           gc_event_fn on_mark,
                           gc_event_fn on_collect,
                           gc_event_fn on_destroy);

/* Add new objects for the garbage collector to manage.
 *
 * Arguments:
 * - gc: a garbage collector
 * - nobjects: the number of objects to add
 */
void  gc_add     (GarbageCollector gc , size_t nobjects);

/* Request an object from the garbage collector.
 * 
 * Arguments:
 * - gc: a garbage collector
 * Returns:
 * a pointer to a usable objects or NULL if no objects are available
 */
void* gc_alloc   (GarbageCollector gc);

/* Add an object to the garbage collector's root set.
 *
 * Arguments:
 * - gc: a garbage collector
 * - object: the object to add; must have been created with gc_alloc
 */
void  gc_root    (GarbageCollector gc , void* object);

/* Remove an object from the garbage collector's root set.
 *
 * Arguments:
 * - gc: a garbage collector
 * - object: the object to remove; must have been created with gc_alloc
 */
void  gc_unroot  (GarbageCollector gc , void* object);

/* Protect objects at a specific memory location from garbage collection.
 *
 * Arguments:
 * - gc: a garbage collector
 * - object: address of a pointer to an object
 */
void  gc_protect (GarbageCollector gc , void** object);

/* Expose the most recently protected memory locations to the garbage collector.
 *
 * Arguments:
 * - gc: a garbage collector
 * - n: the number of objects to expose
 */
void  gc_expose  (GarbageCollector gc , size_t n);

/* Free all memory associated with a garbage collector. The function
 * on_destroy given when creating the the garbage collector is called
 * for each managed object with that object as an argument.
 *
 * Arguments:
 * - gc: address of a garbage collector
 * Ensures:
 * - gc will point to NULL afterwards
 */
void  gc_free    (GarbageCollector* gc);

/* Mark an object as reachable. This prevents it from being collected
 * once. A function can be passed in to continue marking other objects
 * referenced by the current object.
 *
 * Arguments:
 * - cont: pointer to a function that is marking other objects; can be NULL
 * - object: the object that just has been marked
 */
void  gc_mark    (gc_event_fn cont, void* object);

/* Collect all unreachable objects.
 *
 * Arguments:
 * - gc: a garbage collector
 */
void  gc_collect (GarbageCollector gc);
#endif
