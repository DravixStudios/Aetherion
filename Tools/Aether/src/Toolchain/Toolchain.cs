namespace Aether.Toolchain;

public enum ToolchainFamily { MSVC, ClangGcc }

public class Toolchain
{
    public string Compiler { get; }
    public string CCompiler { get; }
    public string Linker { get; }
    public ToolchainFamily Family { get; }

    public Toolchain(string compiler, string linker, ToolchainFamily family, string? cCompiler = null)
    {
        Compiler = compiler;
        Linker = linker;
        Family = family;
        CCompiler = string.IsNullOrWhiteSpace(cCompiler)
            ? DeriveCCompiler(compiler, family)
            : cCompiler;
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

                return new Toolchain("cl.exe", "link.exe", ToolchainFamily.MSVC, "cl.exe");
            }

            if (OperatingSystem.IsMacOS() || OperatingSystem.IsLinux())
            {
                string? cxx = Environment.GetEnvironmentVariable("CXX");
                if (string.IsNullOrWhiteSpace(cxx))
                {
                    string[] candidates = OperatingSystem.IsMacOS()
                        ? new[] { "clang++", "g++", "c++" }
                        : new[] { "g++", "clang++", "c++" };

                    foreach (string candidate in candidates)
                    {
                        if (FindInPath(candidate) != null)
                        {
                            cxx = candidate;
                            break;
                        }
                    }
                }

                if (string.IsNullOrWhiteSpace(cxx))
                {
                    throw new InvalidOperationException(
                        "No C++ compiler found in PATH (tried clang++, g++, c++). Install Xcode Command Line Tools / GCC or Clang, or set AETHER_TOOLCHAIN=<compiler>;<linker>.");
                }

                string? cc = Environment.GetEnvironmentVariable("CC");
                if (string.IsNullOrWhiteSpace(cc))
                {
                    cc = DeriveCCompiler(cxx, ToolchainFamily.ClangGcc);
                }

                return new Toolchain(cxx, cxx, ToolchainFamily.ClangGcc, cc);
            }

            throw new InvalidOperationException("AETHER_TOOLCHAIN=<compiler>;<linker> environment var not found.");
        }

        string[] parts = env.Split(';', StringSplitOptions.TrimEntries | StringSplitOptions.RemoveEmptyEntries);
        string rawCompiler = parts[0];
        string rawLinker = parts.Length > 1 ? parts[1] : rawCompiler;
        string compilerFile = Path.GetFileName(rawCompiler);
        var family = compilerFile.Equals("cl", StringComparison.OrdinalIgnoreCase) ||
                     compilerFile.Equals("cl.exe", StringComparison.OrdinalIgnoreCase)
            ? ToolchainFamily.MSVC
            : ToolchainFamily.ClangGcc;

        string? envCc = Environment.GetEnvironmentVariable("CC");
        if (family == ToolchainFamily.ClangGcc)
        {
            string cCompiler = string.IsNullOrWhiteSpace(envCc)
                ? DeriveCCompiler(rawCompiler, family)
                : envCc;
            string cxxCompiler = DeriveCxxCompiler(rawCompiler);
            string cxxLinker = DeriveCxxLinker(rawLinker, cxxCompiler);
            return new Toolchain(cxxCompiler, cxxLinker, family, cCompiler);
        }

        return new Toolchain(rawCompiler, rawLinker, family, envCc);
    }

    private static string DeriveCxxCompiler(string compiler)
    {
        string? dir = Path.GetDirectoryName(compiler);
        string file = Path.GetFileName(compiler);

        string cxxFile;
        if (file.Equals("gcc", StringComparison.OrdinalIgnoreCase))
            cxxFile = "g++";
        else if (file.StartsWith("gcc-", StringComparison.OrdinalIgnoreCase))
            cxxFile = "g++" + file.Substring(3);
        else if (file.Equals("clang", StringComparison.OrdinalIgnoreCase))
            cxxFile = "clang++";
        else if (file.StartsWith("clang-", StringComparison.OrdinalIgnoreCase))
            cxxFile = "clang++" + file.Substring(5);
        else if (file.Equals("cc", StringComparison.OrdinalIgnoreCase))
            cxxFile = "c++";
        else
            return compiler;

        string candidate = string.IsNullOrEmpty(dir) ? cxxFile : Path.Combine(dir, cxxFile);
        if (Path.IsPathRooted(candidate))
            return File.Exists(candidate) ? candidate : compiler;

        return FindInPath(candidate) != null ? candidate : compiler;
    }

    private static string DeriveCxxLinker(string linker, string cxxCompiler)
    {
        string file = Path.GetFileName(linker);
        if (file.Equals("ld", StringComparison.OrdinalIgnoreCase) ||
            file.Equals("ld.lld", StringComparison.OrdinalIgnoreCase) ||
            file.Equals("ld64.lld", StringComparison.OrdinalIgnoreCase) ||
            file.Equals("gold", StringComparison.OrdinalIgnoreCase) ||
            file.Equals("lld", StringComparison.OrdinalIgnoreCase))
        {
            return cxxCompiler;
        }

        return DeriveCxxCompiler(linker);
    }

    private static string DeriveCCompiler(string compiler, ToolchainFamily family)
    {
        if (family == ToolchainFamily.MSVC)
            return compiler;

        string? dir = Path.GetDirectoryName(compiler);
        string file = Path.GetFileName(compiler);

        string cFile;
        if (file.Contains("clang++", StringComparison.OrdinalIgnoreCase))
            cFile = file.Replace("clang++", "clang", StringComparison.OrdinalIgnoreCase);
        else if (file.Contains("g++", StringComparison.OrdinalIgnoreCase))
            cFile = file.Replace("g++", "gcc", StringComparison.OrdinalIgnoreCase);
        else if (file.Equals("c++", StringComparison.OrdinalIgnoreCase))
            cFile = "cc";
        else
            return compiler;

        string candidate = string.IsNullOrEmpty(dir) ? cFile : Path.Combine(dir, cFile);
        if (Path.IsPathRooted(candidate))
            return File.Exists(candidate) ? candidate : compiler;

        return FindInPath(candidate) != null ? candidate : compiler;
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
