using System.Diagnostics;
using Aether.Presets;
using Aether.Rules;
using Aether.Toolchain;

namespace Aether;

internal static class Program
{
    public enum CommandType
    {
        Build,
        ClearCache,
        GetDeps,
        GenerateVs
    };

    public static CommandType ConvertCommand(string command)
    {
        return command.ToLowerInvariant() switch
        {
            "build" => CommandType.Build,
            "clearcache" or "clean" => CommandType.ClearCache,
            "get-deps" or "deps" => CommandType.GetDeps,
            "generate-vs" or "gen-vs" or "setup-vs" => CommandType.GenerateVs,
            _ => throw new ArgumentException($"Unknown command: '{command}'")
        };
    }

    static async Task<int> Main(string[] args)
    {
        Console.ForegroundColor = ConsoleColor.Cyan;
        Console.WriteLine("====================================");
        Console.WriteLine("   Aether(ion) Build Tool v0.1");
        Console.WriteLine("====================================");
        Console.ResetColor();

        string command;
        string[] cmdArgs;

        if (args.Length == 0)
        {
#if DEBUG
            command = "build";
            cmdArgs = args;
#else
            PrintUsage();
            return 1;
#endif
        }
        else
        {
            command = args[0];
            cmdArgs = args;
        }

        // Detect repo root by walking up from CWD first, then from binary location
        string? repoRoot = FindRepoRoot(Directory.GetCurrentDirectory())
                        ?? FindRepoRoot(AppDomain.CurrentDomain.BaseDirectory);

        if (repoRoot == null)
        {
            PrintError("Could not find project root. Run aether from within the Aetherion repository, or place the binary inside it.");
            return 1;
        }

        Console.WriteLine($"[Aether] Project root: {repoRoot}");

        CommandType cmd;
        try
        {
            cmd = ConvertCommand(command);
        }
        catch (Exception ex)
        {
            PrintError(ex.Message);
            PrintUsage();
            return 1;
        }

        switch (cmd)
        {
            case CommandType.ClearCache:
                return HandleClearCache(repoRoot);
            case CommandType.GetDeps:
                return await HandleGetDepsAsync(repoRoot, cmdArgs);
            case CommandType.Build:
                return await HandleBuildAsync(repoRoot, cmdArgs);
            case CommandType.GenerateVs:
                return HandleGenerateVs(repoRoot, cmdArgs);
            default:
                return 1;
        }
    }

    private static string GetDefaultPresetName()
    {
        if (OperatingSystem.IsMacOS()) return "macos-debug-spdlog";
        if (OperatingSystem.IsLinux()) return "linux-debug-spdlog";
        return "x64-debug-spdlog";
    }

    private static async Task<int> HandleGetDepsAsync(string repoRoot, string[] args)
    {
        string targetName = args.Length > 1 ? args[1] : "Aetherion";
        string presetName = args.Length > 2 ? args[2] : GetDefaultPresetName();

        try
        {
            string cacheDir = Path.Combine(repoRoot, ".cache");
            if (!Directory.Exists(cacheDir))
                Directory.CreateDirectory(cacheDir);

            string ninjaPath = await NinjaProvider.EnsureNinjaAsync(repoRoot);
            string cmakePath = await CMakeProvider.EnsureCMakeAsync(repoRoot);

            var preset = PresetManager.LoadPreset(repoRoot, presetName);
            string targetBin = Path.Combine(repoRoot, "Bin", targetName, preset.Configuration);

            Console.WriteLine($"[Aether] Resolving and fetching dependencies for target '{targetName}'...");
            var target = TargetLoader.ResolveTarget(repoRoot, targetName, cmakePath, ninjaPath, targetBin, preset);

            Console.ForegroundColor = ConsoleColor.Green;
            Console.WriteLine($"\n[OK] Dependencies for {target.Name} synchronized successfully.");
            Console.ResetColor();
            return 0;
        }
        catch (Exception ex)
        {
            PrintError(ex.Message);
            return 1;
        }
    }

