#include <gitissues/api/api.h>
#include <gitissues/ecs/registry.h>
#include <gitissues/global.h>
#include <gitissues/iff/iff.h>
#include <gitissues/iff/schema.h>
#include <gitissues/issue.h>
#include <gitissues/umbra_string.h>
#include <jni.h>

_Static_assert(sizeof(jlong) >= sizeof(intptr_t),
               "jlong must be at least as large as intptr_t");
_Static_assert(sizeof(jlong) >= sizeof(struct Issue),
               "jlong must be at least as large as an issue struct");

// Cache for frequently used class and method IDs
static jclass arrayListClass = NULL;
static jmethodID arrayListConstructor = NULL;
static jmethodID arrayListAddMethod = NULL;
static jclass longClass = NULL;
static jmethodID longValueOfMethod = NULL;

static inline jlong IssueToID(struct Issue issue) {
  return (jlong)issue.entity;
}

static inline jlong SchemaToID(struct Schema *schema) {
  return (jlong)(intptr_t)schema;
}

static inline struct Issue IDToIssue(jlong id) {
  struct Issue issue;
  issue.entity = (Entity)id;

  return issue;
}

static inline struct Schema *IDToSchema(jlong id) {
  return (struct Schema *)(intptr_t)id;
}

// Initialize cached class and method IDs
static void cacheIds(JNIEnv *env) {
  // Cache ArrayList class and methods
  arrayListClass =
      (*env)->NewGlobalRef(env, (*env)->FindClass(env, "java/util/ArrayList"));
  arrayListConstructor =
      (*env)->GetMethodID(env, arrayListClass, "<init>", "()V");
  arrayListAddMethod =
      (*env)->GetMethodID(env, arrayListClass, "add", "(Ljava/lang/Object;)Z");

  // Cache Long class and methods
  longClass =
      (*env)->NewGlobalRef(env, (*env)->FindClass(env, "java/lang/Long"));
  longValueOfMethod = (*env)->GetStaticMethodID(env, longClass, "valueOf",
                                                "(J)Ljava/lang/Long;");
}

JNIEXPORT void JNICALL Java_gitissues_jni_GitIssues_init(JNIEnv *env,
                                                         jclass clazz) {
  (void)env;
  (void)clazz;

  gitissuesInit();
  // Cache frequently used class and method IDs
  cacheIds(env);
}

JNIEXPORT void JNICALL Java_gitissues_jni_GitIssues_terminate(JNIEnv *env,
                                                              jclass class) {
  (void)env;
  (void)class;

  gitissuesTerminate();
}

JNIEXPORT jlong JNICALL Java_gitissues_jni_GitIssues_loadSchema(
    JNIEnv *env, jclass clazz, jstring filename) {
  (void)env;
  (void)clazz;

  char const *charString = (*env)->GetStringUTFChars(env, filename, NULL);
  DEBUG_ASSERT(charString != NULL,
               "Out of memory getting string characters in loadSchema");

  struct Schema *schema =
      transientAllocate(sizeof(struct Schema), alignof(struct Schema));
  *schema = readSchema(charString);

  (*env)->ReleaseStringUTFChars(env, filename, charString);

  return SchemaToID(schema);
}

JNIEXPORT void JNICALL Java_gitissues_jni_GitIssues_freeSchema(JNIEnv *env,
                                                               jclass clazz,
                                                               jlong schema) {
  (void)env;
  (void)clazz;

  struct Schema *schemaPtr = IDToSchema(schema);
  freeSchema(schemaPtr);

  freeTransient(schemaPtr, sizeof(struct Schema));
}

JNIEXPORT jobject JNICALL Java_gitissues_jni_GitIssues_loadIFF(
    JNIEnv *env, jclass clazz, jlong schema, jstring filename) {
  (void)clazz;

  struct Schema *schemaPtr = IDToSchema(schema);
  char const *charString = (*env)->GetStringUTFChars(env, filename, NULL);
  DEBUG_ASSERT(charString != NULL,
               "Out of memory getting string characters in loadIFF");

  struct Issue *issues;
  uint32_t issuesSize;
  gitissuesLoadIFF(schemaPtr, charString, &issues, &issuesSize);

  (*env)->ReleaseStringUTFChars(env, filename, charString);

  // Create Java ArrayList<Long> using cached IDs
  jobject arrayList =
      (*env)->NewObject(env, arrayListClass, arrayListConstructor);

  for (uint32_t i = 0; i < issuesSize; i++) {
    jlong issueId = IssueToID(issues[i]);
    jobject longObject = (*env)->CallStaticObjectMethod(
        env, longClass, longValueOfMethod, issueId);
    (*env)->CallBooleanMethod(env, arrayList, arrayListAddMethod, longObject);
  }

  // Free the issues array (it was allocated by gitissuesLoadIFF)
  freeTransient(issues, issuesSize * sizeof(struct Issue));

  return arrayList;
}

