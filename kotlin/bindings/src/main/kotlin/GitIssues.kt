package gitissues

class GitIssues {
    external fun add(
        a: Int,
        b: Int,
    ): Int

    companion object {
        init {
            System.loadLibrary("gitissues_jni")
        }
    }
}
