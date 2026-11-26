using Aether.Presets;
using Aether.Toolchain;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using System.Diagnostics;
using System.Reflection;

namespace Aether.Rules;

public static class TargetLoader
{
    public static TargetRules ResolveTarget(string workspaceRoot, string targetName, string cmakePath, string ninjaPath, string binDir, Preset preset)
    {
        string cachePath = Path.Combine(workspaceRoot, ".cache");
        Directory.CreateDirectory(cachePath);

        var targetFiles = Directory.GetFiles(workspaceRoot, $"{targetName}.Target.cs", SearchOption.AllDirectories)
            .Where(f => !f.Contains(".cache") && !f.Contains("obj") && !f.Contains("bin") && !f.Contains("Bin"))
            .ToList();

        if (targetFiles.Count == 0)
            throw new FileNotFoundException($"[Aether] '{targetName}.Target.cs' not found in {workspaceRoot}");

        var targetAssembly = CompileScripts(new[] { targetFiles[0] }, $"Aether_Target_{targetName}");
        var targetType = targetAssembly.GetTypes().FirstOrDefault(t =>
            typeof(TargetRules).IsAssignableFrom(t) && !t.IsAbstract)
            ?? throw new InvalidOperationException($"No TargetRules class found in {targetFiles[0]}");

        var ctorTargetConfig = targetType.GetConstructor(new[] { typeof(string), typeof(string) });
        var ctorTarget = targetType.GetConstructor(new[] { typeof(string) });

        var target = (TargetRules)(ctorTargetConfig != null
            ? ctorTargetConfig.Invoke(new object[] { workspaceRoot, preset.Configuration })
            : (ctorTarget != null
                ? ctorTarget.Invoke(new object[] { workspaceRoot })
                : Activator.CreateInstance(targetType)!));

        target.Configuration = preset.Configuration;
        target.Platform = preset.Platform;
        target.Architecture = preset.Architecture;

        Console.WriteLine($"[Aether] Target: {target.Name} | Required modules: [{string.Join(", ", target.Modules)}]");

        var moduleScriptPaths = new List<string>();
        foreach (var moduleName in target.Modules)
        {
            var found = Directory.GetFiles(workspaceRoot, $"{moduleName}.Module.cs", SearchOption.AllDirectories)
                .Where(f => !f.Contains(".cache") && !f.Contains("obj") && !f.Contains("bin") && !f.Contains("Bin"))
                .ToList();

            if (found.Count == 0)
                throw new FileNotFoundException($"[Aether] Target '{target.Name}' requires '{moduleName}', but '{moduleName}.Module.cs' was not found.");

            Console.WriteLine($"[Aether] Module found: {Path.GetFileName(found[0])}");
            moduleScriptPaths.Add(found[0]);
        }

        var externalRules = new List<ExternalRules>();

        if (moduleScriptPaths.Count > 0)
        {
            var modulesAssembly = CompileScripts(moduleScriptPaths, $"Aether_Modules_{targetName}");
            foreach (var type in modulesAssembly.GetTypes().Where(t => typeof(ModuleRules).IsAssignableFrom(t) && !t.IsAbstract))
            {
                var ctor = type.GetConstructor(new[] { typeof(string) });
                var moduleInstance = (ModuleRules)(ctor != null
                    ? ctor.Invoke(new object[] { workspaceRoot })
                    : Activator.CreateInstance(type)!);

                target.ResolvedModules.Add(moduleInstance);
                externalRules.AddRange(moduleInstance.Externals);
            }
        }

        externalRules.AddRange(target.Externals);

        foreach (ExternalRules externalRule in externalRules)
        {
            string externalPath = Path.Combine(cachePath, externalRule.Name);

            // Pull repository with git if not cloned yet
            bool externalIsEmpty = Directory.Exists(externalPath) &&
                                   Directory.GetFileSystemEntries(externalPath).Length == 0;
            if (!string.IsNullOrWhiteSpace(externalRule.Repo) &&
                (!Directory.Exists(externalPath) || externalIsEmpty))
            {
                if (externalIsEmpty)
                    Directory.Delete(externalPath);

                Console.ForegroundColor = ConsoleColor.Yellow;
                Console.WriteLine($"[INFO] Cloning {externalRule.Name} from {externalRule.Repo} with git...");
                Console.ResetColor();

                string branchArgs = string.IsNullOrWhiteSpace(externalRule.Branch)
                    ? string.Empty
                    : $" --branch {externalRule.Branch} --single-branch";
                var psi = new ProcessStartInfo
                {
                    FileName = "git",
                    Arguments = $"clone{branchArgs} {externalRule.Repo} \"{externalPath}\"",
                    UseShellExecute = false,
                    WorkingDirectory = cachePath
                };

                using var process = Process.Start(psi) ?? throw new Exception($"Failed to start git clone for {externalRule.Name}");
                process.WaitForExit();
                if (process.ExitCode != 0)
                {
                    throw new Exception($"git clone exited with code {process.ExitCode} for {externalRule.Name} ({externalRule.Repo})");
                }
            }

            if (!string.IsNullOrWhiteSpace(externalRule.Branch) && Directory.Exists(Path.Combine(externalPath, ".git")))
            {
                var checkout = new ProcessStartInfo
                {
                    FileName = "git",
                    Arguments = $"-C \"{externalPath}\" checkout {externalRule.Branch}",
                    UseShellExecute = false,
                    WorkingDirectory = cachePath
                };

                using var checkoutProcess = Process.Start(checkout);
                checkoutProcess?.WaitForExit();
            }

            EnsureCacheIncludeAlias(cachePath, externalRule.Name, externalPath);

            // Check if it's a cmake build or not and build it
            if (externalRule.BuildWithCMake && Directory.Exists(externalPath))
            {
                string staticLibDir = Path.Combine(externalPath, "static-lib");
                string buildDir = Path.Combine(externalPath, "build");
                string libExt = OperatingSystem.IsWindows() ? "*.lib" : "*.a";
                string sharedLibExt = OperatingSystem.IsMacOS() ? "*.dylib" : "*.so";
                bool hasStatic = Directory.Exists(staticLibDir) && Directory.GetFiles(staticLibDir, libExt).Length > 0;
                bool hasShared = !OperatingSystem.IsWindows() && Directory.Exists(staticLibDir) && Directory.GetFiles(staticLibDir, sharedLibExt).Length > 0;
                bool alreadyBuilt = hasStatic || hasShared;

                string cmakeArgsMarker = Path.Combine(staticLibDir, ".aether-cmake-args");
                string cmakeCacheKey = $"{target.Platform}|{target.Architecture}|{target.Configuration}|{externalRule.Branch}|{externalRule.CMakeArguments}".Trim();
                if (alreadyBuilt)
                {
                    alreadyBuilt = File.Exists(cmakeArgsMarker) &&
                                   File.ReadAllText(cmakeArgsMarker).Trim() == cmakeCacheKey;
                }

                if (!alreadyBuilt)
                {
                    if (Directory.Exists(staticLibDir))
                        Directory.Delete(staticLibDir, recursive: true);
                    if (Directory.Exists(buildDir))
                        Directory.Delete(buildDir, recursive: true);

                    BuildWithCMake(cmakePath, ninjaPath, externalPath, binDir, externalRule.CMakeArguments, target.Configuration);
                    Directory.CreateDirectory(staticLibDir);
                    File.WriteAllText(cmakeArgsMarker, cmakeCacheKey);
                }
                else
                {
                    Console.WriteLine($"[Aether] External '{externalRule.Name}' is already built.");
                }

                if (Directory.Exists(staticLibDir))
                {
                    if (!target.LibraryPaths.Contains(staticLibDir))
                        target.LibraryPaths.Add(staticLibDir);

                    var collectedFiles = Directory.GetFiles(staticLibDir, libExt);
                    string activeExt = libExt;
                    if (collectedFiles.Length == 0 && !OperatingSystem.IsWindows())
                    {
                        collectedFiles = Directory.GetFiles(staticLibDir, sharedLibExt);
                        activeExt = sharedLibExt;
                    }

                    foreach (var lib in collectedFiles)
                    {
                        string libName = Path.GetFileName(lib);

                        if (target.Configuration.Equals("Debug", StringComparison.OrdinalIgnoreCase))
                        {
                            string stem = Path.GetFileNameWithoutExtension(libName);
                            string baseStem = stem.EndsWith("d", StringComparison.OrdinalIgnoreCase)
                                ? stem.Substring(0, stem.Length - 1)
                                : stem;
                            string debugVariant = baseStem + "d" + activeExt.Substring(1);

                            if (!stem.EndsWith("d", StringComparison.OrdinalIgnoreCase) &&
                                File.Exists(Path.Combine(staticLibDir, debugVariant)))
                            {
                                continue;
                            }
                        }

                        string libEntry = OperatingSystem.IsWindows() ? libName : lib;
                        if (!target.Libraries.Contains(libEntry))
                            target.Libraries.Add(libEntry);
                    }
                }

                string extInc = Path.Combine(externalPath, "include");
                if (Directory.Exists(extInc) && !target.IncludePaths.Contains(extInc))
                    target.IncludePaths.Add(extInc);

                string extBuildInc = Path.Combine(externalPath, "build", "include");
                if (Directory.Exists(extBuildInc) && !target.IncludePaths.Contains(extBuildInc))
                    target.IncludePaths.Add(extBuildInc);
            }
            else
            {
                foreach (var inc in externalRule.IncludePaths)
                {
                    string resolvedInc = Path.IsPathRooted(inc) ? inc : Path.Combine(externalPath, inc);
                    if (Directory.Exists(resolvedInc) && !target.IncludePaths.Contains(resolvedInc))
                        target.IncludePaths.Add(resolvedInc);
                }

                foreach (var src in externalRule.SourceFiles)
                {
                    string resolvedSrc = Path.IsPathRooted(src) ? src : Path.Combine(externalPath, src);
                    if (File.Exists(resolvedSrc) && !target.SourceFiles.Contains(resolvedSrc))
                        target.SourceFiles.Add(resolvedSrc);
                }
            }

            foreach (var inc in externalRule.IncludePaths)
            {
                string resolvedInc = Path.IsPathRooted(inc) ? inc : Path.Combine(externalPath, inc);
                if (Directory.Exists(resolvedInc) && !target.IncludePaths.Contains(resolvedInc))
                    target.IncludePaths.Add(resolvedInc);
            }

            foreach (var def in externalRule.Defines)
            {
                if (!target.Defines.Contains(def))
                    target.Defines.Add(def);
            }

            foreach (var lib in externalRule.Libraries)
            {
                if (!target.Libraries.Contains(lib))
                    target.Libraries.Add(lib);
            }

            foreach (var libPath in externalRule.LibraryPaths)
            {
                string resolvedLibPath = Path.IsPathRooted(libPath) ? libPath : Path.Combine(externalPath, libPath);
                if (Directory.Exists(resolvedLibPath) && !target.LibraryPaths.Contains(resolvedLibPath))
                    target.LibraryPaths.Add(resolvedLibPath);
            }
        }

        return target;
    }

