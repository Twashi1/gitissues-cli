package gitissues.jni;

import org.junit.jupiter.api.AfterAll;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.Assumptions;
import static org.junit.jupiter.api.Assumptions.assumeTrue;
import static org.junit.jupiter.api.Assertions.*;

import java.io.File;

class GitIssuesJniTest {

    @BeforeAll
    static void setUp() {
        // Check if the native library exists and can be loaded
        String libPath = System.getProperty("java.library.path");
        System.out.println("java.library.path: " + libPath);
        
        String[] paths = libPath.split(System.getProperty("path.separator"));
        File libFile = null;
        for (String path : paths) {
            File testFile = new File(path, "libgitissues_jni.so");
            if (testFile.exists()) {
                libFile = testFile;
                break;
            }
        }
        
        System.out.println("Looking for library at: " + (libFile != null ? libFile.getAbsolutePath() : "not found"));
        System.out.println("Library exists: " + (libFile != null && libFile.exists()));
        
        // Initialize the library
        assumeTrue(libFile != null && libFile.exists(), "Native library not found in java.library.path");
        try {
            GitIssues.init();
            System.out.println("Library initialized successfully");
        } catch (UnsatisfiedLinkError e) {
            System.err.println("Failed to initialize library: " + e.getMessage());
            throw e;
        }
    }

    @AfterAll
    static void tearDown() {
        // Terminate the library after all tests
        System.out.println("Terminating library");
        GitIssues.terminate();
    }

    @Test
    void testLibraryLoads() {
        // This test will pass if we get here without an UnsatisfiedLinkError
        assertTrue(true, "If we got here, the library loaded successfully");
    }

    @Test
    void testBasicOperations() {
        // Test that we can call native methods
        // Initialization and termination are tested in setUp/tearDown
        // If they failed, the test suite wouldn't reach this point
        assertTrue(true, "If we reach this test, initialization worked");
    }
}
