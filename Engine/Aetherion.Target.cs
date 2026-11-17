using System;
using System.IO;
using Aether.Rules;

public class ImguiExternal : ExternalRules
{
    public ImguiExternal(string workspaceRoot)
    {
        Name = "ImGui";
        Repo = "https://github.com/ocornut/imgui";
        BuildWithCMake = false;

        string imguiPath = Path.Combine(workspaceRoot, "Engine", "include", "imgui");
        if (Directory.Exists(imguiPath))
        {
            IncludePaths.Add(imguiPath);
            IncludePaths.Add(Path.Combine(imguiPath, "backends"));

            string[] sources = new[]
            {
                "imgui.cpp",
                "imgui_draw.cpp",
                "imgui_tables.cpp",
                "imgui_widgets.cpp",
                "backends/imgui_impl_glfw.cpp",
                "backends/imgui_impl_vulkan.cpp",
                "backends/imgui_impl_win32.cpp"
            };

            foreach (var s in sources)
            {
                string full = Path.Combine(imguiPath, s);
                if (File.Exists(full))
                {
                    SourceFiles.Add(full);
                }
            }
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
        IncludePaths.Add(Path.Combine(engineDir, "include"));
        IncludePaths.Add(Path.Combine(sharedDir, "src", "public"));

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

        // vcpkg dependencies
        string vcpkgDir = Path.Combine(workspaceRoot, "out", "build", "x64-debug-spdlog", "vcpkg_installed", "x64-windows");
        if (Directory.Exists(vcpkgDir))
        {
            IncludePaths.Add(Path.Combine(vcpkgDir, "include"));
            string vcpkgLibDir = isDebug ? Path.Combine(vcpkgDir, "debug", "lib") : Path.Combine(vcpkgDir, "lib");
            if (Directory.Exists(vcpkgLibDir))
            {
                LibraryPaths.Add(vcpkgLibDir);
                Libraries.Add("glfw3dll.lib");
                Libraries.Add(isDebug ? "spdlogd.lib" : "spdlog.lib");
                Libraries.Add(isDebug ? "fmtd.lib" : "fmt.lib");
                Libraries.Add("xxhash.lib");
                Libraries.Add("nfd.lib");
                if (isDebug && File.Exists(Path.Combine(vcpkgLibDir, "assimp-vc145-mtd.lib")))
                {
                    Libraries.Add("assimp-vc145-mtd.lib");
                }
            }

            // Copy runtime DLLs to target bin
            string vcpkgBinDir = isDebug ? Path.Combine(vcpkgDir, "debug", "bin") : Path.Combine(vcpkgDir, "bin");
            string targetBinDir = Path.Combine(workspaceRoot, "Bin", Name, configuration);
            if (Directory.Exists(vcpkgBinDir))
            {
                Directory.CreateDirectory(targetBinDir);
                foreach (var dll in Directory.GetFiles(vcpkgBinDir, "*.dll"))
                {
                    string dest = Path.Combine(targetBinDir, Path.GetFileName(dll));
                    if (!File.Exists(dest) || File.GetLastWriteTimeUtc(dest) < File.GetLastWriteTimeUtc(dll))
                    {
                        try { File.Copy(dll, dest, overwrite: true); } catch { }
                    }
                }
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

        // Assimp in .cache (only if not already linking from vcpkg)
        if (!isDebug)
        {
            AssimpExternal assimp = new AssimpExternal();
            Externals.Add(assimp);
        }

        string srcDir = Path.Combine(engineDir, "src");
        if (Directory.Exists(srcDir))
        {
            SourceFiles.AddRange(Directory.GetFiles(srcDir, "*.cpp", SearchOption.AllDirectories));
        }

        string minizC = Path.Combine(engineDir, "include", "miniz", "miniz.c");
        if (File.Exists(minizC))
        {
            SourceFiles.Add(minizC);
        }
    }
}
