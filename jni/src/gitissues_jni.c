#include <gitissues/umbra_string.h>
#include <jni.h>

JNIEXPORT jint JNICALL Java_gitissues_GitIssues_add(JNIEnv *env, jobject obj,
                                                    jint a, jint b) {
  return a + b;
}
