namespace Aether.Toolchain;

public enum ToolchainFamily { MSVC, ClangGcc }

public class Toolchain
{
    public string Compiler {  get; }
    public string Linker { get; }
    public ToolchainFamily Family { get; }

    public Toolchain(string compiler, string linker, ToolchainFamily family)
    {
        Compiler = compiler;
        Linker = linker;
        Family = family;
    }

    public static Toolchain FromEnvironment()
    {
        string? env = Environment.GetEnvironmentVariable("AETHER_TOOLCHAIN");
        if (string.IsNullOrWhiteSpace(env))
        {
            // Windows users: check if MSVC environment is active or locate via VS
            if (OperatingSystem.IsWindows())
            {
                bool isClInPath = FindInPath("cl.exe") != null;
                if (!isClInPath)
                {
                    VisualStudioLocator.TrySetupMsvcEnvironment();
                }

                return new Toolchain("cl.exe", "link.exe", ToolchainFamily.MSVC);
            }

            /* 
             * Sorry Linux/macOS users... It's really difficult to guess which compiler are you using.
             * At least we know Windows users usually use MSVC
             * Song for you as an apology https://open.spotify.com/track/5Zo6dDs13wDivyjmvR5odE?si=dcbfe271b2a543ee 
             */
            throw new InvalidOperationException("AETHER_TOOLCHAIN=<compiler>;<linker> environment var not found.");
        }

        string[] parts = env.Split(';', StringSplitOptions.TrimEntries | StringSplitOptions.RemoveEmptyEntries);
        var family = parts[0].Contains("cl", StringComparison.OrdinalIgnoreCase) ? ToolchainFamily.MSVC : ToolchainFamily.ClangGcc;

        return new Toolchain(parts[0], parts[1], family);
    }

    private static string? FindInPath(string filename)
    {
        string? pathEnv = Environment.GetEnvironmentVariable("PATH");
        if (string.IsNullOrEmpty(pathEnv)) return null;

        string[] paths = pathEnv.Split(Path.PathSeparator);
        foreach (string dir in paths)
        {
            try
            {
                string fullpath = Path.Combine(dir, filename);
                if (File.Exists(fullpath)) return fullpath;
            }
            catch
            {
                // Ignore invalid routes in PATH
            }
        }
        return null;
    }
}
