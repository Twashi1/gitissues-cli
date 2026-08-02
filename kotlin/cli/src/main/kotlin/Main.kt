import gitissues.Codec
import gitissues.GitIssues
import kotlinx.serialization.Serializable
import kotlinx.serialization.decodeFromString
import kotlinx.serialization.encodeToString
import kotlinx.serialization.json.Json

// TODO: will a realistic user even interact with this generally through a CLI? just provide good bindings for scripting...?
// TODO: we actually want to make a shell; not a CLI

// TODO: why do we have Serializable tag, we don't use it
@Serializable
data class PlayerTag(
    var name: String,
    var score: Int,
)

class PlayerTagCodec : Codec<PlayerTag> {
    private val json = Json

    override fun encode(value: PlayerTag): String = json.encodeToString(value)

    override fun decode(value: String): PlayerTag = json.decodeFromString(value)
}

fun main(args: Array<String>) {
    GitIssues.init()

    GitIssues.createRegistry().use { registry ->

        val playerTag =
            registry.registerTag(
                "player",
                PlayerTagCodec(),
            )

        val issue = registry.createIssue()

        issue.attachTag(
            playerTag,
            PlayerTag("Alice", 30),
        )

        val player: PlayerTag? = issue.getTag(playerTag)

        println(player)

        issue.save("issue.dat")

        println("Saved player object")

        issue.close()
    }

    println("Terminating program")

    GitIssues.terminate()
}
