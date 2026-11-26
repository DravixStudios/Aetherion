using System;
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
        CMakeArguments = $"-DSPDLOG_FMT_EXTERNAL=ON -DFMT_DIR=\"{fmtBuildDir}\" -DCMAKE_PREFIX_PATH=\"{fmtBuildDir}\"";
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
    }
}

public class GlfwExternal : ExternalRules
{
    public GlfwExternal()
    {
        Name = "glfw";
        Repo = "https://github.com/glfw/glfw";
        BuildWithCMake = true;
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

    public AetherionTarget(string workspaceRoot) : this(workspaceRoot, "Debug")
    {
    }

    public AetherionTarget(string workspaceRoot, string configuration)
    {
        Name = "Aetherion";
        Configuration = configuration;

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

        Defines.Add("NOMINMAX");
        Defines.Add("EXCEPTION_HANDLER_PATH=\\\"ExceptionHandler.exe\\\"");
        Defines.Add("GIT_COMMIT=\\\"dev\\\"");

        bool isDebug = configuration.Equals("Debug", StringComparison.OrdinalIgnoreCase);

        // Vulkan SDK
        string? vulkanSdk = Environment.GetEnvironmentVariable("VULKAN_SDK");
        if (!string.IsNullOrEmpty(vulkanSdk))
        {
            IncludePaths.Add(Path.Combine(vulkanSdk, "Include"));
            string vulkanLib = Path.Combine(vulkanSdk, "Lib");
            LibraryPaths.Add(vulkanLib);
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

        // Standard Windows system libraries
        Libraries.Add("user32.lib");
        Libraries.Add("gdi32.lib");
        Libraries.Add("shell32.lib");
        Libraries.Add("ole32.lib");
        Libraries.Add("ws2_32.lib");

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
