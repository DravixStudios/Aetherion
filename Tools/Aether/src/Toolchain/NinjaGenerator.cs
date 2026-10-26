using System.Text;
using Aether.Presets;
using Aether.Rules;

namespace Aether.Toolchain;

public static class NinjaGenerator
{
    public static string Generate(string workspaceRoot, TargetRules target, Toolchain toolchain, Preset preset)
    {
        string binDir = Path.Combine(workspaceRoot, "Bin", target.Name, preset.Configuration).Replace("\\", "/");
        Directory.CreateDirectory(binDir);

        string intermediateDir = Path.Combine(workspaceRoot, ".cache", target.Name, preset.Configuration).Replace("\\", "/");
        Directory.CreateDirectory(intermediateDir);

        bool isDebug = preset.Configuration.Equals("Debug", StringComparison.OrdinalIgnoreCase);

        var ninja = new StringBuilder();
        ninja.AppendLine("ninja_required_version = 1.5");
        ninja.AppendLine();

        ninja.AppendLine($"compiler = {toolchain.Compiler}");
        ninja.AppendLine($"linker = {toolchain.Linker}");
        ninja.AppendLine();

        string cxxFlags;
        string linkFlags;

        // MSVC Flags
        if (toolchain.Family == ToolchainFamily.MSVC)
        {
            string optFlags = isDebug ? "/Od /RTC1 /Zi /MDd" : "/O2 /DNDEBUG /MD";
            cxxFlags = $"/nologo /std:c++20 /EHsc /W3 {optFlags}";
            linkFlags = isDebug ? "/nologo /DEBUG" : "/nologo";

            ninja.AppendLine("rule compile");
            ninja.AppendLine($"  command = $compiler {cxxFlags} /showIncludes $defines $includes /c $in /Fo$out");
            ninja.AppendLine("  deps = msvc");
            ninja.AppendLine("  description = [CXX] $in");
            ninja.AppendLine();

            ninja.AppendLine("rule link");
            ninja.AppendLine($"  command = $linker {linkFlags} $in /OUT:$out");
            ninja.AppendLine("  description = [LINK] $out");
        }
        // Clang / GCC Flags
        else
        {
            string optFlags = isDebug ? "-O0 -g" : "-O3 -DNDEBUG";
            cxxFlags = $"-std=c++20 -Wall {optFlags}";
            linkFlags = isDebug ? "-g" : "";

            ninja.AppendLine("rule compile");
            ninja.AppendLine($"  command = $compiler {cxxFlags} -MMD -MF $out.d $defines $includes -c $in -o $out");
            ninja.AppendLine("  depfile = $out.d");
            ninja.AppendLine("  deps = gcc");
            ninja.AppendLine("  description = [CXX] $in");
            ninja.AppendLine();

            ninja.AppendLine("rule link");
            ninja.AppendLine($"  command = $linker {linkFlags} $in -o $out");
            ninja.AppendLine("  description = [LINK] $out");
        }

        ninja.AppendLine();

        string definePrefix = toolchain.Family == ToolchainFamily.MSVC ? "/D" : "-D";
        string globalDefines = string.Join(" ", preset.Defines.Select(d => $"{definePrefix}{d}"));

        var objectFiles = new List<string>();

        foreach (var module in target.Modules)
        {
            string incPrefix = toolchain.Family == ToolchainFamily.MSVC ? "/I" : "-I";
            string includeFlags = string.Join(" ", module.IncludePaths.Select(p => $"{incPrefix}\"{p.Replace("\\", "/")}\""));

            foreach (var src in module.SourceFiles)
            {
                string normalizedSrc = Path.GetFullPath(src).Replace("\\", "/");

                string relativeSrc = Path.GetRelativePath(workspaceRoot, src).Replace("\\", "_").Replace("/", "_");
                string objFileName = Path.ChangeExtension(relativeSrc, toolchain.Family == ToolchainFamily.MSVC ? ".obj" : ".o");
                string objFile = $"{intermediateDir}/{objFileName}";

                ninja.AppendLine($"build {objFile}: compile {normalizedSrc}");
                ninja.AppendLine($"  defines = {globalDefines}");
                ninja.AppendLine($"  includes = {includeFlags}");

                objectFiles.Add(objFile);
            }
        }

        string exeExtension = OperatingSystem.IsWindows() ? ".exe" : "";
        string finalExeName = Path.ChangeExtension(target.OutputExecutable, exeExtension);
        string finalExePath = $"{binDir}/{finalExeName}";

        ninja.AppendLine();
        ninja.AppendLine($"build {finalExePath}: link {string.Join(" ", objectFiles)}");
        ninja.AppendLine();
        ninja.AppendLine($"default {finalExePath}");

        string ninjaFilePath = Path.Combine(intermediateDir, "build.ninja");
        File.WriteAllText(ninjaFilePath, ninja.ToString());

        return ninjaFilePath;
    }
}
