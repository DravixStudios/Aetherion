using System.Diagnostics;

namespace Aether.Toolchain;

public static class VisualStudioLocator
{
    private static string? s_cachedVsPath;
    private static bool s_envInitialized;

    public static string? GetInstallationPath()
    {
        if (s_cachedVsPath != null)
            return s_cachedVsPath;

        if (!OperatingSystem.IsWindows())
            return null;

        string progFilesX86 = Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86);
        string vswherePath = Path.Combine(progFilesX86, "Microsoft Visual Studio", "Installer", "vswhere.exe");

        if (File.Exists(vswherePath))
        {
            try
            {
                var psi = new ProcessStartInfo
                {
                    FileName = vswherePath,
                    Arguments = "-latest -prerelease -property installationPath",
                    RedirectStandardOutput = true,
                    UseShellExecute = false,
                    CreateNoWindow = true
                };

                using var process = Process.Start(psi);
                if (process != null)
                {
                    string output = process.StandardOutput.ReadToEnd().Trim();
                    process.WaitForExit();
                    if (!string.IsNullOrEmpty(output) && Directory.Exists(output))
                    {
                        s_cachedVsPath = output;
                        return output;
                    }
                }
            }
            catch
            {
                // Ignore failure
            }
        }

        string[] standardDirs =
        {
            @"C:\Program Files\Microsoft Visual Studio\18\Community",
            @"C:\Program Files\Microsoft Visual Studio\2022\Community",
            @"C:\Program Files\Microsoft Visual Studio\2022\Professional",
            @"C:\Program Files\Microsoft Visual Studio\2022\Enterprise",
            @"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community",
            @"C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional",
            @"C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise"
        };

        foreach (var dir in standardDirs)
        {
            if (Directory.Exists(dir))
            {
                s_cachedVsPath = dir;
                return dir;
            }
        }

        return null;
    }

    public static string? FindNinja()
    {
        string? vsPath = GetInstallationPath();
        if (vsPath != null)
        {
            string candidate = Path.Combine(vsPath, "Common7", "IDE", "CommonExtensions", "Microsoft", "CMake", "Ninja", "ninja.exe");
            if (File.Exists(candidate))
                return candidate;
        }

        return null;
    }

    public static string? FindCMake()
    {
        string? vsPath = GetInstallationPath();
        if (vsPath != null)
        {
            string candidate = Path.Combine(vsPath, "Common7", "IDE", "CommonExtensions", "Microsoft", "CMake", "CMake", "bin", "cmake.exe");
            if (File.Exists(candidate))
                return candidate;
        }

        string standardCMake = @"C:\Program Files\CMake\bin\cmake.exe";
        if (File.Exists(standardCMake))
            return standardCMake;

        return null;
    }

    public static bool TrySetupMsvcEnvironment()
    {
        if (s_envInitialized || !OperatingSystem.IsWindows())
            return true;

        string? vsPath = GetInstallationPath();
        if (vsPath == null)
            return false;

        string vcvars64 = Path.Combine(vsPath, "VC", "Auxiliary", "Build", "vcvars64.bat");
        if (!File.Exists(vcvars64))
            return false;

        try
        {
            var psi = new ProcessStartInfo
            {
                FileName = "cmd.exe",
                Arguments = $"/c \"call \"{vcvars64}\" >nul && set\"",
                RedirectStandardOutput = true,
                UseShellExecute = false,
                CreateNoWindow = true
            };

            using var process = Process.Start(psi);
            if (process != null)
            {
                while (!process.StandardOutput.EndOfStream)
                {
                    string? line = process.StandardOutput.ReadLine();
                    if (string.IsNullOrEmpty(line)) continue;

                    int idx = line.IndexOf('=');
                    if (idx > 0)
                    {
                        string key = line.Substring(0, idx);
                        string value = line.Substring(idx + 1);
                        Environment.SetEnvironmentVariable(key, value);
                    }
                }
                process.WaitForExit();
                s_envInitialized = true;
                return true;
            }
        }
        catch
        {
            // Ignore failure
        }

        return false;
    }
}