    private static async Task<int> HandleBuildAsync(string repoRoot, string[] args)
    {
        string targetName = args.Length > 1 ? args[1] : "Aetherion";
        string presetName = args.Length > 2 ? args[2] : GetDefaultPresetName();

        try
        {
            string cacheDir = Path.Combine(repoRoot, ".cache");
            if (!Directory.Exists(cacheDir))
                Directory.CreateDirectory(cacheDir);

            string ninjaPath = await NinjaProvider.EnsureNinjaAsync(repoRoot);
            string cmakePath = await CMakeProvider.EnsureCMakeAsync(repoRoot);

            var toolchain = Toolchain.Toolchain.FromEnvironment();
            Console.WriteLine($"[Aether] Compiler: {toolchain.Compiler} | Linker: {toolchain.Linker}");

            var preset = PresetManager.LoadPreset(repoRoot, presetName);
            Console.WriteLine($"[Aether] Preset: {preset.Name} ({preset.Configuration} / {preset.Architecture})");
            Console.WriteLine($"[Aether] Defines: {string.Join(", ", preset.Defines)}");

            string targetBin = Path.Combine(repoRoot, "Bin", targetName, preset.Configuration);

            var target = TargetLoader.ResolveTarget(repoRoot, targetName, cmakePath, ninjaPath, targetBin, preset);
            int totalCpp = target.SourceFiles.Count + target.ResolvedModules.Sum(m => m.SourceFiles.Count);
            Console.WriteLine($"[Aether] {target.Name}: {totalCpp} source files to compile");

            string ninjaFile = NinjaGenerator.Generate(repoRoot, target, toolchain, preset, targetBin);
            Console.WriteLine($"[Aether] Ninja file: {ninjaFile}");

            Console.ForegroundColor = ConsoleColor.Yellow;
            Console.WriteLine($"\n--- Building {target.Name} [{preset.Configuration}] ---");
            Console.ResetColor();

            var psi = new ProcessStartInfo
            {
                FileName = ninjaPath,
                Arguments = $"-f \"{ninjaFile}\"",
                UseShellExecute = false,
                WorkingDirectory = repoRoot
            };

            using var process = Process.Start(psi);
            process?.WaitForExit();

            if (process?.ExitCode != 0)
            {
                PrintError($"Build failed with exit code {process?.ExitCode}.");
                return process?.ExitCode ?? 1;
            }

            GenerateCompilationDatabase(ninjaPath, ninjaFile, repoRoot);

            Console.ForegroundColor = ConsoleColor.Green;
            Console.WriteLine($"\n[OK] Build completed successfully.");
            Console.WriteLine($"[OK] Output: Bin/{target.Name}/{preset.Configuration}/");
            Console.ResetColor();

            return 0;
        }
        catch (Exception ex)
        {
            PrintError(ex.Message);
            return 1;
        }
    }

    private static int HandleClearCache(string repoRoot)
    {
        string cacheDir = Path.Combine(repoRoot, ".cache");
        if (Directory.Exists(cacheDir))
        {
            try
            {
                Directory.Delete(cacheDir, recursive: true);
                Console.ForegroundColor = ConsoleColor.Green;
                Console.WriteLine("[Aether] .cache successfully deleted!");
                Console.ResetColor();
            }
            catch (Exception ex)
            {
                PrintError($"Couldn't delete .cache: {ex.Message}");
                return 1;
            }
        }
        else
        {
            Console.WriteLine("[Aether] Folder .cache was already clean.");
        }

        return 0;
    }

