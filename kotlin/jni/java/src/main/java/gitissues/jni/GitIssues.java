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

  public static native long loadSchema(String filename);
  public static native void freeSchema(long schema); 
  public static native long getRegistry(long schema);

  public static native void saveIssues(long schema, long[] issues, String filename);
  public static native long[] loadIssues(long schema, String filename);

  private static final class PoolIterator implements java.util.Iterator<Object> {
    // ptr is the dense map pointer 
    private final int count;
    private final long ptr;
    private final long sizeOfType; // always size of jobject?
    private int index;

    private PoolIterator(long ptr, int count, long sizeOfType) {
      this.count = count;
      this.ptr = ptr;
      this.sizeOfType = sizeOfType;
      this.index = 0;
    }

    @Override
    public boolean hasNext() {
      return index < count;
    }

    @Override
    public Object next() {
      if (!hasNext()) {
        throw new java.util.NoSuchElementException();
      }

      return nativeNext(ptr, index++, sizeOfType);
    }

    private static native Object nativeNext(long ptr, int index, long sizeOfType);
  }

  public static native PoolIterator iteratePool(long registry, long tagID);
}
