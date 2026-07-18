package gitissues

class Registry internal constructor(
    internal val handle: Long,
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

    fun getTagID(name: String): Long =
        gitissues.jni.GitIssues.getTagID(
            handle,
            name,
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

    override fun close() {
        gitissues.jni.GitIssues.registryFree(handle)
    }
}
