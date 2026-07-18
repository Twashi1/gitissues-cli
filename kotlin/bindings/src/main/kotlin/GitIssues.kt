package gitissues

object GitIssues {
    fun init() = gitissues.jni.GitIssues.init()

    fun terminate() = gitissues.jni.GitIssues.terminate()

    fun createRegistry(): Registry =
        Registry(
            gitissues.jni.GitIssues.registryCreate(),
        )
}
