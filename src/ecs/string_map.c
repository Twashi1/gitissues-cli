#include <gitissues/ecs/string_map.h>
#include <gitissues/errs.h>
#include <gitissues/log.h>

struct StringMap createStringMap(void) {
  struct StringMap map = {0};
  map.stringAllocator = createBlockAllocator(4096);
  map.capacity = 0;
  map.size = 0;
  map.data = NULL;

  map.dense.capacity = 0;
  map.dense.size = 0;
  map.dense.data = NULL;

  reserveStringMap(&map, _GITISSUES_HASH_MAP_MINIMUM);

  return map;
}

void freeStringMap(struct StringMap *map) {
  dropBlockAllocator(&map->stringAllocator);
  free(map->data);
  free(map->dense.data);
}

enum ErrorCode reserveStringMap(struct StringMap *map, uint32_t minCapacity) {
  // TODO: check threshold!
  uint32_t newThreshold =
      (uint32_t)ceilf(minCapacity * _GITISSUES_STRING_MAP_LOAD_FACTOR);
  if (map->size + 1 < newThreshold)
    return GITISSUES_OK;

  uint32_t growthFactor = map->capacity * 2;
  uint32_t newCapacity =
      growthFactor > minCapacity ? growthFactor : minCapacity;

  struct StringMap newMap = {0};
  newMap.size = 0;
  newMap.capacity = newCapacity;
  newMap.data = NULL;
  newMap.stringAllocator = map->stringAllocator;
  newMap.threshold = newThreshold;

  // Create new allocation, give to newMap temporarily
  newMap.data =
      (struct StringMapNode *)calloc(newCapacity, sizeof(struct StringMapNode));
  if (DEBUG_CONDITION(newMap.data == NULL)) {
    return GITISSUES_OUT_OF_MEMORY;
  }

  newMap.dense.data = (struct StringMapNode *)realloc(
      map->dense.data, newCapacity * sizeof(struct StringMapNode));
  DEBUG_ASSERT(newMap.dense.data != NULL,
               "Failed to reallocate dense array of StringMap");
  newMap.dense.capacity = newCapacity;

  // TODO: use dense array
  // Iterate old map and rehash
  for (uint32_t i = 0; i < map->capacity; i++) {
    struct StringMapNode *oldNode = map->data + i;

    // Assumes we disallow any 0-length strings
    if (oldNode->string.size == 0)
      continue;

    // Reinsert
    _insertUncheckedStringMap(&newMap, oldNode->string, oldNode->value);
  }

  free(map->data);
  map->data = newMap.data;
  map->capacity = newMap.capacity;
  map->dense.data = newMap.dense.data;
  map->dense.capacity = newMap.dense.capacity;
  map->threshold = newMap.threshold;

  return GITISSUES_OK;
}

enum ErrorCode insertStringMap(struct StringMap *map, struct UmbraString string,
                               ComponentID value) {
  enum ErrorCode ec = reserveStringMap(map, map->capacity + 1);
  if (DEBUG_CONDITION(ec != GITISSUES_OK)) {
    return ec;
  }

  _insertUncheckedStringMap(map, string, value);

  return GITISSUES_OK;
}

enum ErrorCode _insertUncheckedStringMap(struct StringMap *map,
                                         struct UmbraString string,
                                         ComponentID value) {
  uint32_t hash = fnv1aHashUmbra(string);
  uint32_t index = hash % map->capacity;
  uint32_t steps = 0;

  // TODO: debug assert not already inserted
  DEBUG_ASSERT(string.size != 0, "Attempted to add empty string");

  do {
    // Indicator that this slot is empty
    if (map->data[index].string.size == 0) {
      map->data[index].string = string;
      map->data[index].stringHash = hash;
      map->data[index].value = value;
      ++map->size;

      // Insert into dense
      map->dense.data[map->dense.size].string = string;
      map->dense.data[map->dense.size].stringHash = hash;
      map->dense.data[map->dense.size].value = value;
      ++map->dense.size;

      return GITISSUES_OK;
    }

    index = (index + 1) % map->capacity;
  } while (steps++ < map->capacity);

  return GITISSUES_STRING_MAP_INSERTION_FAIL;
}

ComponentID getStringMap(struct StringMap *map, struct UmbraString string) {
  if (map->size == 0)
    return _GITISSUES_COMPONENT_INVALID;

  uint32_t hash = fnv1aHashUmbra(string);
  uint32_t index = hash % map->capacity;
  uint32_t steps = 0;

  do {
    // Empty slot, thus we couldn't find string
    if (map->data[index].string.size == 0)
      return _GITISSUES_COMPONENT_INVALID;

    // Check if slot matches
    if (map->data[index].stringHash == hash) {
      // Check string matches (OR: assume collisions are too rare or fail on
      // collisions)
      // TODO: if profiling concern, switch to the above assumption
      if (compare(map->data[index].string, string)) {
        return map->data[index].value;
      }
    }

    index = (index + 1) % map->capacity;
  } while (steps++ < map->capacity);

  return _GITISSUES_COMPONENT_INVALID;
}

void saveStringMap(struct StringMap const *map, FILE *p) {
  // struct StringMapNode* data;
  // uint32_t size;
  // uint32_t capacity;
  // uint32_t threshold;
  //
  // struct StringMapNode
  // struct UmbraString string;
  // uint32_t stringHash;
  // ComponentID value;
  // fwrite(&map->size, sizeof(map->size), 1, p);
  // fwrite(&map->capacity, sizeof(map->capacity), 1, p);
  // fwrite(&map->threshold, sizeof(map->threshold), 1, p);
  fwrite(&map->dense.size, sizeof(map->dense.size), 1, p);
  // fwrite(&map->dense.capacity, sizeof(map->dense.capacity), 1, p);

  for (uint32_t i = 0; i < map->dense.size; i++) {
    struct StringMapNode *node = &map->dense.data[i];
    fwrite(&node->value, sizeof(node->value), 1, p);
    saveUmbraString(&node->string, p);
  }
}

struct StringMap loadStringMap(FILE *p) {
  struct StringMap map = {0};
  map.stringAllocator = createBlockAllocator(4096);

  uint32_t numElements = 0;
  // fread(&map.threshold, sizeof(map.threshold), 1, p);
  fread(&numElements, sizeof(numElements), 1, p);

  // TODO: reserve space beforehand
  for (uint32_t i = 0; i < numElements; i++) {
    ComponentID value;
    fread(&value, sizeof(value), 1, p);
    struct UmbraString string = loadUmbraString(&map.stringAllocator, p);
    insertStringMap(&map, string, value);
  }

  return map;
}
