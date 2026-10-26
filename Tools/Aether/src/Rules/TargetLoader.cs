using System.Reflection;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;

namespace Aether.Rules;

public static class TargetLoader
{
    public static TargetRules ResolveTarget(string workspaceRoot, string targetName)
    {
        // Find and instantiate target
        var targetFiles = Directory.GetFiles(workspaceRoot, $"{targetName}.Target.cs", SearchOption.AllDirectories)
            .Where(f => !f.Contains(".cache") && !f.Contains("obj") && !f.Contains("bin") && !f.Contains("Bin"))
            .ToList();

        if (targetFiles.Count == 0)
        {
            throw new FileNotFoundException($"[Aether] Couldn't find file with name {targetName}.Target.cs.");
        }

        string targetFilePath = targetFiles[0];
        Console.WriteLine($"[Aether] Target found: {targetFilePath}");

        var targetAssembly = CompileScripts(new[] { targetFilePath }, $"Aether_Target_{targetName}");
        var targetType = targetAssembly.GetTypes().FirstOrDefault(t => typeof(TargetRules).IsAssignableFrom(t) && !t.IsAbstract) ?? throw new InvalidOperationException($"Target rules not found at {targetFilePath}");

        var target = (TargetRules)Activator.CreateInstance(targetType)!;
        Console.WriteLine($"[Aether] Target: {target.Name} | Required Modules: [{string.Join(" , ", target.Modules)}]");

        // Find target modules
        var moduleScriptPaths = new List<string>();

        foreach (var moduleName in target.Modules)
        {
            var found = Directory.GetFiles(workspaceRoot, $"{moduleName}.Module.cs", SearchOption.AllDirectories)
                .Where(f => !f.Contains(".cache") && !f.Contains("obj") && !f.Contains("bin") && !f.Contains("Bin"))
                .ToList();
            
            if(found.Count == 0)
                throw new FileNotFoundException($"[Aether] Module {moduleName} is not found.");

            Console.WriteLine($"[Aether] Module localized: {found[0]}");
            moduleScriptPaths.Add(found[0]);
        }

        if (moduleScriptPaths.Count == 0) return target;

        var modulesAssembly = CompileScripts(moduleScriptPaths, $"Aether_Modules_{targetName}");

        foreach (var type in modulesAssembly.GetTypes().Where(t => typeof(ModuleRules).IsAssignableFrom(t) && !t.IsAbstract))
        {
            var ctor = type.GetConstructor(new[] { typeof(string) });
            var moduleInstance = ctor != null 
                ? (ModuleRules)ctor.Invoke(new object[] { workspaceRoot })
                : (ModuleRules)Activator.CreateInstance(type)!;

            target.ResolvedModules.Add(moduleInstance);
        }

        return target;
    }

    private static Assembly CompileScripts(IEnumerable<string> filePaths, string assemblyName)
    {
        var syntaxTrees = filePaths.Select(f => CSharpSyntaxTree.ParseText(File.ReadAllText(f), path: f)).ToList();

        var coreDir = Path.GetDirectoryName(typeof(object).Assembly.Location);
        var references = new List<MetadataReference>
        {
            MetadataReference.CreateFromFile(typeof(object).Assembly.Location),
            MetadataReference.CreateFromFile(typeof(TargetRules).Assembly.Location),
            MetadataReference.CreateFromFile(Path.Combine(coreDir, "System.Runtime.dll")),
            MetadataReference.CreateFromFile(Path.Combine(coreDir, "System.Collections.dll")),
            MetadataReference.CreateFromFile(Path.Combine(coreDir, "System.IO.dll")),
            MetadataReference.CreateFromFile(Path.Combine(coreDir, "System.Linq.dll"))
        };

        var compilation = CSharpCompilation.Create(assemblyName, syntaxTrees, references, new CSharpCompilationOptions(OutputKind.DynamicallyLinkedLibrary));
        using var ms = new MemoryStream();
        var result = compilation.Emit(ms);

        if (!result.Success)
        {
            Console.ForegroundColor = ConsoleColor.Red;
            Console.WriteLine($"[Aether] ERROR: Error compiling C# Scripts:");
            foreach (var diag in result.Diagnostics.Where(d => d.Severity == DiagnosticSeverity.Error))
                Console.WriteLine($" -> {diag.GetMessage()}");
            Console.ResetColor();
            throw new Exception("Failed compiling C# Rules");
        }

        ms.Seek(0, SeekOrigin.Begin);
        return Assembly.Load(ms.ToArray());
    }
}