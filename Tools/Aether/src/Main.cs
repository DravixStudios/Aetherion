using System.Diagnostics;
using Aether.Presets;
using Aether.Rules;
using Aether.Toolchain;

namespace Aether;

internal static class Program
{
    public enum CommandType
    {
        Build,
        ClearCache,
        GetDeps
    };

    public static CommandType ConvertCommand(string command)
    {
        return command.ToLowerInvariant() switch
        {
            "build" => CommandType.Build,
            "clearcache" or "clean" => CommandType.ClearCache,
            "get-deps" or "deps" => CommandType.GetDeps,
            _ => throw new ArgumentException($"Unknown command: '{command}'")
        };
    }

    static async Task<int> Main(string[] args)
    {
        Console.ForegroundColor = ConsoleColor.Cyan;
        Console.WriteLine("====================================");
        Console.WriteLine("   Aether(ion) Build Tool v0.1");
        Console.WriteLine("====================================");
        Console.ResetColor();

        string command;
        string[] cmdArgs;

        if (args.Length == 0)
        {
#if DEBUG
            command = "build";
            cmdArgs = args;
#else
            PrintUsage();
            return 1;
#endif
        }
        else
        {
            command = args[0];
            cmdArgs = args;
        }

        // Detect repo root by walking up from CWD first, then from binary location
        string? repoRoot = FindRepoRoot(Directory.GetCurrentDirectory())
                        ?? FindRepoRoot(AppDomain.CurrentDomain.BaseDirectory);

        if (repoRoot == null)
        {
            PrintError("Could not find project root. Run aether from within the Aetherion repository, or place the binary inside it.");
            return 1;
        }

        Console.WriteLine($"[Aether] Project root: {repoRoot}");

        CommandType cmd;
        try
        {
            cmd = ConvertCommand(command);
        }
        catch (Exception ex)
        {
            PrintError(ex.Message);
            PrintUsage();
            return 1;
        }

        switch (cmd)
        {
            case CommandType.ClearCache:
                return HandleClearCache(repoRoot);
            case CommandType.GetDeps:
                return await HandleGetDepsAsync(repoRoot, cmdArgs);
            case CommandType.Build:
                return await HandleBuildAsync(repoRoot, cmdArgs);
            default:
                return 1;
        }
    }

    private static async Task<int> HandleGetDepsAsync(string repoRoot, string[] args)
    {
        string targetName = args.Length > 1 ? args[1] : "Aetherion";
        string presetName = args.Length > 2 ? args[2] : "x64-debug-spdlog";

        try
        {
            string cacheDir = Path.Combine(repoRoot, ".cache");
            if (!Directory.Exists(cacheDir))
                Directory.CreateDirectory(cacheDir);

            string ninjaPath = await NinjaProvider.EnsureNinjaAsync(repoRoot);
            string cmakePath = await CMakeProvider.EnsureCMakeAsync(repoRoot);

            var preset = PresetManager.LoadPreset(repoRoot, presetName);
            string targetBin = Path.Combine(repoRoot, "Bin", targetName, preset.Configuration);

            Console.WriteLine($"[Aether] Resolving and fetching dependencies for target '{targetName}'...");
            var target = TargetLoader.ResolveTarget(repoRoot, targetName, cmakePath, ninjaPath, targetBin, preset);

            Console.ForegroundColor = ConsoleColor.Green;
            Console.WriteLine($"\n[OK] Dependencies for {target.Name} synchronized successfully.");
            Console.ResetColor();
            return 0;
        }
        catch (Exception ex)
        {
            PrintError(ex.Message);
            return 1;
        }
    }

