#include <gitissues/ecs/registry.h>
#include <gitissues/global.h>
#include <gitissues/issue.h>
#include <gitissues/umbra_string.h>
#include <jni.h>

// TODO: instead of returning pointers to issues, issues can just be an integer
// we return

struct GitIssuesCodecInfo {
  jobject codec;
  jclass clazz;
  // Cached fields
  jmethodID encode;
  jmethodID decode;
};

_Static_assert(sizeof(jlong) >= sizeof(intptr_t),
               "jlong must be at least as large as intptr_t");
_Static_assert(sizeof(jlong) >= sizeof(struct Issue),
               "jlong must be at least as large as an issue struct");

static struct GitIssuesCodecInfo createCodecInfo(JNIEnv *env, jobject codec) {
  jclass clazz = (*env)->GetObjectClass(env, codec);

  struct GitIssuesCodecInfo info = {0};

  info.codec = (*env)->NewGlobalRef(env, codec);
  info.clazz = (*env)->NewGlobalRef(env, clazz);

  info.encode = (*env)->GetMethodID(env, clazz, "encode",
                                    "(Ljava/lang/Object;)Ljava/lang/String;");

  info.decode = (*env)->GetMethodID(env, clazz, "decode",
                                    "(Ljava/lang/String;)Ljava/lang/Object;");

  return info;
}

JNIEXPORT void JNICALL Java_gitissues_jni_GitIssues_init(JNIEnv *env,
                                                         jclass class) {
  createGlobalContext();
}

JNIEXPORT jlong JNICALL
Java_gitissues_jni_GitIssues_registryCreate(JNIEnv *env, jclass class) {
  (void)env;
  (void)class;

  struct Registry *regPointer =
      lifetimeAllocate(sizeof(struct Registry), alignof(struct Registry));
  *regPointer = createRegistry();

  return (jlong)(intptr_t)regPointer;
}

JNIEXPORT void JNICALL Java_gitissues_jni_GitIssues_registryFree(JNIEnv *env,
                                                                 jclass class,
                                                                 jlong handle) {
  (void)env;
  (void)class;

  struct Registry *regPointer = (struct Registry *)(intptr_t)handle;

  freeRegistry(regPointer);
}

JNIEXPORT void JNICALL Java_gitissues_jni_GitIssues_terminate(JNIEnv *env,
                                                              jclass class) {
  freeGlobalContext();
}

JNIEXPORT jlong JNICALL Java_gitissues_jni_GitIssues_issueCreate(
    JNIEnv *env, jclass class, jlong registry) {
  (void)env;
  (void)class;

  struct Issue *issuePointer =
      transientAllocate(sizeof(struct Issue), alignof(struct Issue));
  *issuePointer = createIssue((struct Registry *)registry);

  return (jlong)(intptr_t)issuePointer;
}

JNIEXPORT void JNICALL Java_gitissues_jni_GitIssues_issueFree(JNIEnv *env,
                                                              jclass class,
                                                              jlong registry,
                                                              jlong issue) {
  (void)env;
  (void)class;

  freeIssue((struct Registry *)registry, (struct Issue *)issue);
  freeTransient((struct Issue *)issue, sizeof(struct Issue));
}

JNIEXPORT jlong JNICALL Java_gitissues_jni_GitIssues_getTagID(JNIEnv *env,
                                                              jclass class,
                                                              jlong registry,
                                                              jstring string) {
  (void)class;

  struct Registry *registryPointer = (struct Registry *)registry;

  char const *charString = (*env)->GetStringUTFChars(env, string, NULL);
  DEBUG_ASSERT(charString != NULL,
               "Out of memory getting string characters in getTagID");

  struct UmbraString umbra;
  createUmbraStringTransient(&umbra, charString, getGlobalTransientAllocator());

  DEBUG_ASSERT(isRegistered(registryPointer, umbra),
               "Tag was not already registered");

  jlong res = (jlong)getComponentID(registryPointer, umbra);

  freeUmbraStringTransient(&umbra, getGlobalTransientAllocator());

  return res;
}

