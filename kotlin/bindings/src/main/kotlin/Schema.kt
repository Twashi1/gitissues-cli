package gitissues

class Schema internal constructor(
    internal val handle: Long,
) : AutoCloseable {
    fun getRegistry(): Registry =
        Registry(
            gitissues.jni.GitIssues.getRegistry(
                handle,
            ),
            handle,
        )

    override fun close() {
        gitissues.jni.GitIssues.freeSchema(
            handle,
        )
    }
}