JNIEXPORT void JNICALL Java_gitissues_jni_GitIssues_saveIFF(
    JNIEnv *env, jclass clazz, jlong schema, jstring filename, jobject issues) {
  (void)clazz;

  struct Schema *schemaPtr = IDToSchema(schema);
  char const *charString = (*env)->GetStringUTFChars(env, filename, NULL);
  DEBUG_ASSERT(charString != NULL,
               "Out of memory getting string characters in saveIFF");

  // Convert Java List<Long> to C array
  jclass listClass = (*env)->GetObjectClass(env, issues);
  jmethodID sizeMethod = (*env)->GetMethodID(env, listClass, "size", "()I");
  jmethodID getMethod =
      (*env)->GetMethodID(env, listClass, "get", "(I)Ljava/lang/Object;");

  jint listSize = (*env)->CallIntMethod(env, issues, sizeMethod);

  struct Issue *cIssues =
      transientAllocate(listSize * sizeof(struct Issue), alignof(struct Issue));

  jclass longClassLocal = (*env)->FindClass(env, "java/lang/Long");
  jmethodID longLongValue =
      (*env)->GetMethodID(env, longClassLocal, "longValue", "()J");

  for (jint i = 0; i < listSize; i++) {
    jobject longObject = (*env)->CallObjectMethod(env, issues, getMethod, i);
    jlong issueId = (*env)->CallLongMethod(env, longObject, longLongValue);
    cIssues[i] = IDToIssue(issueId);
  }

  (*env)->ReleaseStringUTFChars(env, filename, charString);

  gitissuesSaveIFF(schemaPtr, charString, cIssues, listSize);

  // Free the transient array
  freeTransient(cIssues, listSize * sizeof(struct Issue));
}

JNIEXPORT jlong JNICALL Java_gitissues_jni_GitIssues_createIssue(JNIEnv *env,
                                                                 jclass clazz,
                                                                 jlong schema) {
  (void)env;
  (void)clazz;

  struct Issue issue = gitissuesCreateIssue(IDToSchema(schema));

  return IssueToID(issue);
}

JNIEXPORT void JNICALL Java_gitissues_jni_GitIssues_attachTag(
    JNIEnv *env, jclass clazz, jlong schema, jlong issue, jstring tag,
    jbyteArray data) {
  (void)env;
  (void)clazz;

  struct Schema *schemaPtr = IDToSchema(schema);
  struct Issue issueStruct = IDToIssue(issue);

  // Get the byte array data
  jsize dataSize = (*env)->GetArrayLength(env, data);
  jbyte *dataBytes = (*env)->GetByteArrayElements(env, data, NULL);

  // Convert Java string to UmbraString
  const char *tagChars = (*env)->GetStringUTFChars(env, tag, NULL);
  DEBUG_ASSERT(tagChars != NULL, "Out of memory getting tag string");

  struct UmbraString tagString;
  createUmbraStringBoundParasitic(&tagString, tagChars, strlen(tagChars));

  // Attach the tag - this will auto-register if needed
  gitissuesAttachTag(schemaPtr, issueStruct, (void *)dataBytes,
                     (uint32_t)dataSize, tagString);

  (*env)->ReleaseStringUTFChars(env, tag, tagChars);
  (*env)->ReleaseByteArrayElements(env, data, dataBytes, 0);
}

JNIEXPORT void JNICALL Java_gitissues_jni_GitIssues_attachTagByID(
    JNIEnv *env, jclass clazz, jlong schema, jlong issue, jlong tagID,
    jbyteArray data) {
  (void)env;
  (void)clazz;

  struct Schema *schemaPtr = IDToSchema(schema);
  struct Issue issueStruct = IDToIssue(issue);

  // Get the byte array data
  jsize dataSize = (*env)->GetArrayLength(env, data);
  jbyte *dataBytes = (*env)->GetByteArrayElements(env, data, NULL);

  gitissuesAttachTagByID(schemaPtr, issueStruct, (void *)dataBytes,
                         (ComponentID)tagID);

  (*env)->ReleaseByteArrayElements(env, data, dataBytes, 0);
}

