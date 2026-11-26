using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using Aether.Rules;

public class ImguiExternal : ExternalRules
{
    public ImguiExternal(string workspaceRoot)
    {
        Name = "imgui";
        Repo = "https://github.com/ocornut/imgui";
        Branch = "docking";
        BuildWithCMake = false;
        IncludePaths.Add(".");
        IncludePaths.Add("backends");
        SourceFiles.AddRange(new[]
        {
            "imgui.cpp",
            "imgui_draw.cpp",
            "imgui_tables.cpp",
            "imgui_widgets.cpp",
            "backends/imgui_impl_glfw.cpp",
            "backends/imgui_impl_vulkan.cpp"
        });

        if (OperatingSystem.IsWindows())
        {
            SourceFiles.Add("backends/imgui_impl_win32.cpp");
        }
    }
}

public class AssimpExternal : ExternalRules
{
    public AssimpExternal()
    {
        Name = "Assimp";
        Repo = "https://github.com/assimp/assimp";
        BuildWithCMake = true;
        CMakeArguments = "-DBUILD_SHARED_LIBS=OFF -DASSIMP_BUILD_TESTS=OFF -DASSIMP_BUILD_ASSIMP_TOOLS=OFF -DASSIMP_INSTALL=OFF -DASSIMP_WARNINGS_AS_ERRORS=OFF";
    }
}

public class SpdlogExternal : ExternalRules
{
    public SpdlogExternal(string workspaceRoot)
    {
        Name = "spdlog";
        Repo = "https://github.com/gabime/spdlog";
        Branch = "v1.15.3";
        BuildWithCMake = true;
        Defines.Add("SPDLOG_FMT_EXTERNAL");
        string fmtBuildDir = Path.Combine(workspaceRoot, ".cache", "fmt", "build").Replace("\\", "/");
        CMakeArguments = $"-DSPDLOG_FMT_EXTERNAL=ON -DSPDLOG_BUILD_EXAMPLE=OFF -DSPDLOG_BUILD_TESTS=OFF -DSPDLOG_INSTALL=OFF -DFMT_DIR=\"{fmtBuildDir}\" -DCMAKE_PREFIX_PATH=\"{fmtBuildDir}\"";
    }
}

public class FmtExternal : ExternalRules
{
    public FmtExternal()
    {
        Name = "fmt";
        Repo = "https://github.com/fmtlib/fmt";
        Branch = "11.2.0";
        BuildWithCMake = true;
        CMakeArguments = "-DFMT_TEST=OFF -DFMT_DOC=OFF -DFMT_INSTALL=OFF";
    }
}

public class GlfwExternal : ExternalRules
{
    public GlfwExternal()
    {
        Name = "glfw";
        Repo = "https://github.com/glfw/glfw";
        BuildWithCMake = true;
        CMakeArguments = "-DGLFW_BUILD_EXAMPLES=OFF -DGLFW_BUILD_TESTS=OFF -DGLFW_BUILD_DOCS=OFF -DGLFW_INSTALL=OFF";
    }
}

public class XxHashExternal : ExternalRules
{
    public XxHashExternal()
    {
        Name = "xxhash";
        Repo = "https://github.com/Cyan4973/xxHash";
        BuildWithCMake = false;
        IncludePaths.Add(".");
        SourceFiles.Add("xxhash.c");
    }
}

public class NfdExternal : ExternalRules
{
    public NfdExternal()
    {
        Name = "NFD";
        Repo = "https://github.com/btzy/nativefiledialog-extended";
        BuildWithCMake = true;
        CMakeArguments = "-DNFD_BUILD_TESTS=OFF";
        IncludePaths.Add("src/include");
    }
}

public class NanoSvgExternal : ExternalRules
{
    public NanoSvgExternal()
    {
        Name = "nanosvg";
        Repo = "https://github.com/memononen/nanosvg";
        BuildWithCMake = false;
    }
}

