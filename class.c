#include "hash.h"
#include "sparse_data_map.h"
#include "gc.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef struct _class class;

typedef union _ivar
{
	unsigned long integer;
	void* pointer;
} ivar;

static void finalise ( void* object );

struct _class
{
	unsigned long hash;
	class* superclass;
	char* name;
	sparse_data_map classMethods;
	sparse_data_map instanceMethods;
	unsigned long numInstanceVariables;
	ivar svars[0];
};

typedef struct _method method;

struct _method
{
	unsigned long hash;
	void* code;
};

static sparse_data_map classes;

static class* lookup ( unsigned long parent )
{
	return (class*)parent;
}

// runtime functions
void* obj_lookup_method ( void* object, unsigned long meth )
{
	class* cls = *(class**)object;
	method* method = sparse_data_map_lookup(&(cls->instanceMethods), meth);
	if (!method)
		return NULL;
	return method->code;
}

void* class_lookup_method ( unsigned long classID, unsigned long meth )
{
	class* cls = lookup(classID);
	if (!cls)
		return NULL;
	method* method = sparse_data_map_lookup(&(cls->classMethods), meth);
	if (!method)
		return NULL;
	return method->code;
}

void class_init_runtime ()
{
	GC_init();
	sparse_data_map_init(&classes);
}

unsigned long class_register ( const char* name, unsigned long parent, unsigned long nivars, unsigned long nsvars )
{
	unsigned long aHash = hash(name);
	int i;
	class* cls = malloc(sizeof(class) + (sizeof(void*)*nsvars));
	cls->hash = aHash;
	cls->superclass = lookup(parent);
	cls->name = strdup(name);
	sparse_data_map_init(&cls->classMethods);
	sparse_data_map_init(&cls->instanceMethods);
	cls->numInstanceVariables = nivars;
	for (i = 0; i < nsvars; i++)
	{
		cls->svars[i].pointer = NULL;
	}
	sparse_data_map_insert(&classes, cls);
	return aHash;
}

void* obj_allocate_stack_frame ()
{
	return GC_new_object(sizeof(void*), GC_ROOT, NULL);
}

void obj_release_stack_frame ( void* frame )
{
	GC_unregister_reference(GC_ROOT, frame);
}

void obj_frame_transfer ( void* object, void* frame, void* parentFrame )
{
	GC_register_reference(parentFrame, object, NULL);
	GC_unregister_reference(frame, object);
}

static void ivwrite ( ivar* var, void* target, void* gcSource, bool isWeak )
{
	if (var->integer)
	{
		if (var->integer & 1)
		{
			var->integer &= 1UL;
			GC_unregister_weak_reference(gcSource, var->pointer);
		}
		else
		{
			GC_unregister_reference(gcSource, var->pointer);
		}
	}
	var->pointer = target;
	if (isWeak)
	{
		GC_register_weak_reference(gcSource, target, &(var->pointer));
		var->integer |= 1;
	}
	else
	{
		GC_register_reference(gcSource, target, &(var->pointer));
	}
}

static void* ivread ( ivar* var )
{
	ivar copy = *var;
	if (!copy.integer)
		return NULL;
	copy.integer &= ~1UL;
	return copy.pointer;
}

void obj_write ( void* object, unsigned long index, void* target )
{
	ivar* objectVars = (ivar*)object;
	objectVars++;
	ivwrite(&(objectVars[index]), target, object, false);
}

void obj_write_weak ( void* object, unsigned long index, void* target )
{
	ivar* objectVars = (ivar*)object;
	objectVars++;
	ivwrite(&(objectVars[index]), target, object, true);
}

void* obj_read ( void* object, unsigned long index )
{
	ivar* objectVars = (ivar*)object;
	objectVars++;
	return ivread(&(objectVars[index]));
}

void class_write_static ( unsigned long classID, unsigned long index, void* target )
{
	class* cls = lookup(classID);
	if (!cls)
		return;
	ivwrite(&(cls->svars[index]), target, GC_ROOT, false);
}

void class_write_static_weak ( unsigned long classID, unsigned long index, void* target )
{
	class* cls = lookup(classID);
	if (!cls)
		return;
	ivwrite(&(cls->svars[index]), target, GC_ROOT, true);
}

void* class_read_static ( unsigned long classID, unsigned long index )
{
	class* cls = lookup(classID);
	if (!cls)
		return NULL;
	return ivread(&(cls->svars[index]));
}

void* obj_new ( unsigned long classID, void* stackFrame, unsigned long slot )
{
	void* obj;
	class* cls = lookup(classID);
	if (!cls)
		return NULL;
	obj = GC_new_object(sizeof(void*) * (cls->numInstanceVariables + 1), stackFrame ? stackFrame : GC_ROOT, finalise);
	return obj;
}

unsigned long class_lookup ( const char* name )
{
	unsigned long h = hash(name);
	class* cls = (class*)sparse_data_map_lookup(&classes, h);
	return (unsigned long)cls;
}

unsigned long method_lookup ( const char* name )
{
	return hash(name);
}

extern void obj_msg_send ( void*, void*, unsigned long, unsigned long );
extern void class_msg_send ( void*, unsigned long, unsigned long, unsigned long );

static void finalise ( void* object )
{
	void* frame = obj_allocate_stack_frame();
	obj_msg_send(frame, object, method_lookup("finalise"), 0);
	obj_release_stack_frame(frame);
}

void rt_start ( unsigned long keyclass )
{
	void* rootFrame = obj_allocate_stack_frame();
	class_msg_send(rootFrame, keyclass, method_lookup("start"), 0);
	obj_release_stack_frame(rootFrame);
}