    private static Assembly CompileScripts(IEnumerable<string> filePaths, string assemblyName)
    {
        var syntaxTrees = filePaths.Select(f => CSharpSyntaxTree.ParseText(File.ReadAllText(f), path: f)).ToList();

        var references = AppDomain.CurrentDomain.GetAssemblies()
            .Where(a => !a.IsDynamic && !string.IsNullOrWhiteSpace(a.Location))
            .Select(a => MetadataReference.CreateFromFile(a.Location))
            .Cast<MetadataReference>()
            .ToList();

        var compilation = CSharpCompilation.Create(
            assemblyName,
            syntaxTrees,
            references,
            new CSharpCompilationOptions(OutputKind.DynamicallyLinkedLibrary)
        );

        using var ms = new MemoryStream();
        var result = compilation.Emit(ms);

        if (!result.Success)
        {
            Console.ForegroundColor = ConsoleColor.Red;
            Console.WriteLine("[ERROR] Failed compiling C# scripts:");
            foreach (var diag in result.Diagnostics.Where(d => d.Severity == DiagnosticSeverity.Error))
            {
                var span = diag.Location.GetLineSpan();
                Console.WriteLine($"  -> {Path.GetFileName(span.Path)}:{span.StartLinePosition.Line + 1} - {diag.GetMessage()}");
            }
            Console.ResetColor();
            throw new Exception("C# rule script compilation failed.");
        }

        ms.Seek(0, SeekOrigin.Begin);
        return Assembly.Load(ms.ToArray());
    }

