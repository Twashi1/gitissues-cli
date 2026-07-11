import gitissues.GitIssues

fun main(args: Array<String>) {
    val a = args[0].toInt()
    val b = args[1].toInt()

    println(GitIssues().add(a, b))
}
