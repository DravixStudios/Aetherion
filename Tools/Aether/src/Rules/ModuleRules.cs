namespace Aether.Rules;

public abstract class ModuleRules
{
    public string Name { get; set; } = string.Empty;
    public List<string> IncludePaths { get; } = new();
    public List<string> SourceFiles { get; } = new();
    public List<string> Defines { get; } = new();
    public List<string> Libraries { get; } = new();
    public List<string> LibraryPaths { get; } = new();
    public List<ExternalRules> Externals { get; } = new();
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