public class StbExternal : ExternalRules
{
    public StbExternal()
    {
        Name = "stb";
        Repo = "https://github.com/nothings/stb";
        BuildWithCMake = false;
    }
}

public class MinizExternal : ExternalRules
{
    public MinizExternal()
    {
        Name = "miniz";
        Repo = "https://github.com/richgel999/miniz";
        BuildWithCMake = false;
        SourceFiles.Add("miniz.c");
    }
}

public class TinyExrExternal : ExternalRules
{
    public TinyExrExternal()
    {
        Name = "tinyexr";
        Repo = "https://github.com/syoyo/tinyexr";
        BuildWithCMake = false;
        SourceFiles.Add("deps/miniz/miniz.c");
    }
}

public class NlohmannExternal : ExternalRules
{
    public NlohmannExternal()
    {
        Name = "nlohmann";
        Repo = "https://github.com/nlohmann/json";
        BuildWithCMake = false;
        IncludePaths.Add("single_include");
    }
}

public class GlmExternal : ExternalRules
{
    public GlmExternal()
    {
        Name = "glm";
        Repo = "https://github.com/g-truc/glm";
        BuildWithCMake = false;
        IncludePaths.Add(".");
    }
}

public class AetherionTarget : TargetRules
{
    private static void CopyDirectory(string sourceDir, string destinationDir)
    {
        Directory.CreateDirectory(destinationDir);

        foreach (string file in Directory.GetFiles(sourceDir, "*", SearchOption.AllDirectories))
        {
            string relativePath = Path.GetRelativePath(sourceDir, file);
            string destinationFile = Path.Combine(destinationDir, relativePath);

            Directory.CreateDirectory(Path.GetDirectoryName(destinationFile)!);
            if (!File.Exists(destinationFile) ||
                File.GetLastWriteTimeUtc(destinationFile) < File.GetLastWriteTimeUtc(file))
            {
                File.Copy(file, destinationFile, overwrite: true);
            }
        }
    }

    private void AddUniqueIncludePath(string path)
    {
        if (Directory.Exists(path) && !IncludePaths.Contains(path))
            IncludePaths.Add(path);
    }

    private void AddUniqueLibraryPath(string path)
    {
        if (Directory.Exists(path) && !LibraryPaths.Contains(path))
            LibraryPaths.Add(path);
    }

    private void AddPrefixIncludeAndLib(string prefix)
    {
        if (string.IsNullOrWhiteSpace(prefix) || !Directory.Exists(prefix))
            return;

        AddUniqueIncludePath(Path.Combine(prefix, "include"));
        AddUniqueIncludePath(Path.Combine(prefix, "Include"));
        AddUniqueLibraryPath(Path.Combine(prefix, "lib"));
        AddUniqueLibraryPath(Path.Combine(prefix, "Lib"));
    }

    private bool HasLibraryFile(string fileName)
    {
        foreach (string dir in LibraryPaths)
        {
            if (File.Exists(Path.Combine(dir, fileName)))
                return true;
        }
        return false;
    }

    private static List<string> QueryPkgConfigLibs(string packages)
    {
        var result = new List<string>();
        try
        {
            var psi = new ProcessStartInfo
            {
                FileName = "pkg-config",
                Arguments = $"--libs {packages}",
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false
            };

            using var process = Process.Start(psi);
            if (process != null)
            {
                string output = process.StandardOutput.ReadToEnd();
                process.WaitForExit();
                if (process.ExitCode == 0 && !string.IsNullOrWhiteSpace(output))
                {
                    foreach (string token in output.Split(new[] { ' ', '\t', '\r', '\n' }, StringSplitOptions.RemoveEmptyEntries))
                    {
                        result.Add(token);
                    }
                }
            }
        }
        catch
        {
            // Ignore pkg-config unavailability
        }
        return result;
    }

    public AetherionTarget(string workspaceRoot) : this(workspaceRoot, "Debug")
    {
    }