    private static void EnsureCacheIncludeAlias(string cachePath, string externalName, string externalPath)
    {
        string[] aliasNames = { "nanosvg", "stb", "miniz", "tinyexr", "xxhash" };
        if (!aliasNames.Contains(externalName, StringComparer.OrdinalIgnoreCase))
            return;

        string aliasDir = Path.Combine(cachePath, externalName);
        Directory.CreateDirectory(aliasDir);

        foreach (string header in Directory.EnumerateFiles(externalPath, "*.*", SearchOption.AllDirectories)
                     .Where(p => p.EndsWith(".h", StringComparison.OrdinalIgnoreCase) ||
                                 p.EndsWith(".hpp", StringComparison.OrdinalIgnoreCase)))
        {
            string relativeParts = Path.GetRelativePath(externalPath, header);
            string fileName = Path.GetFileName(header);

            if (relativeParts.StartsWith(".git", StringComparison.OrdinalIgnoreCase))
                continue;

            string dest = Path.Combine(aliasDir, fileName);
            if (!File.Exists(dest) ||
                File.GetLastWriteTimeUtc(dest) < File.GetLastWriteTimeUtc(header))
            {
                File.Copy(header, dest, overwrite: true);
            }

            string cacheRootDest = Path.Combine(cachePath, fileName);
            if (!File.Exists(cacheRootDest) ||
                File.GetLastWriteTimeUtc(cacheRootDest) < File.GetLastWriteTimeUtc(header))
            {
                File.Copy(header, cacheRootDest, overwrite: true);
            }
        }

        if (externalName.Equals("tinyexr", StringComparison.OrdinalIgnoreCase))
        {
            string bundledMiniz = Path.Combine(externalPath, "deps", "miniz", "miniz.h");
            if (File.Exists(bundledMiniz))
            {
                Directory.CreateDirectory(aliasDir);
                File.Copy(bundledMiniz, Path.Combine(aliasDir, "miniz.h"), overwrite: true);
                File.Copy(bundledMiniz, Path.Combine(cachePath, "miniz.h"), overwrite: true);
            }
        }
    }