JNIEXPORT void JNICALL Java_gitissues_jni_GitIssues_detachTag(
    JNIEnv *env, jclass clazz, jlong schema, jlong issue, jstring tag) {
  (void)env;
  (void)clazz;

  struct Schema *schemaPtr = IDToSchema(schema);
  struct Issue issueStruct = IDToIssue(issue);

  // Convert Java string to UmbraString
  const char *tagChars = (*env)->GetStringUTFChars(env, tag, NULL);
  DEBUG_ASSERT(tagChars != NULL, "Out of memory getting tag string");

  struct UmbraString tagString;
  createUmbraStringBoundParasitic(&tagString, tagChars, strlen(tagChars));

  // Detach the tag
  gitissuesDetachTag(schemaPtr, issueStruct, tagString);

  (*env)->ReleaseStringUTFChars(env, tag, tagChars);
}

JNIEXPORT void JNICALL Java_gitissues_jni_GitIssues_detachTagByID(
    JNIEnv *env, jclass clazz, jlong schema, jlong issue, jlong tagID) {
  (void)env;
  (void)clazz;

  struct Schema *schemaPtr = IDToSchema(schema);
  struct Issue issueStruct = IDToIssue(issue);

  gitissuesDetachTagByID(schemaPtr, issueStruct, (ComponentID)tagID);
}

JNIEXPORT jbyteArray JNICALL Java_gitissues_jni_GitIssues_getTag(
    JNIEnv *env, jclass clazz, jlong schema, jlong issue, jstring tag) {
  (void)env;
  (void)clazz;

  struct Schema *schemaPtr = IDToSchema(schema);
  struct Issue issueStruct = IDToIssue(issue);

  // Convert Java string to UmbraString
  const char *tagChars = (*env)->GetStringUTFChars(env, tag, NULL);
  DEBUG_ASSERT(tagChars != NULL, "Out of memory getting tag string");

  struct UmbraString tagString;
  createUmbraStringBoundParasitic(&tagString, tagChars, strlen(tagChars));

  // Get the tag data
  void *tagData = gitissuesGetTag(schemaPtr, issueStruct, tagString);
  if (tagData == NULL) {
    (*env)->ReleaseStringUTFChars(env, tag, tagChars);
    return NULL;
  }

  // Get the size of the data from the component pool
  struct Registry *registry = &schemaPtr->registry;
  ComponentID tagID = getComponentID(registry, tagString);

  // Check if tag is valid
  if (tagID == _GITISSUES_COMPONENT_INVALID) {
    (*env)->ReleaseStringUTFChars(env, tag, tagChars);
    return NULL;
  }

  // Get the component pool for this tag ID to get its size
  struct ComponentPool *pool = getPool(registry, tagID);
  if (pool == NULL) {
    (*env)->ReleaseStringUTFChars(env, tag, tagChars);
    return NULL; // Tag not found in registry
  }

  uint32_t sizeOfType = pool->sizeOfType;
  if (sizeOfType == 0) {
    (*env)->ReleaseStringUTFChars(env, tag, tagChars);
    return NULL; // Invalid size
  }

  jbyteArray byteArray = (*env)->NewByteArray(env, sizeOfType);
  if (byteArray == NULL) {
    (*env)->ReleaseStringUTFChars(env, tag, tagChars);
    return NULL; // Out of memory
  }

  (*env)->SetByteArrayRegion(env, byteArray, 0, sizeOfType,
                             (const jbyte *)tagData);

  (*env)->ReleaseStringUTFChars(env, tag, tagChars);
  return byteArray;
}

JNIEXPORT jbyteArray JNICALL Java_gitissues_jni_GitIssues_getTagByID(
    JNIEnv *env, jclass clazz, jlong schema, jlong issue, jlong tagID) {
  (void)env;
  (void)clazz;

  struct Schema *schemaPtr = IDToSchema(schema);
  struct Issue issueStruct = IDToIssue(issue);

  void *tagData =
      gitissuesGetTagByID(schemaPtr, issueStruct, (ComponentID)tagID);
  if (tagData == NULL) {
    return NULL;
  }

  // Get the size of the data from the component pool
  struct Registry *registry = &schemaPtr->registry;
  ComponentID id = (ComponentID)tagID;

  // Check if tag is valid
  if (id == _GITISSUES_COMPONENT_INVALID) {
    return NULL;
  }

  // Get the component pool for this tag ID to get its size
  struct ComponentPool *pool = getPool(registry, id);
  if (pool == NULL) {
    return NULL; // Tag not found in registry
  }

  uint32_t sizeOfType = pool->sizeOfType;
  if (sizeOfType == 0) {
    return NULL; // Invalid size
  }

  jbyteArray byteArray = (*env)->NewByteArray(env, sizeOfType);
  if (byteArray == NULL) {
    return NULL; // Out of memory
  }

  (*env)->SetByteArrayRegion(env, byteArray, 0, sizeOfType,
                             (const jbyte *)tagData);
  return byteArray;
}