    public AetherionTarget(string workspaceRoot, string configuration)
        : this(workspaceRoot, configuration, OperatingSystem.IsMacOS() ? "macOS" : (OperatingSystem.IsLinux() ? "Linux" : "Windows"))
    {
    }

    public AetherionTarget(string workspaceRoot, string configuration, string platform)
    {
        Name = "Aetherion";
        Configuration = configuration;
        Platform = platform;
        ENABLE_JUMBO = true;

        bool isDebug = configuration.Equals("Debug", StringComparison.OrdinalIgnoreCase);
        bool isWindows = platform.Equals("Windows", StringComparison.OrdinalIgnoreCase);
        bool isMac = platform.Equals("macOS", StringComparison.OrdinalIgnoreCase)
                  || platform.Equals("Darwin", StringComparison.OrdinalIgnoreCase);
        bool isLinux = !isWindows && !isMac;

        string engineDir = Path.Combine(workspaceRoot, "Engine");
        string sharedDir = Path.Combine(workspaceRoot, "Shared");
        string shadersTargetBinDir = Path.Combine(workspaceRoot, "Bin", Name, configuration);
        string shadersDir = Path.Combine(engineDir, "src", "shaders");

        if (Directory.Exists(shadersDir))
        {
            CopyDirectory(shadersDir, Path.Combine(shadersTargetBinDir, "shaders"));
        }

        IncludePaths.Add(Path.Combine(engineDir, "src", "public"));
        IncludePaths.Add(Path.Combine(sharedDir, "src", "public"));
        IncludePaths.Add(Path.Combine(workspaceRoot, ".cache"));

        Defines.Add("GIT_COMMIT=\\\"dev\\\"");

        if (isWindows)
        {
            Defines.Add("NOMINMAX");
            Defines.Add("EXCEPTION_HANDLER_PATH=\\\"ExceptionHandler.exe\\\"");

            string? vulkanSdk = Environment.GetEnvironmentVariable("VULKAN_SDK");
            if (!string.IsNullOrEmpty(vulkanSdk))
            {
                AddUniqueIncludePath(Path.Combine(vulkanSdk, "Include"));
                AddUniqueLibraryPath(Path.Combine(vulkanSdk, "Lib"));
                Libraries.Add("vulkan-1.lib");
                if (isDebug)
                {
                    Libraries.Add("shaderc_combinedd.lib");
                    Libraries.Add("spirv-cross-cd.lib");
                    Libraries.Add("spirv-cross-cored.lib");
                    Libraries.Add("spirv-cross-glsld.lib");
                    Libraries.Add("spirv-cross-hlsld.lib");
                    Libraries.Add("spirv-cross-msld.lib");
                }
                else
                {
                    Libraries.Add("shaderc_combined.lib");
                    Libraries.Add("spirv-cross-c.lib");
                    Libraries.Add("spirv-cross-core.lib");
                    Libraries.Add("spirv-cross-glsl.lib");
                    Libraries.Add("spirv-cross-hlsl.lib");
                    Libraries.Add("spirv-cross-msl.lib");
                }
            }

            Libraries.Add("user32.lib");
            Libraries.Add("gdi32.lib");
            Libraries.Add("shell32.lib");
            Libraries.Add("ole32.lib");
            Libraries.Add("ws2_32.lib");
        }
        else if (isMac)
        {
            Defines.Add("EXCEPTION_HANDLER_PATH=\\\"ExceptionHandler\\\"");

            AddPrefixIncludeAndLib("/opt/homebrew/opt/shaderc");
            AddPrefixIncludeAndLib("/opt/homebrew/opt/spirv-cross");
            AddPrefixIncludeAndLib("/opt/homebrew/opt/vulkan-headers");
            AddPrefixIncludeAndLib("/opt/homebrew/opt/vulkan-loader");
            AddPrefixIncludeAndLib("/opt/homebrew/opt/molten-vk");

            string? vulkanSdk = Environment.GetEnvironmentVariable("VULKAN_SDK");
            if (!string.IsNullOrEmpty(vulkanSdk))
                AddPrefixIncludeAndLib(vulkanSdk);

            AddPrefixIncludeAndLib("/usr/local");
            AddPrefixIncludeAndLib("/opt/homebrew");

            Libraries.Add("vulkan");
            Libraries.Add(HasLibraryFile("libshaderc_combined.a")
                ? "shaderc_combined"
                : (HasLibraryFile("libshaderc_shared.dylib") ? "shaderc_shared" : "shaderc_combined"));
            Libraries.Add("spirv-cross-c");
            Libraries.Add("spirv-cross-glsl");
            Libraries.Add("spirv-cross-hlsl");
            Libraries.Add("spirv-cross-msl");
            Libraries.Add("spirv-cross-core");
            Libraries.Add("z");

            Libraries.Add("-framework Cocoa");
            Libraries.Add("-framework IOKit");
            Libraries.Add("-framework CoreVideo");
            Libraries.Add("-framework CoreFoundation");
            Libraries.Add("-framework AppKit");
            Libraries.Add("-framework UniformTypeIdentifiers");
            Libraries.Add("-framework QuartzCore");
        }
        else if (isLinux)
        {
            Defines.Add("EXCEPTION_HANDLER_PATH=\\\"ExceptionHandler\\\"");

            string? vulkanSdk = Environment.GetEnvironmentVariable("VULKAN_SDK");
            if (!string.IsNullOrEmpty(vulkanSdk))
                AddPrefixIncludeAndLib(vulkanSdk);

            AddPrefixIncludeAndLib("/usr/local");
            AddPrefixIncludeAndLib("/usr");
            AddUniqueLibraryPath("/usr/lib/x86_64-linux-gnu");
            AddUniqueLibraryPath("/usr/lib/aarch64-linux-gnu");

            Libraries.Add("vulkan");
            Libraries.Add(HasLibraryFile("libshaderc_combined.a")
                ? "shaderc_combined"
                : (HasLibraryFile("libshaderc_shared.so") ? "shaderc_shared" : "shaderc_combined"));
            Libraries.Add("spirv-cross-c");
            Libraries.Add("spirv-cross-glsl");
            Libraries.Add("spirv-cross-hlsl");
            Libraries.Add("spirv-cross-msl");
            Libraries.Add("spirv-cross-core");

            var nfdLibs = QueryPkgConfigLibs("gtk+-3.0 dbus-1");
            if (nfdLibs.Count > 0)
            {
                Libraries.AddRange(nfdLibs);
            }
            else
            {
                Libraries.Add("gtk-3");
                Libraries.Add("gdk-3");
                Libraries.Add("gobject-2.0");
                Libraries.Add("glib-2.0");
                Libraries.Add("dbus-1");
            }

            Libraries.Add("dl");
            Libraries.Add("pthread");
            Libraries.Add("m");
            Libraries.Add("z");
        }

        ImguiExternal imgui = new ImguiExternal(workspaceRoot);
        Externals.Add(imgui);
        Externals.Add(new FmtExternal());
        Externals.Add(new SpdlogExternal(workspaceRoot));
        Externals.Add(new GlfwExternal());
        Externals.Add(new XxHashExternal());
        Externals.Add(new NfdExternal());
        Externals.Add(new NanoSvgExternal());
        Externals.Add(new StbExternal());
        Externals.Add(new TinyExrExternal());
        Externals.Add(new NlohmannExternal());
        Externals.Add(new GlmExternal());

        // Assimp in .cache
        Externals.Add(new AssimpExternal());

        string srcDir = Path.Combine(engineDir, "src");
        if (Directory.Exists(srcDir))
        {
            SourceFiles.AddRange(Directory.GetFiles(srcDir, "*.cpp", SearchOption.AllDirectories));
        }

    }
}