    private static int HandleGenerateVs(string repoRoot, string[] args)
    {
        string targetName = args.Length > 1 ? args[1] : "Aetherion";
        string presetName = args.Length > 2 ? args[2] : GetDefaultPresetName();
        string presetConfig = PresetManager.LoadPreset(repoRoot, presetName).Configuration;

        try
        {
            string scriptsDir = Path.Combine(repoRoot, "Scripts", "Batch");
            Directory.CreateDirectory(scriptsDir);

            string batchPath = Path.Combine(scriptsDir, "AetherBuild.bat");
            string batch = $@"@echo off
setlocal

set ""ROOT_DIR=%~dp0..\..\""
pushd ""%ROOT_DIR%"" || exit /b 1

dotnet build ""%ROOT_DIR%Tools\Aether\Aether.csproj"" -c Release --nologo -v minimal
if errorlevel 1 (
    popd
    exit /b 1
)

""%ROOT_DIR%bin\Aether\Aether.exe"" build {targetName} {presetName}
set ""EXIT_CODE=%ERRORLEVEL%""

popd
exit /b %EXIT_CODE%
";
            File.WriteAllText(batchPath, batch);

            string projectPath = Path.Combine(repoRoot, $"{targetName}.vcxproj");
            string projectGuid = CreateProjectGuid(targetName);

            var sourceRoots = new[]
            {
                Path.Combine(repoRoot, "Engine", "src"),
                Path.Combine(repoRoot, "Shared", "src")
            }.Where(Directory.Exists);

            var sourceFiles = sourceRoots
                .SelectMany(root => Directory.GetFiles(root, "*", SearchOption.AllDirectories))
                .Where(IsVisualStudioFile)
                .Distinct(StringComparer.OrdinalIgnoreCase)
                .OrderBy(path => path, StringComparer.OrdinalIgnoreCase)
                .ToList();

            var extraFiles = new[]
            {
                Path.Combine(repoRoot, "Engine", $"{targetName}.Target.cs"),
                Path.Combine(repoRoot, "AetherPresets.json"),
                batchPath
            }.Where(File.Exists).ToList();

            var allFiles = sourceFiles.Concat(extraFiles).Distinct(StringComparer.OrdinalIgnoreCase).ToList();
            var clCompile = sourceFiles.Where(IsCppSource).ToList();
            var clInclude = sourceFiles.Where(IsCppHeader).ToList();
            var noneFiles = allFiles.Except(clCompile).Except(clInclude).ToList();

            var filterFolders = allFiles
                .Select(path => Path.GetDirectoryName(Path.GetRelativePath(repoRoot, path))!)
                .Where(path => !string.IsNullOrWhiteSpace(path))
                .SelectMany(GetParentFilterFolders)
                .Distinct(StringComparer.OrdinalIgnoreCase)
                .OrderBy(path => path, StringComparer.OrdinalIgnoreCase)
                .ToList();

            var project = new System.Text.StringBuilder();
            project.AppendLine(@"<?xml version=""1.0"" encoding=""utf-8""?>");
            project.AppendLine(@"<Project DefaultTargets=""Build"" xmlns=""http://schemas.microsoft.com/developer/msbuild/2003"">");
            project.AppendLine(@"  <ItemGroup Label=""ProjectConfigurations"">");
            project.AppendLine($@"    <ProjectConfiguration Include=""{presetConfig}|x64"">");
            project.AppendLine($@"      <Configuration>{presetConfig}</Configuration>");
            project.AppendLine(@"      <Platform>x64</Platform>");
            project.AppendLine(@"    </ProjectConfiguration>");
            project.AppendLine(@"  </ItemGroup>");
            project.AppendLine(@"  <PropertyGroup Label=""Globals"">");
            project.AppendLine(@"    <VCProjectVersion>18.0</VCProjectVersion>");
            project.AppendLine($@"    <ProjectGuid>{projectGuid}</ProjectGuid>");
            project.AppendLine($@"    <ProjectName>{EscapeXml(targetName)}</ProjectName>");
            project.AppendLine(@"  </PropertyGroup>");
            project.AppendLine(@"  <Import Project=""$(VCTargetsPath)\Microsoft.Cpp.Default.props"" />");
            project.AppendLine($@"  <PropertyGroup Condition=""'$(Configuration)|$(Platform)'=='{presetConfig}|x64'"" Label=""Configuration"">");
            project.AppendLine(@"    <ConfigurationType>Utility</ConfigurationType>");
            project.AppendLine(@"    <PlatformToolset>v145</PlatformToolset>");
            project.AppendLine($@"    <OutDir>$(ProjectDir)Bin\{EscapeXml(targetName)}\{EscapeXml(presetConfig)}\</OutDir>");
            project.AppendLine($@"    <IntDir>$(ProjectDir).cache\Aetherion\VS\</IntDir>");
            project.AppendLine($@"    <TargetName>{EscapeXml(targetName)}</TargetName>");
            project.AppendLine(@"    <TargetExt>.exe</TargetExt>");
            project.AppendLine($@"    <TargetPath>$(OutDir){EscapeXml(targetName)}.exe</TargetPath>");
            project.AppendLine(@"  </PropertyGroup>");
            project.AppendLine(@"  <Import Project=""$(VCTargetsPath)\Microsoft.Cpp.props"" />");
            project.AppendLine(@"  <ItemDefinitionGroup>");
            project.AppendLine(@"    <ClCompile>");
            string includeDirectories = GetVisualStudioIncludeDirectories(repoRoot);
            project.AppendLine($@"      <AdditionalIncludeDirectories>{EscapeXml(includeDirectories)};%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>");
            project.AppendLine(@"      <LanguageStandard>stdcpp20</LanguageStandard>");
            project.AppendLine(@"      <PreprocessorDefinitions>SPDLOG_FMT_EXTERNAL=1;LOGGING_USE_SPDLOG=1;RENDERER_USE_VULKAN=1;EXCEPTION_HANDLER_PATH=&quot;\&quot;ExceptionHandler.exe\&quot;&quot;;GIT_COMMIT=&quot;\&quot;dev\&quot;&quot;;NOMINMAX;WIN32;_WIN32;%(PreprocessorDefinitions)</PreprocessorDefinitions>");
            project.AppendLine(@"      <AdditionalOptions>/Zc:preprocessor /permissive- /utf-8 %(AdditionalOptions)</AdditionalOptions>");
            project.AppendLine(@"      <ConformanceMode>true</ConformanceMode>");
            project.AppendLine(@"      <DisableLanguageExtensions>false</DisableLanguageExtensions>");
            project.AppendLine(@"      <DisableSpecificWarnings>4068;4996</DisableSpecificWarnings>");
            project.AppendLine(@"    </ClCompile>");
            project.AppendLine(@"  </ItemDefinitionGroup>");
            project.AppendLine($@"  <ItemDefinitionGroup Condition=""'$(Configuration)|$(Platform)'=='{presetConfig}|x64'"">");
            project.AppendLine(@"    <PreBuildEvent>");
            project.AppendLine(@"      <Command>call ""$(ProjectDir)Scripts\Batch\AetherBuild.bat""</Command>");
            project.AppendLine($@"      <Message>Building {EscapeXml(targetName)} with Aether and Ninja</Message>");
            project.AppendLine(@"    </PreBuildEvent>");
            project.AppendLine(@"  </ItemDefinitionGroup>");

            AppendProjectItems(project, "ClCompile", clCompile, repoRoot);
            AppendProjectItems(project, "ClInclude", clInclude, repoRoot);
            AppendProjectItems(project, "None", noneFiles, repoRoot);

            project.AppendLine(@"  <Import Project=""$(VCTargetsPath)\Microsoft.Cpp.targets"" />");
            project.AppendLine(@"  <ImportGroup Label=""ExtensionTargets"" />");
            project.AppendLine(@"</Project>");
            File.WriteAllText(projectPath, project.ToString());

            var filters = new System.Text.StringBuilder();
            filters.AppendLine(@"<?xml version=""1.0"" encoding=""utf-8""?>");
            filters.AppendLine(@"<Project ToolsVersion=""4.0"" xmlns=""http://schemas.microsoft.com/developer/msbuild/2003"">");
            if (filterFolders.Count > 0)
            {
                filters.AppendLine(@"  <ItemGroup>");
                foreach (string folder in filterFolders)
                {
                    filters.AppendLine($@"    <Filter Include=""{EscapeXml(folder)}"">");
                    filters.AppendLine($@"      <UniqueIdentifier>{CreateProjectGuid(folder)}</UniqueIdentifier>");
                    filters.AppendLine(@"    </Filter>");
                }
                filters.AppendLine(@"  </ItemGroup>");
            }
            AppendFilterItems(filters, "ClCompile", clCompile, repoRoot);
            AppendFilterItems(filters, "ClInclude", clInclude, repoRoot);
            AppendFilterItems(filters, "None", noneFiles, repoRoot);
            filters.AppendLine(@"</Project>");
            File.WriteAllText(projectPath + ".filters", filters.ToString());

            string tasks = $@"{{
  ""version"": ""0.2.1"",
  ""tasks"": [
    {{
      ""taskLabel"": ""Aether Build"",
      ""appliesTo"": ""*"",
      ""type"": ""launch"",
      ""contextType"": ""build"",
      ""command"": ""${{workspaceRoot}}\\Scripts\\Batch\\AetherBuild.bat"",
      ""workingDirectory"": ""${{workspaceRoot}}"",
      ""launchOption"": ""None""
    }},
    {{
      ""taskLabel"": ""Aether Clean"",
      ""appliesTo"": ""*"",
      ""type"": ""launch"",
      ""contextType"": ""clean"",
      ""command"": ""${{workspaceRoot}}\\bin\\Aether\\Aether.exe"",
      ""args"": [ ""clearcache"" ],
      ""workingDirectory"": ""${{workspaceRoot}}""
    }}
  ]
}}
";
            File.WriteAllText(Path.Combine(repoRoot, "tasks.vs.json"), tasks);

