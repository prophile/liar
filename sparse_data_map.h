#ifndef __SPARSE_DATA_MAP__
#define __SPARSE_DATA_MAP__

#include <stdbool.h>

typedef struct _sparse_data_map sparse_data_map;

struct _sparse_data_map
{
	unsigned long len, capacity;
	unsigned long** data;
	bool isSorted;
};

void sparse_data_map_init ( sparse_data_map* map );
void sparse_data_map_insert ( sparse_data_map* map, void* object );
void* sparse_data_map_lookup ( sparse_data_map* map, unsigned long hash );

#endif
