#ifndef _GITISSUES_CLI_CLI_C_
#define _GITISSUES_CLI_CLI_C_

#include "gitissues/iff/iff.h"
#include "gitissues/json/json.h"
#include "gitissues/log.h"

struct CLIContext {
  struct Registry registry;
  struct BlockAllocator allocator;
  struct Schema schema;
  struct UmbraString filename;
};

#endif
