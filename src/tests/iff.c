#include "gitissues/iff/iff.h"
#include "gitissues/tests/test.h"
#include "gitissues/umbra_string.h"
#include <gitissues/tests/iff.h>

struct TestingTagSpec {
  char const *alias;
  char const *name;
  enum SchemaPropertyTypes type;
  bool required;
};

static bool checkTagSpecMatches(struct TestingTagSpec *spec,
                                struct TagMetadata *tagMeta) {
  if (!umbraCompareString(tagMeta->alias, spec->alias)) {
    return false;
  }

  if (!umbraCompareString(tagMeta->name, spec->name)) {
    return false;
  }

  if (tagMeta->type != spec->type) {
    return false;
  }

  if (tagMeta->isRequired != spec->required) {
    return false;
  }

  return true;
}

static bool checkIssueDescription(struct Issue *issue,
                                  struct IFFContext *context,
                                  char const *expected) {
  TEST_FAIL_IF_MSG(&context->suite,
                   !hasComponent(&context->registry, issue->entity,
                                 context->schema.descriptionID),
                   "Loaded issue must have description component");

  struct UmbraString *description = (struct UmbraString *)getTagById(
      &context->registry, *issue, context->schema.descriptionID);
  DEBUG_ASSERT(description != NULL, "Description data must exist");

  return umbraCompareString(*description, expected);
}

static void testIssue(struct IFFContext *context) {
  pushTest(&context->suite, "Loading issue from file");
  struct Issue *issues = NULL;
  uint32_t issuesSize = 0;

  readIFFFile("./examples/issue.iff", &context->registry, &context->allocator,
              &issues, &issuesSize, &context->schema);

  testPassed(&context->suite, "Issue loaded successfully");

  // Test number of issues
  pushTest(&context->suite, "Correct number of issues loaded");
  uint32_t const expectedIssueCount = 2;
  TEST_PASS_CONDITION(&context->suite, issuesSize == expectedIssueCount);

  // Test issue text
  pushHeader(&context->suite, "Issue text");

  // TODO: expecting loaded in order of appearance, ideally we're okay with any
  // ordering
  pushTest(&context->suite, "Issue 0 text matches expectations");
  TEST_PASS_CONDITION(
      &context->suite,
      checkIssueDescription(&issues[0], context, "Example issue text"));

  pushTest(&context->suite, "Issue 1 text matches expectations");
  TEST_PASS_CONDITION(
      &context->suite,
      checkIssueDescription(&issues[1], context, "Second issue"));

  popHeader(&context->suite);

  // TODO: Check issue have correct project tag and entity tag data
}

static void testSchema(struct IFFContext *context) {
  pushTest(&context->suite, "Loading schema from file");
  context->schema =
      readSchema("./examples/defaultSchema.json", &context->registry);
  testPassed(&context->suite, "Schema loaded successfully");

  pushTest(&context->suite, "Schema loaded separator/terminator correctly");
  // Test separator, terminator
  TEST_FAIL_IF_MSG(&context->suite,
                   !umbraCompareString(context->schema.separator, "::"),
                   "Expected separator to be ::");
  TEST_FAIL_IF_MSG(&context->suite,
                   !umbraCompareString(context->schema.terminator, ";"),
                   "Expected terminator to be ;");
  testPassed(&context->suite, NULL);

  // Test tag list, that each tag has correct properties

  pushHeader(&context->suite, "Schema tag list");

  uint32_t const expectedTagCount = 2;

  pushTest(&context->suite, "Correct number of tags loaded");
  uint32_t numValidTags = 0;
  // Iterate and count number of valid tags
  for (uint32_t i = 0; i < context->schema.tagMeta.capacity; i++) {
    struct TagMetadata *tagMeta = &context->schema.tagMeta.data[i];

    if (tagMeta->type != SCHEMA_TYPE_INVALID) {
      numValidTags++;

      GITISSUES_LOG_DEBUG("Found valid tag at %d, prefix was %.4s, type was %d",
                          i, (char const *)&tagMeta->name.prefix,
                          (int)tagMeta->type);
    }
  }

  GITISSUES_LOG_DEBUG("Found %d valid tags", numValidTags);

  TEST_PASS_CONDITION(&context->suite, numValidTags == expectedTagCount);

  struct TestingTagSpec tagSpecs[] = {
      {"@", "project", SCHEMA_TYPE_STRING, false},
      {"$", "entity", SCHEMA_TYPE_INT64, false}};

  DEBUG_ASSERT(sizeof(tagSpecs) / sizeof(tagSpecs[0]) == expectedTagCount,
               "Mismatched tag spec count");

  bool tagSpecFound[] = {false, false};

  DEBUG_ASSERT(sizeof(tagSpecFound) / sizeof(tagSpecFound[0]) ==
                   expectedTagCount,
               "Mismatched tag spec count");

  for (uint32_t i = 0; i < context->schema.tagMeta.capacity; i++) {
    if (context->schema.tagMeta.data[i].type == SCHEMA_TYPE_INVALID) {
      continue;
    }

    for (uint32_t j = 0; j < expectedTagCount; j++) {
      if (checkTagSpecMatches(&tagSpecs[j], &context->schema.tagMeta.data[i])) {
        tagSpecFound[j] = true;
        break;
      }
    }

    // TODO: Validate additional properties of the tag metadata
  }

  bool allTagsFound = true;

  for (uint32_t i = 0; i < expectedTagCount; i++) {
    if (!tagSpecFound[i]) {
      allTagsFound = false;
      break;
    }
  }

  if (allTagsFound) {
    testPassed(&context->suite,
               "All tags found with good properties matching expectation");
  }

  // TODO: more complex tests; what about tags that don't specify type

  popHeader(&context->suite);
}

void testIFF(void) {
  struct IFFContext context = {0};
  context.suite = createSuite("IFF");
  context.allocator = createBlockAllocator(1024);
  context.registry = createRegistry();

  pushHeader(&context.suite, "Schema");

  testSchema(&context);

  popHeader(&context.suite);

  pushHeader(&context.suite, "Issue");

  testIssue(&context);

  popHeader(&context.suite);

  freeSuite(&context.suite);
  freeRegistry(&context.registry);
  freeBlockAllocator(&context.allocator);
}
