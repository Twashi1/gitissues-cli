#include <gitissues/iff/schema.h>

struct Schema defaultSchema;

void initSchema(void) {
  // TODO: read a schema from file instead of constructing it manually
  defaultSchema.aliases = createStringMap();

  struct UmbraString projectAlias;
  createUmbraStringParasitic(&projectAlias, "@");
  struct UmbraString titleAlias;
  createUmbraStringParasitic(&titleAlias, "");
  // in this case, componentID is going to be an index into tag metadata?
  insertStringMap(&defaultSchema.aliases, projectAlias, 0);
  insertStringMap(&defaultSchema.aliases, titleAlias, 1);

  createUmbraStringParasitic(&defaultSchema.separator, ";;");
  createUmbraStringParasitic(&defaultSchema.terminator, "\n");

  // TODO: tags?
}

void terminateSchema(void) { freeStringMap(&defaultSchema.aliases); }

struct Schema readSchema(char const *filename) {
  // schema format
  // declare separator, terminator
}
void dropSchema(struct Schema *schema);
