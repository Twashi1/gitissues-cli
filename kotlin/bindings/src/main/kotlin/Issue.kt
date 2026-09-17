package gitissues

class Issue internal constructor(
    private val schema: Schema,
    internal val handle: Long,
) {

    fun attachTag(tagName: String, data: ByteArray) {
        gitissues.jni.GitIssues.attachTag(
            schema.handle,
            handle,
            tagName,
            data
        )
    }

    fun attachTagByID(tagID: Long, data: ByteArray) {
        gitissues.jni.GitIssues.attachTagByID(
            schema.handle,
            handle,
            tagID,
            data
        )
    }

    fun <T> getTag(tagName: String): T? {
        val data = gitissues.jni.GitIssues.getTag(
            schema.handle,
            handle,
            tagName
        )
        return if (data == null) null else data as T?
    }

    fun <T> getTagByID(tagID: Long): T? {
        val data = gitissues.jni.GitIssues.getTagByID(
            schema.handle,
            handle,
            tagID
        )
        return if (data == null) null else data as T?
    }

    fun hasTag(tagName: String): Boolean =
        gitissues.jni.GitIssues.hasTag(
            schema.handle,
            handle,
            tagName
        )

    fun hasTagByID(tagID: Long): Boolean =
        gitissues.jni.GitIssues.hasTagByID(
            schema.handle,
            handle,
            tagID
        )

    fun detachTag(tagName: String) {
        gitissues.jni.GitIssues.detachTag(
            schema.handle,
            handle,
            tagName
        )
    }

    fun detachTagByID(tagID: Long) {
        gitissues.jni.GitIssues.detachTagByID(
            schema.handle,
            handle,
            tagID
        )
    }
}