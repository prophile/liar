#include "hash.h"
#include "sparse_data_map.h"
#include "gc.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct _class class;

#ifdef __BIG_ENDIAN__
#define BE16(x) (x)
#define BE32(x) (x)
#define BE64(x) (x)
#else
static unsigned short BE16 ( unsigned short in )
{
	return (in << 8) | (in >> 8);
}

static unsigned int BE32 ( unsigned int in )
{
	return __builtin_bswap32(in);
}

static unsigned long BE64 ( unsigned long in )
{
	return __builtin_bswap64(in);
}
#endif

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
	unsigned long instanceVariableOffset;
	unsigned long numInstanceVariables, numStaticVariables;
	ivar svars[0];
};

typedef struct _method method;

struct _method
{
	unsigned long hash;
	const void* code;
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
	do
	{
		method* method = sparse_data_map_lookup(&(cls->instanceMethods), meth);
		if (method)
			return (void*)(method->code);
		cls = cls->superclass;
	} while(cls);
	return NULL;
}

void* class_lookup_method ( unsigned long classID, unsigned long meth )
{
	class* cls = lookup(classID);
	do
	{
		method* method = sparse_data_map_lookup(&(cls->instanceMethods), meth);
		if (method)
			return (void*)(method->code);
		cls = cls->superclass;
	} while(cls);
	return NULL;
}

void rt_init ()
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
	if (cls->superclass)
	{
		cls->instanceVariableOffset = cls->superclass->numInstanceVariables;
		cls->numInstanceVariables = nivars + cls->instanceVariableOffset;
	}
	else
	{
		cls->instanceVariableOffset = 0;
		cls->numInstanceVariables = 0;
	}
	cls->numStaticVariables = nsvars;
	for (i = 0; i < nsvars; i++)
	{
		cls->svars[i].pointer = NULL;
	}
	sparse_data_map_insert(&classes, cls);
	return (unsigned long)cls;
}

unsigned long class_register_instance_method ( unsigned long aClass, const char* name, const void* address )
{
	unsigned long aHash = hash(name);
	class* cls = lookup(aClass);
	method* meth = malloc(sizeof(method));
	meth->hash = aHash;
	meth->code = address;
	sparse_data_map_insert(&(cls->instanceMethods), meth);
	return aHash;
}

unsigned long class_register_class_method ( unsigned long aClass, const char* name, const void* address )
{
	unsigned long aHash = hash(name);
	class* cls = lookup(aClass);
	method* meth = malloc(sizeof(method));
	meth->hash = aHash;
	meth->code = address;
	sparse_data_map_insert(&(cls->classMethods), meth);
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
	*(class**)obj = cls;
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

extern const void* findsymbol ( const char* name, void* modhandle );

static void class_register_from_data ( const void* data, void* modhandle )
{
	unsigned long i;
	const unsigned char* bdata = (const unsigned char*)data;
	unsigned char nameLength = *bdata;
	bdata += 1;
	char name[256];
	name[nameLength] = 0;
	memcpy(name, bdata, nameLength);
	bdata += nameLength;
	nameLength = *bdata;
	bdata += 1;
	char supername[256];
	supername[nameLength] = 0;
	memcpy(supername, bdata, nameLength);
	bdata += nameLength;
	unsigned long nsvars = BE64(*(const unsigned long*)bdata);
	bdata += 8;
	unsigned long nivars = BE64(*(const unsigned long*)bdata);
	bdata += 8;
	unsigned long superObj = class_lookup(supername);
	unsigned long classObj = class_register(name, superObj, nivars, nsvars);
	unsigned long nmethods = BE64(*(const unsigned long*)bdata);
	bdata += 8;
	for (i = 0; i < nmethods; i++)
	{
		char methodName[256];
		char methodEncodedName[540];
		nameLength = *bdata;
		bdata += 1;
		methodName[nameLength] = 0;
		memcpy(methodName, bdata, nameLength);
		bdata += nameLength;
		sprintf(methodEncodedName, "class.%s.%s", name, methodName);
		const void* ptr = findsymbol(methodEncodedName, modhandle);
		if (ptr)
		{
			class_register_class_method(classObj, methodName, ptr);
		}
	}
	nmethods = BE64(*(const unsigned long*)bdata);
	bdata += 8;
	for (i = 0; i < nmethods; i++)
	{
		char methodName[256];
		char methodEncodedName[540];
		nameLength = *bdata;
		bdata += 1;
		methodName[nameLength] = 0;
		memcpy(methodName, bdata, nameLength);
		bdata += nameLength;
		sprintf(methodEncodedName, "instance.%s.%s", name, methodName);
		const void* ptr = findsymbol(methodName, modhandle);
		if (ptr)
		{
			class_register_instance_method(classObj, methodName, ptr);
		}
	}
}

unsigned long obj_class ( void* object )
{
	if (!object)
		return 0;
	return *(unsigned long*)object;
}

void rt_register_module ( void* modhandle )
{
	const unsigned char* infoblock = (unsigned char*)findsymbol("module.info", modhandle);
	unsigned long nclasses = BE64(*(const unsigned long*)infoblock);
	infoblock += 8;
	unsigned long i;
	for (i = 0; i < nclasses; i++)
	{
		char name[256];
		char dataName[280];
		unsigned char nameLength = *infoblock;
		infoblock += 1;
		name[nameLength] = 0;
		memcpy(name, infoblock, nameLength);
		sprintf(dataName, "class.%s", name);
		const void* classBlock = findsymbol(dataName, modhandle);
		if (classBlock)
			class_register_from_data(classBlock, modhandle);
	}
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
