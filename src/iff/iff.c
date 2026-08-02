#include "gitissues/allocator.h"
#include "gitissues/defines.h"
#include "gitissues/ecs/string_map.h"
#include "gitissues/umbra_string.h"
#include <gitissues/iff/iff.h>

void readIFFFile(char const *filename, struct Registry *registry,
                 struct BlockAllocator *allocator, struct Issue **issues,
                 uint32_t *issuesSize, struct Schema const schema) {
  FILE *p = fopen(filename, "r");
  DEBUG_ASSERT(p != NULL, "Failed to open file for reading IFF");

  // TODO: there is no point in this, we should just read it in as a massive
  // allocation? the only benefit is we might double allocation size if we
  // create individual allocations for each tag we read store a lookup buffer of
  // size same as separator string make it a rolling buffer, which we
  // continually add characters to end, then shift down
  DEBUG_ASSERT(schema.terminator.size <= 8,
               "Cannot have terminator larger than 8 characters");

  struct {
    struct Issue *data;
    uint32_t size;
    uint32_t capacity;
  } issueArray;

  issueArray.data = NULL;
  issueArray.size = 0;
  issueArray.capacity = 0;

  ARRAY_RESERVE(issueArray, 8, ARRAY_GROWTH_PLUS_ONE);

  fseek(p, 0, SEEK_END);
  size_t fileSize = ftell(p);
  rewind(p);

  char *fileContent = malloc(fileSize + 1);
  DEBUG_ASSERT(fileContent != NULL, "Failed to allocate file content");

  size_t bytesRead = fread(fileContent, sizeof(char), fileSize, p);
  fileContent[bytesRead] = '\0';

  // TODO: annoying that we're using this workaround
  struct UmbraString umbraFileContent;
  umbraFileContent.size = bytesRead;
  umbraFileContent.prefix = 0;
  umbraFileContent.ptr = fileContent;

  size_t lastIssueStart = 0;

  bool reachedEOF = false;

  while (!reachedEOF) {
    // Read ahead from current pointer to look for separator
    uint32_t terminatorIndex =
        umbraFirstIndexOf(umbraFileContent, schema.terminator, lastIssueStart);

    // Consider EOF to be a terminator
    if (terminatorIndex == UINT32_MAX) {
      terminatorIndex = bytesRead;
      reachedEOF = true;
    }

    struct Issue issue = createIssue(registry);

    // look for the separator character
    // TODO: what about escape characters?
    uint32_t separatorIndex =
        umbraFirstIndexOf(umbraFileContent, schema.separator, lastIssueStart);

    // TODO: bad code, initialise umbra strings based on range
    // extract from issue start -> separator as "description"
    uint32_t descriptionSize = separatorIndex - lastIssueStart;
    struct UmbraString umbraDescription;
    createUmbraStringBoundAllocate(&umbraDescription,
                                   fileContent + lastIssueStart,
                                   descriptionSize, allocator);

    // TODO
  }

  free(fileContent);

  fclose(p);
}
