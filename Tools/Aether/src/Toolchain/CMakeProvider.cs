using System.Formats.Tar;
using System.IO.Compression;
using System.Runtime.InteropServices;

namespace Aether.Toolchain;

public static class CMakeProvider
{
    // TODO: Promote this to AETHER_TOOLCHAIN
    private const string CMakeVersion = "4.4.5";
    
    public static async Task<string> EnsureCMakeAsync(string workspaceRoot)
    {
        // Find CMake in the system path
        string cmake = OperatingSystem.IsWindows() ? "cmake.exe" : "cmake";

        string? systemCMake = FindInPath(cmake);
        if(systemCMake != null)
        {
            return systemCMake;
        }

        // Check if CMake already cached
        string cacheDir = Path.Combine(workspaceRoot, ".cache");
        string cmakeExe = Path.Combine(cacheDir, cmake);

        if(File.Exists(cmakeExe))
        {
            return cmakeExe;
        }

        // If doesn't exist, gather it from Github
        Directory.CreateDirectory(cacheDir);
        Console.WriteLine($"[Aether] CMake not found. Automatically downloading {CMakeVersion}...");

        string url = GetDownloadUrl();
        using var client = new HttpClient();
        byte[] byteData = await client.GetByteArrayAsync(url);

        // Check if compressed file is .tar.gz or .zip
        bool isTarGz = url.EndsWith(".tar.gz");
        using var mmStream = new MemoryStream(byteData);

        string cmakePath = isTarGz ? ExtractTarGz(mmStream, cacheDir) : ExtractZip(mmStream, cacheDir);

        if (cmakePath != null) return cmakePath;

        throw new FileNotFoundException("CMake not found.");
    }

    private static string ExtractZip(MemoryStream mmStream, string cacheDir)
    {
        using var archive = new ZipArchive(mmStream);


        foreach (var entry in archive.Entries)
        {
            if (entry.Name.StartsWith("cmake", StringComparison.OrdinalIgnoreCase) && !entry.Name.StartsWith("cmake-gui", StringComparison.OrdinalIgnoreCase))
            {
                string dest = Path.Combine(cacheDir, entry.Name);
                entry.ExtractToFile(dest, overwrite: true);

                // Give execution rights on macOS/Linux
                if (!OperatingSystem.IsWindows())
                {
                    File.SetUnixFileMode(dest,
                        UnixFileMode.UserRead | UnixFileMode.UserWrite | UnixFileMode.UserExecute |
                        UnixFileMode.GroupRead | UnixFileMode.GroupExecute |
                        UnixFileMode.OtherRead | UnixFileMode.OtherExecute);
                }

                Console.WriteLine($"[Aether] CMake ready at: {dest}");
                return dest;
            }
        }

        return null;
    }

    private static string ExtractTarGz(MemoryStream mmStream, string cacheDir)
    {
        mmStream.Position = 0;

        using var gzip = new GZipStream(mmStream, CompressionMode.Decompress, leaveOpen: true);
        using var archive = new TarReader(gzip);

        TarEntry? entry;
        while ((entry = archive.GetNextEntry()) != null)
        {
            if (entry.EntryType != TarEntryType.RegularFile)
                continue;

            string filename = Path.GetFileName(entry.Name);
            if (!filename.StartsWith("cmake", StringComparison.OrdinalIgnoreCase))
                continue;

            string dest = Path.Combine(cacheDir, filename);

            if (entry.DataStream == null)
                continue;

            if (!OperatingSystem.IsWindows())
            {
                File.SetUnixFileMode(dest,
                       UnixFileMode.UserRead | UnixFileMode.UserWrite | UnixFileMode.UserExecute |
                       UnixFileMode.GroupRead | UnixFileMode.GroupExecute |
                       UnixFileMode.OtherRead | UnixFileMode.OtherExecute);
            }

            Console.WriteLine($"[Aether] CMake ready at: {dest}");
            return dest;
        }

        return null;
    }

    private static string GetDownloadUrl()
    {
        if (OperatingSystem.IsWindows())
            return $"https://github.com/Kitware/CMake/releases/download/v{CMakeVersion}/cmake-{CMakeVersion}-windows-x86_64.zip";
        if(OperatingSystem.IsMacOS())                                       
            return $"https://github.com/Kitware/CMake/releases/download/v{CMakeVersion}/cmake-{CMakeVersion}-macos-universal.tar.gz";
        if (OperatingSystem.IsLinux())                                      
            return $"https://github.com/Kitware/CMake/releases/download/v{CMakeVersion}/cmake-{CMakeVersion}-linux-x86_64.tar.gz";

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