    private static void BuildWithCMake(string cmakePath, string ninjaPath, string modulePath, string binDir, string cmakeArguments, string configuration)
    {
        string ninjaArg = !string.IsNullOrEmpty(ninjaPath)
            ? $"-G Ninja -DCMAKE_MAKE_PROGRAM=\"{ninjaPath.Replace("\\", "/")}\""
            : "";
        string extraArguments = string.IsNullOrWhiteSpace(cmakeArguments) ? string.Empty : $" {cmakeArguments}";
        bool isDebug = configuration.Equals("Debug", StringComparison.OrdinalIgnoreCase);
        string buildType = isDebug ? "Debug" : "Release";
        string runtimeLibrary = OperatingSystem.IsWindows()
            ? (isDebug ? " -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDebugDLL" : " -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL")
            : string.Empty;
        RunCMake(cmakePath, $"-S . -B build {ninjaArg} -DCMAKE_BUILD_TYPE={buildType}{runtimeLibrary}{extraArguments}", modulePath);
        RunCMake(cmakePath, $"--build build --config {buildType}", modulePath);

        ArtifactCollector.Collect(modulePath, binDir, buildType);
    }

    private static void RunCMake(string cmakePath, string arguments, string workingDirectory)
    {
        var psi = new ProcessStartInfo
        {
            FileName = cmakePath,
            Arguments = arguments,
            UseShellExecute = false,
            WorkingDirectory = workingDirectory
        };

        using var process = Process.Start(psi) ?? throw new Exception("Failed to start CMake");
        process.WaitForExit();

        if (process.ExitCode != 0)
            throw new Exception($"CMake failed with exit code {process.ExitCode}: {arguments}");
    }
}
