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
        bool isWindows = preset.Platform.Equals("Windows", StringComparison.OrdinalIgnoreCase);
        bool isMac = preset.Platform.Equals("macOS", StringComparison.OrdinalIgnoreCase)
                  || preset.Platform.Equals("Darwin", StringComparison.OrdinalIgnoreCase);

        var ninja = new StringBuilder();
        ninja.AppendLine("ninja_required_version = 1.5");
        ninja.AppendLine();

        ninja.AppendLine($"compiler = {toolchain.Compiler}");
        ninja.AppendLine($"linker = {toolchain.Linker}");
        ninja.AppendLine();

        string definePrefix = toolchain.Family == ToolchainFamily.MSVC ? "/D" : "-D";
        string globalDefines = string.Join(" ", preset.Defines.Select(d => $"{definePrefix}{d}"));

        if (toolchain.Family == ToolchainFamily.MSVC)
        {
            string opt = isDebug ? "/Od /RTC1 /Zi /MDd" : "/O2 /DNDEBUG /MD";
            string cxx = $"/nologo /std:c++20 /EHsc /W3 {opt}";
            string lnk = isDebug ? "/nologo /DEBUG" : "/nologo";

            ninja.AppendLine("rule compile");
            ninja.AppendLine($"  command = $compiler {cxx} /Fd$out.pdb /FS /showIncludes $defines $includes /c $in /Fo$out");
            ninja.AppendLine("  deps = msvc");
            ninja.AppendLine("  description = [CXX] $in");
            ninja.AppendLine();

            ninja.AppendLine("rule link_dll");
            ninja.AppendLine($"  command = $linker {lnk} /DLL $in /OUT:$out /IMPLIB:$implib");
            ninja.AppendLine("  description = [DLL] $out");
            ninja.AppendLine();

            ninja.AppendLine("rule link_exe");
            ninja.AppendLine($"  command = $linker {lnk} $in $libs /OUT:$out");
            ninja.AppendLine("  description = [EXE] $out");
        }
        else
        {
            string opt = isDebug ? "-O0 -g" : "-O3 -DNDEBUG";
            string cxx = $"-std=c++20 -Wall -fPIC {opt}";
            string lnk = isDebug ? "-g" : "";
            string sharedFlag = isMac ? "-dynamiclib" : "-shared";
            string rpath = isMac ? "-Wl,-rpath,@loader_path" : "-Wl,-rpath,'$$ORIGIN'";

            ninja.AppendLine("rule compile");
            ninja.AppendLine($"  command = $compiler {cxx} -MMD -MF $out.d $defines $includes -c $in -o $out");
            ninja.AppendLine("  depfile = $out.d");
            ninja.AppendLine("  deps = gcc");
            ninja.AppendLine("  description = [CXX] $in");
            ninja.AppendLine();

            ninja.AppendLine("rule link_dll");
            ninja.AppendLine($"  command = $linker {sharedFlag} {lnk} $in -o $out");
            ninja.AppendLine("  description = [SHARED] $out");
            ninja.AppendLine();

            ninja.AppendLine("rule link_exe");
            ninja.AppendLine($"  command = $linker {lnk} {rpath} $in $libs -o $out");
            ninja.AppendLine("  description = [EXE] $out");
        }

        ninja.AppendLine();

        var moduleImportLibs = new List<string>();
        var allDllOutputs = new List<string>();

        foreach (var module in target.ResolvedModules)
        {
            var moduleObjFiles = new List<string>();
            string incPrefix = toolchain.Family == ToolchainFamily.MSVC ? "/I" : "-I";
            string moduleIncludes = string.Join(" ", module.IncludePaths.Select(p => $"{incPrefix}\"{p.Replace("\\", "/")}\""));
            string moduleDefines = $"{globalDefines} {definePrefix}MODULE_{module.Name.ToUpperInvariant()}_EXPORTS";

            foreach (var def in module.Defines)
                moduleDefines += $" {definePrefix}{def}";

            foreach (var src in module.SourceFiles)
            {
                string absoluteSrc = Path.GetFullPath(src);
                string normSrc = ToNinjaPath(absoluteSrc);

                string relSrc = Path.GetRelativePath(workspaceRoot, absoluteSrc)
                                    .Replace("\\", "_")
                                    .Replace("/", "_");

                string objName = Path.ChangeExtension(
                    relSrc,
                    isWindows ? ".obj" : ".o");

                string objFile = ToNinjaPath($"{intermediateDir}/{objName}");

                ninja.AppendLine($"build {objFile}: compile {normSrc}");
                ninja.AppendLine($"  defines = {moduleDefines}");
                ninja.AppendLine($"  includes = {moduleIncludes}");
                moduleObjFiles.Add(objFile);
            }

            string dllFile;
            if (isWindows)
            {
                dllFile = ToNinjaPath(Path.Combine(binDir, $"{module.Name}.dll"));
                string implibFile = ToNinjaPath(Path.Combine(binDir, $"{module.Name}.lib"));
                ninja.AppendLine($"build {dllFile}: link_dll {string.Join(" ", moduleObjFiles)}");
                ninja.AppendLine($"  implib = {implibFile}");
                moduleImportLibs.Add(implibFile);
            }
            else if (isMac)
            {
                dllFile = $"{binDir}/lib{module.Name}.dylib";
                ninja.AppendLine($"build {dllFile}: link_dll {string.Join(" ", moduleObjFiles)}");
                moduleImportLibs.Add(dllFile);
            }
            else
            {
                dllFile = $"{binDir}/lib{module.Name}.so";
                ninja.AppendLine($"build {dllFile}: link_dll {string.Join(" ", moduleObjFiles)}");
                moduleImportLibs.Add($"-L{binDir} -l{module.Name}");
            }

            allDllOutputs.Add(dllFile);
            ninja.AppendLine();
        }

        var targetObjFiles = new List<string>();
        string tIncPrefix = toolchain.Family == ToolchainFamily.MSVC ? "/I" : "-I";
        string targetIncludes = string.Join(" ", target.IncludePaths.Select(p => $"{tIncPrefix}\"{p.Replace("\\", "/")}\""));

        foreach (var mod in target.ResolvedModules)
        {
            string modInc = string.Join(" ", mod.IncludePaths.Select(p => $"{tIncPrefix}\"{p.Replace("\\", "/")}\""));
            if (!string.IsNullOrEmpty(modInc)) targetIncludes += " " + modInc;
        }

        foreach (var src in target.SourceFiles)
        {
            string absoluteSrc = Path.GetFullPath(src);
            string normSrc = ToNinjaPath(absoluteSrc);

            string relSrc = Path.GetRelativePath(
                workspaceRoot,
                absoluteSrc
            ).Replace("\\", "_").Replace("/", "_");

            string objName = Path.ChangeExtension(
                relSrc,
                isWindows ? ".obj" : ".o"
            );

            string objFile = ToNinjaPath(
                Path.Combine(intermediateDir, objName)
            );

            ninja.AppendLine($"build {objFile}: compile {normSrc}");
            ninja.AppendLine($"  defines = {globalDefines}");
            ninja.AppendLine($"  includes = {targetIncludes}");

            targetObjFiles.Add(objFile);
        }

        string exeName = isWindows ? $"{target.Name}.exe" : target.Name;
        string exePath = ToNinjaPath(Path.Combine(binDir, exeName));
        string orderDeps = allDllOutputs.Count > 0 ? " | " + string.Join(" ", allDllOutputs) : "";
        ninja.AppendLine($"build {exePath}: link_exe {string.Join(" ", targetObjFiles)}{orderDeps}");
        if (moduleImportLibs.Count > 0) ninja.AppendLine($"  libs = {string.Join(" ", moduleImportLibs)}");
        ninja.AppendLine();
        ninja.AppendLine($"default {exePath}");

        string ninjaFilePath = Path.Combine(intermediateDir, "build.ninja");
        File.WriteAllText(ninjaFilePath, ninja.ToString());

        /*
         * And yes... 
         * As you can see, we are generating a build.ninja 
         * file based on the presets.
         * 
         * This is really VERBOSE. And as a tradition in this type
         * of files, another gift for you: https://open.spotify.com/track/2aJDlirz6v2a4HREki98cP?si=1cb3ccd87aa44c9f
         * 
         */

        return ninjaFilePath;
    }

    private static string ToNinjaPath(string path)
    {
        return path.Replace("\\", "/")
                   .Replace(" ", "$ ")
                   .Replace(":", "$:");
    }

    private static string GetRelativeNinjaPath(
        string workspaceRoot,
        string path)
    {
        string relative = Path.GetRelativePath(workspaceRoot, path);
        return ToNinjaPath(relative);
    }
}
