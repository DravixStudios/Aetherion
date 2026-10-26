namespace Aether.Rules;

public abstract class ModuleRules
{
    public string Name { get; set; } = string.Empty;
    public List<string> IncludePaths { get; } = new();
    public List<string> SourceFiles { get; } = new();
}