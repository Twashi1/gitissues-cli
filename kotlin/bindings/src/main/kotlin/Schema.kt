package gitissues

class Schema internal constructor(internal val handle: Long) : AutoCloseable {

    fun createIssue(): Issue =
        Issue(this, gitissues.jni.GitIssues.createIssue(handle))

    fun loadIFF(filename: String): List<Long> =
        gitissues.jni.GitIssues.loadIFF(handle, filename)

    fun saveIFF(filename: String, issues: List<Long>) {
        gitissues.jni.GitIssues.saveIFF(handle, filename, issues)
    }

    override fun close() {
        gitissues.jni.GitIssues.freeSchema(handle)
    }
}