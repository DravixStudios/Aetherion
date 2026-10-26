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
            // Fallback method for Windows users
            if (OperatingSystem.IsWindows())
                return new Toolchain("cl.exe", "link.exe", ToolchainFamily.MSVC);

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
}