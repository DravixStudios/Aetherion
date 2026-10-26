namespace Aether.Rules;

public abstract class TargetRules
{
    public string Name { get; set; } = string.Empty;
    public string OutputExecutable { get; set; } = string.Empty;
    public List<ModuleRules> Modules { get; } = new();
    public List<ModuleRules> ResolvedModules { get; } = new();
}