JNIEXPORT void JNICALL Java_gitissues_jni_GitIssues_registerTag(
    JNIEnv *env, jclass class, jlong registry, jstring string, jobject codec) {
  (void)class;

  struct Registry *registryPointer = (struct Registry *)registry;

  struct GitIssuesCodecInfo info = createCodecInfo(env, codec);
  // TODO: should be lifetime attached to registry if possible
  struct GitIssuesCodecInfo *infoPtr = lifetimeAllocate(
      sizeof(struct GitIssuesCodecInfo), alignof(struct GitIssuesCodecInfo));
  *infoPtr = info;

  char const *charString = (*env)->GetStringUTFChars(env, string, NULL);
  DEBUG_ASSERT(charString != NULL,
               "Out of memory getting string characters in registerTag");

  // Attach to lifetime of registry
  struct UmbraString umbra;
  createUmbraStringAllocate(&umbra, charString,
                            &registryPointer->lifetimeAllocations);

  ComponentID id = registerTag(registryPointer, umbra, sizeof(jobject));

  setUserData(registryPointer, id, infoPtr);

  (*env)->ReleaseStringUTFChars(env, string, charString);
}

JNIEXPORT void JNICALL Java_gitissues_jni_GitIssues_attachTag(
    JNIEnv *env, jclass class, jlong registry, jlong issue, jlong tagId,
    jobject data) {
  (void)class;

  struct Registry *registryPointer = (struct Registry *)registry;

  jobject global = (*env)->NewGlobalRef(env, data);

  addTagById(registryPointer, (struct Issue *)issue, (ComponentID)tagId,
             (uint8_t *)&global);
}

JNIEXPORT jobject JNICALL Java_gitissues_jni_GitIssues_detachTag(
    JNIEnv *env, jclass class, jlong registry, jlong issue, jlong tagId) {
  (void)class;

  uint8_t *tagData = getTagById((struct Registry *)registry,
                                (struct Issue *)issue, (ComponentID)tagId);
  jobject global = *(jobject *)tagData;
  jobject local = (*env)->NewLocalRef(env, global);

  removeTagById((struct Registry *)registry, (struct Issue *)issue,
                (ComponentID)tagId);

  (*env)->DeleteGlobalRef(env, global);

  return local;
}

JNIEXPORT jobject JNICALL Java_gitissues_jni_GitIssues_getTag(
    JNIEnv *env, jclass class, jlong registry, jlong issue, jlong tagId) {
  (void)env;
  (void)class;

  uint8_t *tagData = getTagById((struct Registry *)registry,
                                (struct Issue *)issue, (ComponentID)tagId);
  jobject global = *(jobject *)tagData;

  return global;
}

JNIEXPORT void JNICALL Java_gitissues_jni_GitIssues_saveIssue(
    JNIEnv *env, jclass class, jlong registry, jlong issue, jstring filename) {
  (void)class;

  // TODO: repeating code for saving/loading entity json
  struct Issue *issuePtr = (struct Issue *)(intptr_t)issue;
  struct Registry *registryPtr = (struct Registry *)(intptr_t)registry;

  // Open up file to save issue into
  char const *filenameCstring = (*env)->GetStringUTFChars(env, filename, NULL);

  FILE *p = fopen(filenameCstring, "r");
  DEBUG_ASSERT(p != NULL, "Failed to open file");
  (*env)->ReleaseStringUTFChars(env, filename, filenameCstring);

  jsonWriteObjectBegin(p);
  bool first = true;

  // Iterate pools to find each component
  for (uint32_t i = 0; i < registryPtr->pools.size; i++) {
    struct ComponentPool *pool = &registryPtr->pools.data[i];

    if (!hasComponent(registryPtr, issuePtr->entity, (ComponentID)i)) {
      continue;
    }

    uint8_t *tagData = getEntityComponentPool(pool, issuePtr->entity);
    struct GitIssuesCodecInfo *info = getUserDataComponentPool(pool);
    jobject global = *(jobject *)tagData;

    jstring codecString =
        (*env)->CallObjectMethod(env, info->codec, info->encode, global);
    DEBUG_ASSERT(codecString != NULL, "Codec encode returned null string");

    char const *codecCstring =
        (*env)->GetStringUTFChars(env, codecString, NULL);

    if (first) {
      jsonWriteNext(p);
      first = false;
    }

    struct UmbraString componentName =
        registryPtr->componentIDMap.dense.data[i].string;
    jsonWriteKeyUmbra(componentName, p);

    jsonWriteObjectBegin(p);

    fwrite(codecCstring, sizeof(char),
           (*env)->GetStringUTFLength(env, codecString), p);

    jsonWriteObjectEnd(p);

    (*env)->ReleaseStringUTFChars(env, codecString, codecCstring);
  }

  jsonWriteObjectEnd(p);

  fclose(p);
}

