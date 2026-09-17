package gitissues

object GitIssues {
    fun init() = gitissues.jni.GitIssues.init()

    fun terminate() = gitissues.jni.GitIssues.terminate()

    fun loadSchema(filename: String): Schema =
        Schema(gitissues.jni.GitIssues.loadSchema(filename))

    fun freeSchema(schema: Schema) {
        gitissues.jni.GitIssues.freeSchema(schema.handle)
    }

    fun createIssue(schema: Schema): Issue =
        Issue(schema, gitissues.jni.GitIssues.createIssue(schema.handle))
}