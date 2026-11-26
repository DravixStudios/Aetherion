using System.Text;
using Aether.Presets;
using Aether.Rules;

namespace Aether.Toolchain;

public static class NinjaGenerator
{
    public static string Generate(string workspaceRoot, TargetRules target, Toolchain toolchain, Preset preset, string targetBin)
    {
        string binDir = !string.IsNullOrEmpty(targetBin)
            ? targetBin.Replace("\\", "/")
            : Path.Combine(workspaceRoot, "Bin", target.Name, preset.Configuration).Replace("\\", "/");
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
        ninja.AppendLine($"c_compiler = {toolchain.CCompiler}");
        ninja.AppendLine($"linker = {toolchain.Linker}");
        ninja.AppendLine();

        string definePrefix = toolchain.Family == ToolchainFamily.MSVC ? "/D" : "-D";
        string globalDefines = string.Join(" ", preset.Defines.Select(d => $"{definePrefix}{d}"));

        if (toolchain.Family == ToolchainFamily.MSVC)
        {
            string opt = isDebug ? "/Od /RTC1 /Z7 /MDd" : "/O2 /DNDEBUG /MD";
            string cxx = $"/nologo /std:c++20 /EHsc /utf-8 /W3 {opt}";
            string cc = $"/nologo /utf-8 /W3 {opt}";
            string lnk = isDebug ? "/nologo /DEBUG" : "/nologo";

            ninja.AppendLine("rule compile");
            ninja.AppendLine($"  command = $compiler {cxx} /showIncludes $defines $includes /c $in /Fo$out");
            ninja.AppendLine("  deps = msvc");
            ninja.AppendLine("  description = [CXX] $in");
            ninja.AppendLine();

            ninja.AppendLine("rule compile_c");
            ninja.AppendLine($"  command = $c_compiler {cc} /showIncludes $defines $includes /c $in /Fo$out");
            ninja.AppendLine("  deps = msvc");
            ninja.AppendLine("  description = [CC] $in");
            ninja.AppendLine();

            ninja.AppendLine("rule link_dll");
            ninja.AppendLine($"  command = $linker {lnk} /DLL $in $libpaths $libs /OUT:$out /IMPLIB:$implib");
            ninja.AppendLine("  description = [DLL] $out");
            ninja.AppendLine();

            ninja.AppendLine("rule link_exe");
            ninja.AppendLine($"  command = $linker {lnk} $in $libpaths $libs /OUT:$out");
            ninja.AppendLine("  description = [EXE] $out");
        }
        else
        {
            string opt = isDebug ? "-O0 -g" : "-O3 -DNDEBUG";
            string cxx = $"-std=c++20 -Wall -fPIC {opt}";
            string cc = $"-Wall -fPIC {opt}";
            string lnk = isDebug ? "-g" : "";
            string sharedFlag = isMac ? "-dynamiclib" : "-shared";
            var rpathFlags = new List<string>
            {
                isMac ? "-Wl,-rpath,@loader_path" : "-Wl,-rpath,'$$ORIGIN'"
            };
            if (isMac)
            {
                foreach (var libPath in target.LibraryPaths.Distinct(StringComparer.Ordinal))
                {
                    rpathFlags.Add($"-Wl,-rpath,\"{libPath.Replace("\\", "/")}\"");
                }
            }
            string rpath = string.Join(" ", rpathFlags);

            ninja.AppendLine("rule compile");
            ninja.AppendLine($"  command = $compiler {cxx} -MMD -MF $out.d $defines $includes -c $in -o $out");
            ninja.AppendLine("  depfile = $out.d");
            ninja.AppendLine("  deps = gcc");
            ninja.AppendLine("  description = [CXX] $in");
            ninja.AppendLine();

            ninja.AppendLine("rule compile_c");
            ninja.AppendLine($"  command = $c_compiler {cc} -MMD -MF $out.d $defines $includes -c $in -o $out");
            ninja.AppendLine("  depfile = $out.d");
            ninja.AppendLine("  deps = gcc");
            ninja.AppendLine("  description = [CC] $in");
            ninja.AppendLine();

            ninja.AppendLine("rule link_dll");
            ninja.AppendLine($"  command = $linker {sharedFlag} {lnk} $in $libpaths $libs -o $out");
            ninja.AppendLine("  description = [SHARED] $out");
            ninja.AppendLine();

            ninja.AppendLine("rule link_exe");
            ninja.AppendLine($"  command = $linker {lnk} {rpath} $in $libpaths $libs -o $out");
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

                bool isC = absoluteSrc.EndsWith(".c", StringComparison.OrdinalIgnoreCase);
                string ruleName = isC ? "compile_c" : "compile";

                ninja.AppendLine($"build {objFile}: {ruleName} {normSrc}");
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
                string rawDll = Path.Combine(binDir, $"lib{module.Name}.dylib");
                dllFile = ToNinjaPath(rawDll);
                ninja.AppendLine($"build {dllFile}: link_dll {string.Join(" ", moduleObjFiles)}");
                moduleImportLibs.Add($"\"{rawDll.Replace("\\", "/")}\"");
            }
            else
            {
                string rawDll = Path.Combine(binDir, $"lib{module.Name}.so");
                dllFile = ToNinjaPath(rawDll);
                ninja.AppendLine($"build {dllFile}: link_dll {string.Join(" ", moduleObjFiles)}");
                moduleImportLibs.Add($"-L\"{binDir}\" -l{module.Name}");
            }

            allDllOutputs.Add(dllFile);
            ninja.AppendLine();
        }