JNIEXPORT jlong JNICALL Java_gitissues_jni_GitIssues_loadIssue(
    JNIEnv *env, jclass class, jlong registry, jstring filename) {
  (void)class;

  struct Registry *registryPtr = (struct Registry *)(intptr_t)registry;
  struct Issue *issuePtr =
      transientAllocate(sizeof(struct Issue), alignof(struct Issue));
  *issuePtr = createIssue(registryPtr);

  char const *filenameCstring = (*env)->GetStringUTFChars(env, filename, NULL);

  struct JsonReader reader = jsonOpenFile(filenameCstring);
  (*env)->ReleaseStringUTFChars(env, filename, filenameCstring);

  jsonReadObjectBegin(&reader);

  do {
    char *componentName;
    jsonReadKeyTransient(&reader, getGlobalTransientAllocator(),
                         &componentName);
    struct UmbraString umbra;
    createUmbraStringLifetime(&umbra, componentName);
    DEBUG_ASSERT(isRegistered(registryPtr, umbra),
                 "Expected component to already be registered before decoding");

    ComponentID id = getComponentID(registryPtr, umbra);

    freeTransient(componentName, 0);
    // Given component name
    // - find component id
    // - check registered
    // - get codec
    // - use codec method to construct
    // - attach to the entity (would be nicer in-place, but whatever)
    // - return issue

    struct GitIssuesCodecInfo *info = getUserData(registryPtr, id);
    DEBUG_ASSERT(info != NULL, "Need codec info to decrypt");

    // Length of the string starting at { and ending at }
    size_t objectStringLength = jsonGetLengthMatchObject(&reader);
    size_t decodeLength =
        objectStringLength - 1; // Subtract one to remove curly braces
    char *stringBuf =
        transientAllocate((decodeLength + 1) * sizeof(char),
                          alignof(char)); // But add one for null-terminator
    stringBuf[decodeLength] = '\0';

    jsonReadObjectBegin(&reader);

    memcpy(stringBuf, &reader.data[reader.pos], sizeof(char) * decodeLength);
    jstring jsonData = (*env)->NewStringUTF(env, stringBuf);

    // TODO: need to read until the closing object, and find the length
    jobject object =
        (*env)->CallObjectMethod(env, info->codec, info->decode, jsonData);
    DEBUG_ASSERT(object != NULL, "Codec decode returned null object");

    // Add object to entity
    jobject global = (*env)->NewGlobalRef(env, object);
    addTagById(registryPtr, issuePtr, id, (uint8_t *)&global);

    jsonReadObjectEnd(&reader);

    freeTransient(stringBuf, (decodeLength + 1) * sizeof(char));

  } while (jsonReadNext(&reader));

  jsonReadObjectEnd(&reader);

  jsonCloseFile(&reader);

  return (jlong)(intptr_t)issuePtr;
}
