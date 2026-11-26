using System.Text;
using System.Text.RegularExpressions;
using Aether.Presets;
using Aether.Rules;

namespace Aether.Toolchain;

public static partial class NinjaGenerator
{
    public static string Generate(string workspaceRoot, TargetRules target, Toolchain toolchain, Preset preset, string targetBin)
    {
        string templatePath = Path.Combine(workspaceRoot, "Scripts", "Templates", "build.ninja.templ");
        if (!File.Exists(templatePath))
            throw new FileNotFoundException($"[Aether] Ninja template not found at '{templatePath}'");

        string template = File.ReadAllText(templatePath).Replace("\r\n", "\n");

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

        string headerSection = ExtractSection(template, "TOOLCHAIN_HEADER");
        string msvcRulesSection = ExtractSection(template, "MSVC_RULES");
        string unixRulesSection = ExtractSection(template, "UNIX_RULES");
        string moduleObjSection = ExtractSection(template, "MODULE_OBJECT");
        string moduleLinkWinSection = ExtractSection(template, "MODULE_LINK_WINDOWS");
        string moduleLinkUnixSection = ExtractSection(template, "MODULE_LINK_UNIX");
        string targetObjSection = ExtractSection(template, "TARGET_OBJECT");
        string targetLinkSection = ExtractSection(template, "TARGET_LINK");
        string defaultTargetSection = ExtractSection(template, "DEFAULT_TARGET");

        var ninja = new StringBuilder();
        ninja.AppendLine("ninja_required_version = 1.5");
        ninja.AppendLine();

        ninja.AppendLine(Render(headerSection, new Dictionary<string, string>
        {
            ["COMPILER"] = toolchain.Compiler,
            ["C_COMPILER"] = toolchain.CCompiler,
            ["LINKER"] = toolchain.Linker
        }));
        ninja.AppendLine();

        string definePrefix = toolchain.Family == ToolchainFamily.MSVC ? "/D" : "-D";
        string globalDefines = string.Join(" ", preset.Defines.Select(d => $"{definePrefix}{d}"));

        if (toolchain.Family == ToolchainFamily.MSVC)
        {
            string opt = isDebug ? "/Od /RTC1 /Z7 /MDd" : "/O2 /DNDEBUG /MD";
            ninja.AppendLine(Render(msvcRulesSection, new Dictionary<string, string>
            {
                ["CXX_FLAGS"] = $"/nologo /std:c++20 /EHsc /utf-8 /W3 {opt}",
                ["CC_FLAGS"] = $"/nologo /utf-8 /W3 {opt}",
                ["LINK_FLAGS"] = isDebug ? "/nologo /DEBUG" : "/nologo"
            }));
        }
        else
        {
            string opt = isDebug ? "-O0 -g" : "-O3 -DNDEBUG";
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

            ninja.AppendLine(Render(unixRulesSection, new Dictionary<string, string>
            {
                ["CXX_FLAGS"] = $"-std=c++20 -Wall -fPIC {opt}",
                ["CC_FLAGS"] = $"-Wall -fPIC {opt}",
                ["LINK_FLAGS"] = isDebug ? "-g" : "",
                ["SHARED_FLAG"] = isMac ? "-dynamiclib" : "-shared",
                ["RPATH_FLAGS"] = string.Join(" ", rpathFlags)
            }));
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

            bool moduleJumbo = IsJumboEnabled(module.EnableJumbo, module.Defines, preset.Defines);
            var moduleSources = ResolveCompileSources(
                module.SourceFiles,
                moduleJumbo,
                module.JumboBatchSize,
                module.JumboExcludes,
                module.ExternalSourceFiles,
                Path.Combine(intermediateDir, "jumbo", module.Name),
                module.Name);

            foreach (var src in moduleSources)
            {
                string absoluteSrc = Path.GetFullPath(src);
                string normSrc = ToNinjaPath(absoluteSrc);
                string relSrc = Path.GetRelativePath(workspaceRoot, absoluteSrc)
                    .Replace("\\", "_")
                    .Replace("/", "_");
                string objName = Path.ChangeExtension(relSrc, isWindows ? ".obj" : ".o");
                string objFile = ToNinjaPath($"{intermediateDir}/{objName}");
                string ruleName = absoluteSrc.EndsWith(".c", StringComparison.OrdinalIgnoreCase) ? "compile_c" : "compile";

                ninja.AppendLine(Render(moduleObjSection, new Dictionary<string, string>
                {
                    ["OBJ_FILE"] = objFile,
                    ["RULE_NAME"] = ruleName,
                    ["SRC_FILE"] = normSrc,
                    ["DEFINES"] = moduleDefines,
                    ["INCLUDES"] = moduleIncludes
                }));
                moduleObjFiles.Add(objFile);
            }

            string dllFile;
            if (isWindows)
            {
                dllFile = ToNinjaPath(Path.Combine(binDir, $"{module.Name}.dll"));
                string implibFile = ToNinjaPath(Path.Combine(binDir, $"{module.Name}.lib"));
                ninja.AppendLine(Render(moduleLinkWinSection, new Dictionary<string, string>
                {
                    ["DLL_FILE"] = dllFile,
                    ["OBJ_FILES"] = string.Join(" ", moduleObjFiles),
                    ["IMPLIB_FILE"] = implibFile
                }));
                moduleImportLibs.Add(implibFile);
            }
            else if (isMac)
            {
                string rawDll = Path.Combine(binDir, $"lib{module.Name}.dylib");
                dllFile = ToNinjaPath(rawDll);
                ninja.AppendLine(Render(moduleLinkUnixSection, new Dictionary<string, string>
                {
                    ["DLL_FILE"] = dllFile,
                    ["OBJ_FILES"] = string.Join(" ", moduleObjFiles)
                }));
                moduleImportLibs.Add($"\"{rawDll.Replace("\\", "/")}\"");
            }
            else
            {
                string rawDll = Path.Combine(binDir, $"lib{module.Name}.so");
                dllFile = ToNinjaPath(rawDll);
                ninja.AppendLine(Render(moduleLinkUnixSection, new Dictionary<string, string>
                {
                    ["DLL_FILE"] = dllFile,
                    ["OBJ_FILES"] = string.Join(" ", moduleObjFiles)
                }));
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

        bool targetJumbo = IsJumboEnabled(target.EnableJumbo, target.Defines, preset.Defines);
        var targetSources = ResolveCompileSources(
            target.SourceFiles,
            targetJumbo,
            target.JumboBatchSize,
            target.JumboExcludes,
            target.ExternalSourceFiles,
            Path.Combine(intermediateDir, "jumbo", target.Name),
            target.Name);

        foreach (var src in targetSources)
        {
            string absoluteSrc = Path.GetFullPath(src);
            string normSrc = ToNinjaPath(absoluteSrc);
            string relSrc = Path.GetRelativePath(workspaceRoot, absoluteSrc)
                .Replace("\\", "_")
                .Replace("/", "_");
            string objName = Path.ChangeExtension(relSrc, isWindows ? ".obj" : ".o");
            string objFile = ToNinjaPath(Path.Combine(intermediateDir, objName));
            string ruleName = absoluteSrc.EndsWith(".c", StringComparison.OrdinalIgnoreCase) ? "compile_c" : "compile";

            ninja.AppendLine(Render(targetObjSection, new Dictionary<string, string>
            {
                ["OBJ_FILE"] = objFile,
                ["RULE_NAME"] = ruleName,
                ["SRC_FILE"] = normSrc,
                ["DEFINES"] = targetDefines,
                ["INCLUDES"] = targetIncludes
            }));

            targetObjFiles.Add(objFile);
        }

        string exeName = !string.IsNullOrEmpty(target.OutputExecutable)
            ? target.OutputExecutable
            : (isWindows ? $"{target.Name}.exe" : target.Name);
        if (isWindows && !exeName.EndsWith(".exe", StringComparison.OrdinalIgnoreCase))
            exeName += ".exe";

        string exePath = ToNinjaPath(Path.Combine(binDir, exeName));
        string orderDeps = allDllOutputs.Count > 0 ? " | " + string.Join(" ", allDllOutputs) : "";

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

        string libsSubSection = ExtractSection(targetLinkSection, "TARGET_LIBS");
        string libPathsSubSection = ExtractSection(targetLinkSection, "TARGET_LIBPATHS");

        string renderedLink = ReplaceSection(
            targetLinkSection,
            "TARGET_LIBS",
            allLibs.Count > 0
                ? Render(libsSubSection, new Dictionary<string, string> { ["LIBS"] = string.Join(" ", allLibs) })
                : null);

        string libPathPrefix = toolchain.Family == ToolchainFamily.MSVC ? "/LIBPATH:" : "-L";
        string libPathsStr = string.Join(" ", target.LibraryPaths.Select(p => $"{libPathPrefix}\"{p.Replace("\\", "/")}\""));
        renderedLink = ReplaceSection(
            renderedLink,
            "TARGET_LIBPATHS",
            target.LibraryPaths.Count > 0
                ? Render(libPathsSubSection, new Dictionary<string, string> { ["LIBPATHS"] = libPathsStr })
                : null);

        ninja.AppendLine(Render(renderedLink, new Dictionary<string, string>
        {
            ["EXE_PATH"] = exePath,
            ["TARGET_OBJ_FILES"] = string.Join(" ", targetObjFiles),
            ["ORDER_DEPS"] = orderDeps
        }));

        ninja.AppendLine();
        ninja.AppendLine(Render(defaultTargetSection, new Dictionary<string, string>
        {
            ["EXE_PATH"] = exePath
        }));

        string ninjaFilePath = Path.Combine(intermediateDir, "build.ninja");
        File.WriteAllText(ninjaFilePath, ninja.ToString());

        return ninjaFilePath;
    }

    private static string ExtractSection(string content, string sectionName)
    {
        string openTag = $"# <${sectionName}>\n";
        string closeTag = $"\n# </${sectionName}>";

        int startIdx = content.IndexOf(openTag, StringComparison.Ordinal);
        if (startIdx < 0)
            throw new FormatException($"[Aether] Section '# <${sectionName}>' not found in build.ninja.templ");

        int bodyStart = startIdx + openTag.Length;
        int endIdx = content.IndexOf(closeTag, bodyStart, StringComparison.Ordinal);
        if (endIdx < 0)
            throw new FormatException($"[Aether] Closing tag '# </${sectionName}>' not found in build.ninja.templ");

        return content.Substring(bodyStart, endIdx - bodyStart);
    }

    private static string ReplaceSection(string content, string sectionName, string? replacement)
    {
        string openTag = $"# <${sectionName}>";
        string closeTag = $"# </${sectionName}>";

        int startIdx = content.IndexOf(openTag, StringComparison.Ordinal);
        if (startIdx < 0)
            return content;

        int endTagIdx = content.IndexOf(closeTag, startIdx, StringComparison.Ordinal);
        if (endTagIdx < 0)
            return content;

        int afterEnd = endTagIdx + closeTag.Length;
        if (replacement == null)
        {
            // Remove the whole block including the preceding newline if present
            int removeStart = startIdx > 0 && content[startIdx - 1] == '\n' ? startIdx - 1 : startIdx;
            return content.Remove(removeStart, afterEnd - removeStart);
        }

        return content.Substring(0, startIdx) + replacement + content.Substring(afterEnd);
    }

    private static string Render(string template, IReadOnlyDictionary<string, string> vars)
    {
        return TemplateVarRegex().Replace(template, match =>
        {
            string key = match.Groups[1].Value;
            return vars.TryGetValue(key, out string? value) ? value : match.Value;
        });
    }

    private static bool IsJumboEnabled(bool propertyFlag, IEnumerable<string> localDefines, IEnumerable<string> presetDefines)
    {
        if (propertyFlag)
            return true;

        string? env = Environment.GetEnvironmentVariable("ENABLE_JUMBO");
        if (string.Equals(env, "1", StringComparison.OrdinalIgnoreCase) ||
            string.Equals(env, "true", StringComparison.OrdinalIgnoreCase))
        {
            return true;
        }

        return HasEnableJumboDefine(localDefines) || HasEnableJumboDefine(presetDefines);
    }

    private static bool HasEnableJumboDefine(IEnumerable<string> defines)
    {
        foreach (string def in defines)
        {
            if (def.Equals("ENABLE_JUMBO", StringComparison.OrdinalIgnoreCase) ||
                def.Equals("ENABLE_JUMBO=1", StringComparison.OrdinalIgnoreCase) ||
                def.Equals("ENABLE_JUMBO=true", StringComparison.OrdinalIgnoreCase))
            {
                return true;
            }
        }
        return false;
    }

    private static List<string> ResolveCompileSources(
        IReadOnlyList<string> sourceFiles,
        bool enableJumbo,
        int batchSize,
        IReadOnlyList<string> excludes,
        HashSet<string> externalSources,
        string jumboDir,
        string scopeName)
    {
        if (!enableJumbo || sourceFiles.Count <= 1)
            return sourceFiles.ToList();

        int effectiveBatchSize = batchSize > 0 ? batchSize : 8;
        Directory.CreateDirectory(jumboDir);

        var standalone = new List<string>();
        var cppBatchable = new List<string>();
        var cBatchable = new List<string>();

        foreach (string src in sourceFiles)
        {
            string fullPath = Path.GetFullPath(src);
            if (externalSources.Contains(fullPath) || IsExcludedFromJumbo(fullPath, excludes))
            {
                standalone.Add(fullPath);
                continue;
            }

            if (fullPath.EndsWith(".c", StringComparison.OrdinalIgnoreCase))
                cBatchable.Add(fullPath);
            else
                cppBatchable.Add(fullPath);
        }

        cppBatchable.Sort(StringComparer.Ordinal);
        cBatchable.Sort(StringComparer.Ordinal);

        var result = new List<string>();
        result.AddRange(CreateJumboUnits(cppBatchable, effectiveBatchSize, jumboDir, scopeName, ".cpp"));
        result.AddRange(CreateJumboUnits(cBatchable, effectiveBatchSize, jumboDir, scopeName, ".c"));
        result.AddRange(standalone);

        Console.WriteLine($"[Aether] Jumbo ({scopeName}): {sourceFiles.Count} files -> {result.Count} translation units");
        return result;
    }

    private static bool IsExcludedFromJumbo(string fullPath, IReadOnlyList<string> excludes)
    {
        foreach (string pattern in excludes)
        {
            if (!string.IsNullOrWhiteSpace(pattern) &&
                fullPath.Contains(pattern, StringComparison.OrdinalIgnoreCase))
            {
                return true;
            }
        }

        // Files with #pragma once or *_IMPLEMENTATION macros cannot safely share a unity TU
        try
        {
            foreach (string line in File.ReadLines(fullPath).Take(20))
            {
                string trimmed = line.Trim();
                if (trimmed.StartsWith("#pragma once", StringComparison.Ordinal) ||
                    (trimmed.StartsWith("#define", StringComparison.Ordinal) &&
                     trimmed.Contains("_IMPLEMENTATION", StringComparison.Ordinal)))
                {
                    return true;
                }
            }
        }
        catch
        {
            return true;
        }

        return false;
    }

    private static List<string> CreateJumboUnits(
        List<string> files,
        int batchSize,
        string jumboDir,
        string scopeName,
        string extension)
    {
        var units = new List<string>();
        if (files.Count == 0)
            return units;

        string safeScope = scopeName.Replace(" ", "_");
        string kindTag = extension == ".c" ? "c" : "cxx";
        int batchIndex = 0;

        for (int i = 0; i < files.Count; i += batchSize)
        {
            var chunk = files.Skip(i).Take(batchSize).ToList();
            if (chunk.Count == 1)
            {
                units.Add(chunk[0]);
                continue;
            }

            string unitPath = Path.Combine(jumboDir, $"jumbo_{safeScope}_{kindTag}_{batchIndex++}{extension}");
            var sb = new StringBuilder();
            sb.AppendLine("// Auto-generated by Aether Jumbo Build");
            foreach (string file in chunk)
            {
                sb.AppendLine($"#include \"{file.Replace("\\", "/")}\"");
            }

            string content = sb.ToString();
            if (!File.Exists(unitPath) || File.ReadAllText(unitPath) != content)
            {
                File.WriteAllText(unitPath, content);
            }

            units.Add(unitPath);
        }

        return units;
    }

    [GeneratedRegex(@"\$([A-Z0-9_]+)")]
    private static partial Regex TemplateVarRegex();

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
