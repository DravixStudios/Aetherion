namespace Aether.Toolchain;

public static class ArtifactCollector
{
    private sealed record PlatformProfile(string StaticLibraryPattern, string SharedLibraryPattern, string RuntimeLibraryPattern);

    public static List<string> Collect(string modulePath, string targetPath, string config = "Release")
    {
        var collectedStaticLibs = new List<string>();

        if (!Directory.Exists(modulePath))
            return collectedStaticLibs;

        string libDirectory = Path.Combine(modulePath, "static-lib");
        string dllDirectory = targetPath;

        Directory.CreateDirectory(libDirectory);
        if (!string.IsNullOrEmpty(dllDirectory))
            Directory.CreateDirectory(dllDirectory);

        string buildDir = Path.Combine(modulePath, "build");
        string configPath = Path.Combine(buildDir, config);
        string searchPath = Directory.Exists(configPath) ? configPath : (Directory.Exists(buildDir) ? buildDir : modulePath);

        PlatformProfile profile = GetPlatformProfile();

        // Collect all static, shared and runtime libraries
        collectedStaticLibs.AddRange(CollectFiles(searchPath, profile.StaticLibraryPattern, libDirectory));
        if (!string.IsNullOrEmpty(profile.SharedLibraryPattern))
            CollectFiles(searchPath, profile.SharedLibraryPattern, libDirectory);
        if (!string.IsNullOrEmpty(profile.RuntimeLibraryPattern) && !string.IsNullOrEmpty(dllDirectory))
            CollectFiles(searchPath, profile.RuntimeLibraryPattern, dllDirectory);

        return collectedStaticLibs;
    }

    private static List<string> CollectFiles(string srcDir, string pattern, string dstDir)
    {
        var collected = new List<string>();
        if (string.IsNullOrEmpty(pattern) || !Directory.Exists(srcDir))
            return collected;

        string excludePath = Path.GetFullPath(dstDir);

        var files = Directory.GetFiles(srcDir, pattern, SearchOption.AllDirectories)
            .Where(f => !Path.GetFullPath(f).StartsWith(excludePath + Path.DirectorySeparatorChar, StringComparison.OrdinalIgnoreCase)).ToList();

        foreach (string file in files)
        {
            string dest = Path.Combine(dstDir, Path.GetFileName(file));
            CopyArtifact(file, dest);
            collected.Add(dest);
        }

        return collected;
    }

    private static PlatformProfile GetPlatformProfile()
    {
        if (OperatingSystem.IsWindows()) return new PlatformProfile("*.lib", "", "*.dll");
        if (OperatingSystem.IsLinux()) return new PlatformProfile("*.a", "*.so*", "*.so*");
        if (OperatingSystem.IsMacOS()) return new PlatformProfile("*.a", "*.dylib", "*.dylib");
        
        throw new PlatformNotSupportedException("Unsupported platform");
    }

    private static void CopyArtifact(string src, string dest)
    {
        var dir = Path.GetDirectoryName(dest);
        if (dir != null)
            Directory.CreateDirectory(dir);
        
        for (int attempt = 0; attempt < 10; ++attempt)
        {
            try
            {
                File.Copy(src, dest, overwrite: true);
                return;
            }
            catch (IOException) when (attempt < 9)
            {
                Console.WriteLine($"[Aether] {src} is busy, waiting...");
                Thread.Sleep(500);
            }
        }

        throw new IOException($"Couldn't copy artifact: {dest}");
    }
}
