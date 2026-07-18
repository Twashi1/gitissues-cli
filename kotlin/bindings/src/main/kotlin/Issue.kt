package gitissues

class Issue internal constructor(
    private val registry: Registry,
    internal val handle: Long,
) : AutoCloseable {
    fun <T> attachTag(
        tag: Tag<T>,
        value: T,
    ) {
        gitissues.jni.GitIssues.attachTag(
            registry.handle,
            handle,
            tag.id,
            value,
        )
    }

    @Suppress("UNCHECKED_CAST")
    fun <T> getTag(tag: Tag<T>): T? =
        gitissues.jni.GitIssues.getTag(
            registry.handle,
            handle,
            tag.id,
        ) as T?

    @Suppress("UNCHECKED_CAST")
    fun <T> detachTag(tag: Tag<T>): T? =
        gitissues.jni.GitIssues.detachTag(
            registry.handle,
            handle,
            tag.id,
        ) as T?

    fun save(filename: String) {
        gitissues.jni.GitIssues.saveIssue(
            registry.handle,
            handle,
            filename,
        )
    }

    override fun close() {
        gitissues.jni.GitIssues.issueFree(
            registry.handle,
            handle,
        )
    }
}
