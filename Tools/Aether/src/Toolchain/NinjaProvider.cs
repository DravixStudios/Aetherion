using System.IO.Compression;
using System.Runtime.InteropServices;

namespace Aether.Toolchain;

public static class NinjaProvider
{
    // TODO: Promote this to AETHER_TOOLCHAIN
    private const string NinjaVersion = "v1.13.2";
    
    public static async Task<string> EnsureNinjaAsync(string workspaceRoot)
    {
        // Find Ninja in the system path
        string ninja = OperatingSystem.IsWindows() ? "ninja.exe" : "ninja";

        string? systemNinja = FindInPath(ninja);
        if(systemNinja != null)
        {
            return systemNinja;
        }

        // Check if Ninja already cached
        string cacheDir = Path.Combine(workspaceRoot, ".cache");
        string ninjaExe = Path.Combine(cacheDir, ninja);

        if(File.Exists(ninjaExe))
        {
            return ninjaExe;
        }

        // If doesn't exist, gather it from Github
        Directory.CreateDirectory(cacheDir);
        Console.WriteLine($"[Aether] Ninja not found. Automatically downloading {NinjaVersion}...");

        string url = GetDownloadUrl();
        using var client = new HttpClient();
        byte[] zipBytes = await client.GetByteArrayAsync(url);

        using var zipStream = new MemoryStream(zipBytes);
        using var archive = new ZipArchive(zipStream);

        foreach (var entry in archive.Entries) 
        {
            if (entry.Name.StartsWith("ninja", StringComparison.OrdinalIgnoreCase))
            {
                string dest = Path.Combine(cacheDir, entry.Name);
                entry.ExtractToFile(dest, overwrite: true);

                // Give execution rights on macOS/Linux
                if (!OperatingSystem.IsWindows())
                {
                    File.SetUnixFileMode(dest, 
                        UnixFileMode.UserRead |UnixFileMode.UserWrite | UnixFileMode.UserExecute | 
                        UnixFileMode.GroupRead | UnixFileMode.GroupExecute |
                        UnixFileMode.OtherRead | UnixFileMode.OtherExecute);
                }

                Console.WriteLine($"[Aether] Ninja ready at: {dest}");
                return dest;
            }
        }

        throw new FileNotFoundException("Ninja not found.");
    }

    private static string GetDownloadUrl()
    {
        if (OperatingSystem.IsWindows())
            return $"https://github.com/ninja-build/ninja/releases/download/{NinjaVersion}/ninja-win.zip";
        if(OperatingSystem.IsMacOS())
            return $"https://github.com/ninja-build/ninja/releases/download/{NinjaVersion}/ninja-mac.zip";
        if (OperatingSystem.IsLinux())
            return $"https://github.com/ninja-build/ninja/releases/download/{NinjaVersion}/ninja-linux.zip";

        throw new PlatformNotSupportedException("Unsupported operating system");
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
                if(File.Exists(fullpath)) return fullpath;
            }
            catch
            {
                // Ignore invalid routes in PATH
            }
        }
        return null;
    }
}