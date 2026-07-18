package gitissues

interface Codec<T> {
    fun encode(value: T): String

    fun decode(value: String): T
}

internal class JniCodec<T>(
    private val codec: Codec<T>,
) : gitissues.jni.GitIssuesCodec<T> {
    override fun encode(value: T): String = codec.encode(value)

    override fun decode(value: String): T = codec.decode(value)
}
