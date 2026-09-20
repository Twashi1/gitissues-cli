package gitissues.jni;

import java.util.List;
import java.util.ArrayList;

public final class GitIssues {

  static {
      System.loadLibrary("gitissues_jni");
  }

  public static native void init();
  public static native void terminate();

  public static native long loadSchema(String filename);
  public static native void freeSchema(long schema);

  public static native List<Long> loadIFF(long schema, String filename);
  public static native void saveIFF(long schema, String filename, List<Long> issues);

  public static native long createIssue(long schema);

  public static native void attachTag(long schema, long issue, String tag, byte[] data);
  public static native void attachTagByID(long schema, long issue, long tagID, byte[] data);

  public static native void detachTag(long schema, long issue, String tag);
  public static native void detachTagByID(long schema, long issue, long tagID);

  public static native byte[] getTag(long schema, long issue, String tag);
  public static native byte[] getTagByID(long schema, long issue, long tagID);

  public static native boolean hasTag(long schema, long issue, String tag);
  public static native boolean hasTagByID(long schema, long issue, long tagID);
}
