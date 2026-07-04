#include "gitissues/tests/test.h"
#include <gitissues/tests/json.h>

static void testWriting(struct JsonContext *ctx) {

  pushTest(&ctx->suite, "Outputting to JSON file");
  FILE *jsonOut = fopen("jsonOut.json", "w");

  jsonWriteObjectBegin(jsonOut);

  jsonWriteKey("hello world", jsonOut);
  jsonWriteString("My string value", jsonOut);

  jsonWriteNext(jsonOut);

  jsonWriteKey("my integer array", jsonOut);
  jsonWriteArrayBegin(jsonOut);

  jsonWriteInt(1, jsonOut);
  jsonWriteNext(jsonOut);

  jsonWriteInt(2, jsonOut);
  jsonWriteNext(jsonOut);

  jsonWriteInt(3, jsonOut);

  jsonWriteArrayEnd(jsonOut);

  jsonWriteObjectEnd(jsonOut);

  fclose(jsonOut);

  testPassed(&ctx->suite, "No crash");
}

static void testReading(struct JsonContext *ctx) {
  pushHeader(&ctx->suite, "Reading data tests");

  struct JsonReader reader = jsonOpenFile("jsonOut.json");

  jsonReadObjectBegin(&reader);

  char *firstKey;
  char *firstValue;
  jsonReadKey(&reader, &ctx->allocator, &firstKey);
  jsonReadString(&reader, &ctx->allocator, &firstValue);

  pushTest(&ctx->suite, "First key check");
  TEST_PASS_CONDITION(&ctx->suite, strcmp("hello world", firstKey) == 0);
  pushTest(&ctx->suite, "First value check");
  TEST_PASS_CONDITION(&ctx->suite, strcmp("My string value", firstValue) == 0);

  jsonReadNext(&reader);

  char *intarrayKey;
  jsonReadKey(&reader, &ctx->allocator, &intarrayKey);
  jsonReadArrayBegin(&reader);

  pushTest(&ctx->suite, "Intarray key check");
  TEST_PASS_CONDITION(&ctx->suite,
                      strcmp("my integer array", intarrayKey) == 0);

  int32_t arrayValues[3];
  jsonReadInt32(&reader, &arrayValues[0]);
  jsonReadNext(&reader);

  jsonReadInt32(&reader, &arrayValues[1]);
  jsonReadNext(&reader);

  jsonReadInt32(&reader, &arrayValues[2]);

  pushTest(&ctx->suite, "Intarray value check");
  TEST_PASS_CONDITION(&ctx->suite, arrayValues[0] == 1 && arrayValues[1] == 2 &&
                                       arrayValues[2] == 3);

  jsonReadArrayEnd(&reader);

  jsonReadObjectEnd(&reader);

  jsonCloseFile(&reader);

  popHeader(&ctx->suite);
}

void testJson(void) {
  struct JsonContext ctx = {0};
  ctx.suite = createSuite("JSON test suite");
  ctx.allocator = createBlockAllocator(4096);

  testWriting(&ctx);
  testReading(&ctx);

  freeSuite(&ctx.suite);
  dropBlockAllocator(&ctx.allocator);
}
