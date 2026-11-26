namespace Aether.Rules;

public abstract class TargetRules
{
    public string Name { get; set; } = string.Empty;
    public string OutputExecutable { get; set; } = string.Empty;
    public string Configuration { get; set; } = "Debug";
    public string Platform { get; set; } = "Windows";
    public string Architecture { get; set; } = "x64";

    public List<string> SourceFiles { get; } = new();
    public List<string> IncludePaths { get; } = new();
    public List<string> Defines { get; } = new();
    public List<string> Libraries { get; } = new();
    public List<string> LibraryPaths { get; } = new();
    public List<string> Modules { get; } = new();
    public List<ExternalRules> Externals { get; } = new();
    public List<ModuleRules> ResolvedModules { get; } = new();
    public bool EnableJumbo { get; set; } = false;
    public bool ENABLE_JUMBO
    {
        get => EnableJumbo;
        set => EnableJumbo = value;
    }
    public int JumboBatchSize { get; set; } = 8;
    public List<string> JumboExcludes { get; } = new();
    internal HashSet<string> ExternalSourceFiles { get; } = new(StringComparer.OrdinalIgnoreCase);
}
