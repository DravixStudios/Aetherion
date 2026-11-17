namespace Aether.Rules;

public abstract class ExternalRules
{
    public string Name { get; set; } = string.Empty;
    public string Repo { get; set; } = string.Empty;
    public bool BuildWithCMake { get; set; } = false;
    public List<string> SourceFiles { get; set; } = new();
    public List<string> IncludePaths { get; set; } = new();
    public List<string> Defines { get; set; } = new();
    public List<string> Libraries { get; set; } = new();
    public List<string> LibraryPaths { get; set; } = new();
}
