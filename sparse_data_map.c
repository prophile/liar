#include <stdlib.h>
#include <stdbool.h>

#include "sparse_data_map.h"

void sparse_data_map_init ( sparse_data_map* map )
{
	map->data = malloc(sizeof(unsigned long*) * 64);
	map->len = 0;
	map->capacity = 64;
	map->isSorted = false;
}

void sparse_data_map_insert ( sparse_data_map* map, void* object )
{
	unsigned long newLen = map->len + 1;
	if (newLen > map->capacity)
	{
		map->data = realloc(map->data, sizeof(unsigned long*) * map->capacity * 2);
		map->capacity *= 2;
	}
	map->data[map->len] = object;
	map->len = newLen;
	map->isSorted = false;
}

static int __sparse_data_comparator ( const void* left, const void* right )
{
	unsigned long l = *(const unsigned long*)left;
	unsigned long r = *(const unsigned long*)right;
	return l < r ? -1 : ((l == r) ? 0 : 1);
}

static void __sparse_data_map_sort ( sparse_data_map* map )
{
	// TODO: make this a radix sort
	qsort(map->data, map->len, sizeof(unsigned long*), __sparse_data_comparator);
	map->isSorted = true;
}

void* sparse_data_map_lookup ( sparse_data_map* map, unsigned long hash )
{
	if (!map->isSorted)
	{
		// sort map
		__sparse_data_map_sort(map);
	}
	// binary search
	return bsearch(&hash, map->data, map->len, sizeof(unsigned long*), __sparse_data_comparator);
}
