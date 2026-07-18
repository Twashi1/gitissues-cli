package gitissues.jni;

public interface GitIssuesCodec<T> {
  String encode(T value);
  T decode(String value);
}