    private static async Task<int> HandleBuildAsync(string repoRoot, string[] args)
    {
        string targetName = args.Length > 1 ? args[1] : "Aetherion";
        string presetName = args.Length > 2 ? args[2] : "x64-debug-spdlog";

        try
        {
            string cacheDir = Path.Combine(repoRoot, ".cache");
            if (!Directory.Exists(cacheDir))
                Directory.CreateDirectory(cacheDir);

            string ninjaPath = await NinjaProvider.EnsureNinjaAsync(repoRoot);
            string cmakePath = await CMakeProvider.EnsureCMakeAsync(repoRoot);

            var toolchain = Toolchain.Toolchain.FromEnvironment();
            Console.WriteLine($"[Aether] Compiler: {toolchain.Compiler} | Linker: {toolchain.Linker}");

            var preset = PresetManager.LoadPreset(repoRoot, presetName);
            Console.WriteLine($"[Aether] Preset: {preset.Name} ({preset.Configuration} / {preset.Architecture})");
            Console.WriteLine($"[Aether] Defines: {string.Join(", ", preset.Defines)}");

            string targetBin = Path.Combine(repoRoot, "Bin", targetName, preset.Configuration);

            var target = TargetLoader.ResolveTarget(repoRoot, targetName, cmakePath, ninjaPath, targetBin, preset);
            int totalCpp = target.SourceFiles.Count + target.ResolvedModules.Sum(m => m.SourceFiles.Count);
            Console.WriteLine($"[Aether] {target.Name}: {totalCpp} source files to compile");

            string ninjaFile = NinjaGenerator.Generate(repoRoot, target, toolchain, preset, targetBin);
            Console.WriteLine($"[Aether] Ninja file: {ninjaFile}");

            Console.ForegroundColor = ConsoleColor.Yellow;
            Console.WriteLine($"\n--- Building {target.Name} [{preset.Configuration}] ---");
            Console.ResetColor();

            var psi = new ProcessStartInfo
            {
                FileName = ninjaPath,
                Arguments = $"-f \"{ninjaFile}\"",
                UseShellExecute = false,
                WorkingDirectory = repoRoot
            };

            using var process = Process.Start(psi);
            process?.WaitForExit();

            if (process?.ExitCode != 0)
            {
                PrintError($"Build failed with exit code {process?.ExitCode}.");
                return process?.ExitCode ?? 1;
            }

            GenerateCompilationDatabase(ninjaPath, ninjaFile, repoRoot);

            Console.ForegroundColor = ConsoleColor.Green;
            Console.WriteLine($"\n[OK] Build completed successfully.");
            Console.WriteLine($"[OK] Output: Bin/{target.Name}/{preset.Configuration}/");
            Console.ResetColor();

            return 0;
        }
        catch (Exception ex)
        {
            PrintError(ex.Message);
            return 1;
        }
    }

    private static int HandleClearCache(string repoRoot)
    {
        string cacheDir = Path.Combine(repoRoot, ".cache");
        if (Directory.Exists(cacheDir))
        {
            try
            {
                Directory.Delete(cacheDir, recursive: true);
                Console.ForegroundColor = ConsoleColor.Green;
                Console.WriteLine("[Aether] .cache successfully deleted!");
                Console.ResetColor();
            }
            catch (Exception ex)
            {
                PrintError($"Couldn't delete .cache: {ex.Message}");
                return 1;
            }
        }
        else
        {
            Console.WriteLine("[Aether] Folder .cache was already clean.");
        }

        return 0;
    }

    private static void GenerateCompilationDatabase(string ninjaPath, string ninjaFile, string repoRoot)
    {
        try
        {
            var psi = new ProcessStartInfo
            {
                FileName = ninjaPath,
                Arguments = $"-f \"{ninjaFile}\" -t compdb compile",
                RedirectStandardOutput = true,
                UseShellExecute = false,
                WorkingDirectory = repoRoot
            };

            using var proc = Process.Start(psi);
            if (proc != null)
            {
                string json = proc.StandardOutput.ReadToEnd();
                proc.WaitForExit();
                File.WriteAllText(Path.Combine(repoRoot, "compile_commands.json"), json);
                Console.WriteLine("[Aether] compile_commands.json updated.");
            }
        }
        catch
        {
            // Ignore compdb failure
        }
    }

    private static void PrintError(string message)
    {
        Console.ForegroundColor = ConsoleColor.Red;
        Console.WriteLine($"\n[Aether] Error: {message}");
        Console.ResetColor();
    }

    private static void PrintUsage()
    {
        Console.WriteLine("\nUsage:");
        Console.WriteLine("  aether build <target> <preset>   Build a target");
        Console.WriteLine("  aether clearcache                Clear .cache intermediate folder");
        Console.WriteLine("  aether get-deps                  Sync external dependencies");
    }

    /// <summary>
    /// Walk up from <paramref name="startDir"/> looking for the Aetherion repository root.
    /// Returns the first directory that contains the repository markers, or null if the drive root is reached.
    /// </summary>
    private static string? FindRepoRoot(string startDir)
    {
        string? dir = Path.GetFullPath(startDir);
        while (dir != null)
        {
            if (File.Exists(Path.Combine(dir, "AetherPresets.json")) &&
                File.Exists(Path.Combine(dir, "CMakeLists.txt")) &&
                Directory.Exists(Path.Combine(dir, "Engine")) &&
                Directory.Exists(Path.Combine(dir, "Shared")))
            {
                return dir;
            }
            dir = Directory.GetParent(dir)?.FullName;
        }
        return null;
    }
}
