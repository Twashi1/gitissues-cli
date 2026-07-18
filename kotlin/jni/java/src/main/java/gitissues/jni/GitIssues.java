package gitissues.jni;

public final class GitIssues {

  static {
      System.loadLibrary("gitissues_jni");
  }

  public static native void init();
  public static native void terminate();

  public static native long registryCreate();
  public static native void registryFree(long handle);

  public static native long issueCreate(long registry);
  public static native void issueFree(long registry, long issue);

  public static native long registerTag(long registry, String name, Object codec);
  public static native long getTagID(long registry, String name);

  public static native void attachTag(long registry, long issue, long tagID, Object tagObject);
  public static native Object detachTag(long registry, long issue, long tagID);
  public static native Object getTag(long registry, long issue, long tagID);

  // Automatically uses the codec the user specified
  public static native void saveIssue(long registry, long issue, String filename);
  public static native long loadIssue(long registry, String filename);
}