JNIEXPORT jboolean JNICALL Java_gitissues_jni_GitIssues_hasTag(
    JNIEnv *env, jclass clazz, jlong schema, jlong issue, jstring tag) {
  (void)env;
  (void)clazz;

  struct Schema *schemaPtr = IDToSchema(schema);
  struct Issue issueStruct = IDToIssue(issue);

  // Convert Java string to UmbraString
  const char *tagChars = (*env)->GetStringUTFChars(env, tag, NULL);
  DEBUG_ASSERT(tagChars != NULL, "Out of memory getting tag string");

  struct UmbraString tagString;
  createUmbraStringBoundParasitic(&tagString, tagChars, strlen(tagChars));

  // Check if tag exists
  jboolean result =
      gitissuesHasTag(schemaPtr, issueStruct, tagString) ? JNI_TRUE : JNI_FALSE;

  (*env)->ReleaseStringUTFChars(env, tag, tagChars);
  return result;
}

JNIEXPORT jboolean JNICALL Java_gitissues_jni_GitIssues_hasTagByID(
    JNIEnv *env, jclass clazz, jlong schema, jlong issue, jlong tagID) {
  (void)env;
  (void)clazz;

  struct Schema *schemaPtr = IDToSchema(schema);
  struct Issue issueStruct = IDToIssue(issue);

  return gitissuesHasTagByID(schemaPtr, issueStruct, (ComponentID)tagID)
             ? JNI_TRUE
             : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL Java_gitissues_jni_GitIssues_isNullIssue(
    JNIEnv *env, jclass clazz, jlong schema, jlong issue) {
  (void)env;
  (void)clazz;

  struct Schema *schemaPtr = IDToSchema(schema);
  struct Issue issueStruct = IDToIssue(issue);

  return gitissuesIsNullIssue(schemaPtr, issueStruct) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jbyteArray JNICALL Java_gitissues_jni_GitIssues_getOrCreateUUID(
    JNIEnv *env, jclass clazz, jlong schema, jlong issue) {
  (void)clazz;

  struct Schema *schemaPtr = IDToSchema(schema);
  struct Issue issueStruct = IDToIssue(issue);

  UUID7 uuid = gitissuesGetOrCreateUUID(schemaPtr, issueStruct);

  // Create a Java byte array from the C UUID7 struct
  jbyteArray byteArray = (*env)->NewByteArray(env, 16);
  if (byteArray == NULL) {
    return NULL;
  }

  jbyte bytes[16];
  for (int i = 0; i < 16; i++) {
    bytes[i] = (jbyte)uuid.bytes[i];
  }

  (*env)->SetByteArrayRegion(env, byteArray, 0, 16, bytes);

  return byteArray;
}

JNIEXPORT void JNICALL Java_gitissues_jni_GitIssues_removeIssues(
    JNIEnv *env, jclass clazz, jlong schema, jobject issues) {
  (void)env;
  (void)clazz;

  struct Schema *schemaPtr = IDToSchema(schema);

  // Convert Java List<Long> to C array
  jclass listClass = (*env)->GetObjectClass(env, issues);
  jmethodID sizeMethod = (*env)->GetMethodID(env, listClass, "size", "()I");
  jmethodID getMethod =
      (*env)->GetMethodID(env, listClass, "get", "(I)Ljava/lang/Object;");

  jint listSize = (*env)->CallIntMethod(env, issues, sizeMethod);

  struct Issue *cIssues =
      transientAllocate(listSize * sizeof(struct Issue), alignof(struct Issue));

  jclass longClassLocal = (*env)->FindClass(env, "java/lang/Long");
  jmethodID longLongValue =
      (*env)->GetMethodID(env, longClassLocal, "longValue", "()J");

  for (jint i = 0; i < listSize; i++) {
    jobject longObject = (*env)->CallObjectMethod(env, issues, getMethod, i);
    jlong issueId = (*env)->CallLongMethod(env, longObject, longLongValue);
    cIssues[i] = IDToIssue(issueId);
  }

  gitissuesRemoveIssues(schemaPtr, cIssues, listSize);

  // Free the transient array
  freeTransient(cIssues, listSize * sizeof(struct Issue));
}