        var targetObjFiles = new List<string>();
        string tIncPrefix = toolchain.Family == ToolchainFamily.MSVC ? "/I" : "-I";
        string normRoot = Path.GetFullPath(workspaceRoot).Replace("\\", "/").TrimEnd('/') + "/";
        var orderedIncludes = target.IncludePaths
            .OrderBy(p => Path.GetFullPath(p).Replace("\\", "/").StartsWith(normRoot, StringComparison.OrdinalIgnoreCase) ? 0 : 1)
            .ToList();
        string targetIncludes = string.Join(" ", orderedIncludes.Select(p => $"{tIncPrefix}\"{p.Replace("\\", "/")}\""));

        foreach (var mod in target.ResolvedModules)
        {
            string modInc = string.Join(" ", mod.IncludePaths.Select(p => $"{tIncPrefix}\"{p.Replace("\\", "/")}\""));
            if (!string.IsNullOrEmpty(modInc)) targetIncludes += " " + modInc;
        }

        string targetDefines = globalDefines;
        foreach (var def in target.Defines)
            targetDefines += $" {definePrefix}{def}";

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

            bool isC = absoluteSrc.EndsWith(".c", StringComparison.OrdinalIgnoreCase);
            string ruleName = isC ? "compile_c" : "compile";

            ninja.AppendLine($"build {objFile}: {ruleName} {normSrc}");
            ninja.AppendLine($"  defines = {targetDefines}");
            ninja.AppendLine($"  includes = {targetIncludes}");

            targetObjFiles.Add(objFile);
        }

        string exeName = !string.IsNullOrEmpty(target.OutputExecutable)
            ? target.OutputExecutable
            : (isWindows ? $"{target.Name}.exe" : target.Name);
        if (isWindows && !exeName.EndsWith(".exe", StringComparison.OrdinalIgnoreCase))
            exeName += ".exe";

        string exePath = ToNinjaPath(Path.Combine(binDir, exeName));
        string orderDeps = allDllOutputs.Count > 0 ? " | " + string.Join(" ", allDllOutputs) : "";

        ninja.AppendLine($"build {exePath}: link_exe {string.Join(" ", targetObjFiles)}{orderDeps}");

        var allLibs = new List<string>();
        if (toolchain.Family == ToolchainFamily.MSVC)
        {
            allLibs.AddRange(target.Libraries);
            allLibs.AddRange(moduleImportLibs);
        }
        else
        {
            var formattedLibs = target.Libraries
                .Select(FormatClangGccLibrary)
                .Concat(moduleImportLibs)
                .ToList();

            if (!isMac && formattedLibs.Count > 0)
            {
                allLibs.Add("-Wl,--start-group");
                allLibs.AddRange(formattedLibs);
                allLibs.Add("-Wl,--end-group");
            }
            else
            {
                allLibs.AddRange(formattedLibs);
            }
        }

        if (allLibs.Count > 0)
        {
            ninja.AppendLine($"  libs = {string.Join(" ", allLibs)}");
        }

        if (target.LibraryPaths.Count > 0)
        {
            string libPathPrefix = toolchain.Family == ToolchainFamily.MSVC ? "/LIBPATH:" : "-L";
            string libPathsStr = string.Join(" ", target.LibraryPaths.Select(p => $"{libPathPrefix}\"{p.Replace("\\", "/")}\""));
            ninja.AppendLine($"  libpaths = {libPathsStr}");
        }

        ninja.AppendLine();
        ninja.AppendLine($"default {exePath}");

        string ninjaFilePath = Path.Combine(intermediateDir, "build.ninja");
        File.WriteAllText(ninjaFilePath, ninja.ToString());

        return ninjaFilePath;
    }

    private static string FormatClangGccLibrary(string lib)
    {
        if (lib.StartsWith("-", StringComparison.Ordinal))
            return lib;

        if (Path.IsPathRooted(lib) || lib.Contains('/') || lib.Contains('\\'))
            return $"\"{lib.Replace("\\", "/")}\"";

        if (lib.StartsWith("lib", StringComparison.Ordinal) &&
            (lib.EndsWith(".a", StringComparison.OrdinalIgnoreCase) ||
             lib.EndsWith(".so", StringComparison.OrdinalIgnoreCase) ||
             lib.EndsWith(".dylib", StringComparison.OrdinalIgnoreCase)))
        {
            return "-l" + Path.GetFileNameWithoutExtension(lib).Substring(3);
        }

        return "-l" + lib;
    }

    private static string ToNinjaPath(string path)
    {
        return path.Replace("\\", "/")
                   .Replace(" ", "$ ")
                   .Replace(":", "$:");
    }
}