            string launch = $@"{{
  ""version"": ""0.2.1"",
  ""configurations"": [
    {{
      ""type"": ""default"",
      ""name"": ""{targetName} (Aether/Ninja)"",
      ""project"": ""${{workspaceRoot}}\\{targetName}.vcxproj"",
      ""program"": ""${{workspaceRoot}}\\Bin\\{targetName}\\{presetConfig}\\{targetName}.exe"",
      ""currentDir"": ""${{workspaceRoot}}\\Bin\\{targetName}\\{presetConfig}"",
      ""cwd"": ""${{workspaceRoot}}\\Bin\\{targetName}\\{presetConfig}"",
      ""args"": []
    }}
  ]
}}
";
            File.WriteAllText(Path.Combine(repoRoot, "launch.vs.json"), launch);

            string solutionPath = Path.Combine(repoRoot, $"{targetName}.sln");
            var solution = new System.Text.StringBuilder();
            solution.AppendLine("Microsoft Visual Studio Solution File, Format Version 12.00");
            solution.AppendLine("# Visual Studio 18");
            solution.AppendLine($"Project(\"{{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}}\") = \"{targetName}\", \"{targetName}.vcxproj\", \"{projectGuid}\"");
            solution.AppendLine("EndProject");

            string toolProject = Path.Combine(repoRoot, "Tools", "Aether", "Aether.csproj");
            string toolGuid = CreateProjectGuid("AetherTool");
            if (File.Exists(toolProject))
            {
                solution.AppendLine($"Project(\"{{FAE04EC0-301F-11D3-BF4B-00C04F79EFBC}}\") = \"Aether\", \"Tools\\\\Aether\\\\Aether.csproj\", \"{toolGuid}\"");
                solution.AppendLine("EndProject");
            }

            solution.AppendLine("Global");
            solution.AppendLine("\tGlobalSection(SolutionConfigurationPlatforms) = preSolution");
            solution.AppendLine($"\t\t{presetConfig}|x64 = {presetConfig}|x64");
            solution.AppendLine("\tEndGlobalSection");
            solution.AppendLine("\tGlobalSection(ProjectConfigurationPlatforms) = postSolution");
            solution.AppendLine($"\t\t{projectGuid}.{presetConfig}|x64.ActiveCfg = {presetConfig}|x64");
            solution.AppendLine($"\t\t{projectGuid}.{presetConfig}|x64.Build.0 = {presetConfig}|x64");
            if (File.Exists(toolProject))
            {
                solution.AppendLine($"\t\t{toolGuid}.{presetConfig}|x64.ActiveCfg = Release|Any CPU");
                solution.AppendLine($"\t\t{toolGuid}.{presetConfig}|x64.Build.0 = Release|Any CPU");
            }
            solution.AppendLine("\tEndGlobalSection");
            solution.AppendLine("\tGlobalSection(SolutionProperties) = preSolution");
            solution.AppendLine("\t\tHideSolutionNode = FALSE");
            solution.AppendLine("\tEndGlobalSection");
            solution.AppendLine("EndGlobal");
            File.WriteAllText(solutionPath, solution.ToString());

            Console.ForegroundColor = ConsoleColor.Green;
            Console.WriteLine($"[OK] Generated Visual Studio Play integration for '{targetName}'.");
            Console.WriteLine($"[OK] Project: {projectPath}");
            Console.WriteLine($"[OK] Solution: {solutionPath}");
            Console.ResetColor();
            return 0;
        }
        catch (Exception ex)
        {
            PrintError(ex.Message);
            return 1;
        }
    }

    private static bool IsVisualStudioFile(string path)
    {
        return IsCppSource(path) || IsCppHeader(path) ||
               path.EndsWith(".vert", StringComparison.OrdinalIgnoreCase) ||
               path.EndsWith(".frag", StringComparison.OrdinalIgnoreCase) ||
               path.EndsWith(".comp", StringComparison.OrdinalIgnoreCase) ||
               path.EndsWith(".cs", StringComparison.OrdinalIgnoreCase) ||
               path.EndsWith(".json", StringComparison.OrdinalIgnoreCase) ||
               path.EndsWith(".bat", StringComparison.OrdinalIgnoreCase);
    }

    private static bool IsCppSource(string path)
    {
        return path.EndsWith(".cpp", StringComparison.OrdinalIgnoreCase) ||
               path.EndsWith(".c", StringComparison.OrdinalIgnoreCase);
    }

    private static bool IsCppHeader(string path)
    {
        return path.EndsWith(".h", StringComparison.OrdinalIgnoreCase) ||
               path.EndsWith(".hpp", StringComparison.OrdinalIgnoreCase) ||
               path.EndsWith(".inl", StringComparison.OrdinalIgnoreCase);
    }

    private static string GetRelativePath(string root, string path)
    {
        return Path.GetRelativePath(root, path).Replace("/", "\\");
    }

    private static string EscapeXml(string value)
    {
        return System.Security.SecurityElement.Escape(value) ?? string.Empty;
    }

    private static string CreateProjectGuid(string value)
    {
        using var sha = System.Security.Cryptography.SHA256.Create();
        byte[] hash = sha.ComputeHash(System.Text.Encoding.UTF8.GetBytes(value));
        var guidBytes = hash.Take(16).ToArray();
        guidBytes[6] = (byte)((guidBytes[6] & 0x0f) | 0x40);
        guidBytes[8] = (byte)((guidBytes[8] & 0x3f) | 0x80);
        return "{" + new Guid(guidBytes).ToString("D").ToUpperInvariant() + "}";
    }

    private static IEnumerable<string> GetParentFilterFolders(string relativeFolder)
    {
        string folder = relativeFolder.Replace("/", "\\").Trim('\\');
        if (string.IsNullOrWhiteSpace(folder))
            yield break;

        string[] parts = folder.Split('\\', StringSplitOptions.RemoveEmptyEntries);
        for (int i = 1; i <= parts.Length; i++)
            yield return string.Join("\\", parts.Take(i));
    }

    private static string GetVisualStudioIncludeDirectories(string repoRoot)
    {
        var includes = new SortedSet<string>(StringComparer.OrdinalIgnoreCase)
        {
            @"$(ProjectDir)Engine\src\public",
            @"$(ProjectDir)Shared\src\public",
            @"$(ProjectDir).cache"
        };

        string cacheDir = Path.Combine(repoRoot, ".cache");
        if (Directory.Exists(cacheDir))
        {
            foreach (string externalDir in Directory.GetDirectories(cacheDir))
            {
                includes.Add(Path.Combine(externalDir, "include"));
                includes.Add(Path.Combine(externalDir, "src", "include"));
                includes.Add(Path.Combine(externalDir, "build", "include"));
                includes.Add(Path.Combine(externalDir, "single_include"));
                includes.Add(externalDir);
            }

            includes.Add(Path.Combine(cacheDir, "ImGui"));
            includes.Add(Path.Combine(cacheDir, "ImGui", "backends"));
        }

        string? vulkanSdk = Environment.GetEnvironmentVariable("VULKAN_SDK");
        if (!string.IsNullOrWhiteSpace(vulkanSdk))
            includes.Add(Path.Combine(vulkanSdk, "Include"));

        return string.Join(";", includes
            .Where(path => path.StartsWith("$(", StringComparison.OrdinalIgnoreCase) ||
                           Directory.Exists(path))
            .Select(path => path.Replace("/", "\\")));
    }

    private static void AppendProjectItems(System.Text.StringBuilder xml, string itemType, IEnumerable<string> files, string repoRoot)
    {
        var fileList = files.ToList();
        if (fileList.Count == 0)
            return;

        xml.AppendLine(@"  <ItemGroup>");
        foreach (string file in fileList)
        {
            xml.AppendLine($@"    <{itemType} Include=""{EscapeXml(GetRelativePath(repoRoot, file))}"" />");
        }
        xml.AppendLine(@"  </ItemGroup>");
    }

    private static void AppendFilterItems(System.Text.StringBuilder xml, string itemType, IEnumerable<string> files, string repoRoot)
    {
        var fileList = files.ToList();
        if (fileList.Count == 0)
            return;

        xml.AppendLine(@"  <ItemGroup>");
        foreach (string file in fileList)
        {
            string relative = GetRelativePath(repoRoot, file);
            string? folder = Path.GetDirectoryName(relative);
            xml.AppendLine($@"    <{itemType} Include=""{EscapeXml(relative)}"">");
            if (!string.IsNullOrWhiteSpace(folder))
                xml.AppendLine($@"      <Filter>{EscapeXml(folder.Replace("/", "\\"))}</Filter>");
            xml.AppendLine(@"    </" + itemType + @">");
        }
        xml.AppendLine(@"  </ItemGroup>");
    }

    private static void GenerateCompilationDatabase(string ninjaPath, string ninjaFile, string repoRoot)
    {
        try
        {
            var psi = new ProcessStartInfo
            {
                FileName = ninjaPath,
                Arguments = $"-f \"{ninjaFile}\" -t compdb compile",
                RedirectStandardOutput = true,
                UseShellExecute = false,
                WorkingDirectory = repoRoot
            };

            using var proc = Process.Start(psi);
            if (proc != null)
            {
                string json = proc.StandardOutput.ReadToEnd();
                proc.WaitForExit();
                File.WriteAllText(Path.Combine(repoRoot, "compile_commands.json"), json);
                Console.WriteLine("[Aether] compile_commands.json updated.");
            }
        }
        catch
        {
            // Ignore compdb failure
        }
    }

    private static void PrintError(string message)
    {
        Console.ForegroundColor = ConsoleColor.Red;
        Console.WriteLine($"\n[Aether] Error: {message}");
        Console.ResetColor();
    }

    private static void PrintUsage()
    {
        Console.WriteLine("\nUsage:");
        Console.WriteLine("  aether build <target> <preset>   Build a target");
        Console.WriteLine("  aether clearcache                Clear .cache intermediate folder");
        Console.WriteLine("  aether get-deps                  Sync external dependencies");
        Console.WriteLine("  aether generate-vs [target] [preset]  Generate Visual Studio 2026 Play integration");
    }

    /// <summary>
    /// Walk up from <paramref name="startDir"/> looking for the Aetherion repository root.
    /// Returns the first directory that contains the repository markers, or null if the drive root is reached.
    /// </summary>
    private static string? FindRepoRoot(string startDir)
    {
        string? dir = Path.GetFullPath(startDir);
        while (dir != null)
        {
            if (File.Exists(Path.Combine(dir, "AetherPresets.json")) &&
                Directory.Exists(Path.Combine(dir, "Engine")) &&
                Directory.Exists(Path.Combine(dir, "Shared")))
            {
                return dir;
            }
            dir = Directory.GetParent(dir)?.FullName;
        }
        return null;
    }
}
