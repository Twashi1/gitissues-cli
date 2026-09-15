package gitissues

class Registry internal constructor(
    internal val handle: Long,
    internal val schema: Long,
) : AutoCloseable {
    fun createIssue(): Issue =
        Issue(
            this,
            gitissues.jni.GitIssues.issueCreate(handle),
        )

    fun <T> registerTag(
        name: String,
        codec: Codec<T>,
    ): Tag<T> =
        Tag(
            gitissues.jni.GitIssues.registerTag(
                handle,
                name,
                JniCodec(codec),
            ),
        )

    fun <T> getTagObject(name: String): Tag<T> =
        Tag(
            gitissues.jni.GitIssues.getTagID(
                handle,
                name,
            ),
        )

    fun loadIssue(filename: String): Issue {
        val issue =
            gitissues.jni.GitIssues.loadIssue(
                handle,
                filename,
            )

        return Issue(
            this,
            issue,
        )
    }

    fun saveAllIssues(filename: String): Nothing {
        gitissues.jni.GitIssues.saveAllIssues(
            handle,
            filename,
        )
    }

    fun loadAllIssues(filename: String): List<Issue> {
        val issues =
            gitissues.jni.GitIssues.loadAllIssues(
                handle,
                filename,
            )

        return issues.map { Issue(this, it) }
    }

    override fun close() {
        gitissues.jni.GitIssues.registryFree(handle)
    }
}
