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
        if (systemCMake != null)
        {
            return systemCMake;
        }

        // Check if CMake already cached
        string cacheDir = Path.Combine(workspaceRoot, ".cache");
        string cmakeDir = Path.Combine(cacheDir, $"cmake-{CMakeVersion}");
        string cmakeExe = Path.Combine(cmakeDir, "bin", cmake);

        if (File.Exists(cmakeExe))
        {
            return cmakeExe;
        }

        // Check in Visual Studio
        if (OperatingSystem.IsWindows())
        {
            string? vsCmake = VisualStudioLocator.FindCMake();
            if (vsCmake != null)
            {
                return vsCmake;
            }
        }

        // If doesn't exist, gather it from Github
        Directory.CreateDirectory(cacheDir);
        Console.WriteLine($"[Aether] CMake not found. Automatically downloading {CMakeVersion}...");

        string url = GetDownloadUrl();

        Console.WriteLine($"[Aether] Downloading CMake {CMakeVersion} from {url}");

        using var client = new HttpClient();
        byte[] byteData = await client.GetByteArrayAsync(url);

        // Check if compressed file is .tar.gz or .zip
        bool isTarGz = url.EndsWith(".tar.gz");
        using var mmStream = new MemoryStream(byteData);

        string? cmakePath = isTarGz ? ExtractTarGz(mmStream, cacheDir) : ExtractZip(mmStream, cacheDir);

        if (cmakePath != null) return cmakePath;

        throw new FileNotFoundException("CMake not found.");
    }

    private static string ExtractZip(MemoryStream mmStream, string cacheDir)
    {
        string extractedDir = Path.Combine(cacheDir, $"cmake-{CMakeVersion}");

        using var archive = new ZipArchive(mmStream);
        archive.ExtractToDirectory(extractedDir, overwriteFiles: true);

        // Get the inner directory
        string[] innerDirs = Directory.GetDirectories(extractedDir);
        if (innerDirs.Length == 1)
        {
            string innerDir = innerDirs[0];
            foreach (string item in Directory.GetFileSystemEntries(innerDir))
            {
                string dest = Path.Combine(extractedDir, Path.GetFileName(item));

                if (Directory.Exists(item))
                    Directory.Move(item, dest);
                else
                    File.Move(item, dest);
            }

            Directory.Delete(innerDir);
        }

        string cmakePath = Path.Combine(extractedDir, "bin", "cmake.exe");

        if (!File.Exists(cmakePath))
            throw new FileNotFoundException("CMake executable not found");

        return cmakePath;
    }

    private static string? ExtractTarGz(MemoryStream mmStream, string cacheDir)
    {
        if (OperatingSystem.IsWindows())
            return null;

        string extractedDir = Path.Combine(cacheDir, $"cmake-{CMakeVersion}");

        mmStream.Position = 0;

        using var gzip = new GZipStream(mmStream, CompressionMode.Decompress);
        TarFile.ExtractToDirectory(gzip, extractedDir, overwriteFiles: true);

        string cmakePath = Path.Combine(extractedDir, "bin", "cmake");

        if (!File.Exists(cmakePath))
            throw new FileNotFoundException("Cmake executable not found");

        File.SetUnixFileMode(cmakePath,
                       UnixFileMode.UserRead | UnixFileMode.UserWrite | UnixFileMode.UserExecute |
                       UnixFileMode.GroupRead | UnixFileMode.GroupExecute |
                       UnixFileMode.OtherRead | UnixFileMode.OtherExecute);

        return cmakePath;
    }

    private static string GetDownloadUrl()
    {
        if (OperatingSystem.IsWindows())
            return $"https://github.com/Kitware/CMake/releases/download/v{CMakeVersion}/cmake-{CMakeVersion}-windows-x86_64.zip";
        if (OperatingSystem.IsMacOS())                                       
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
