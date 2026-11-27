namespace Aether.Rules;

public interface IExtendedBuild
{
    public abstract void PreBuild();
    public abstract void PostBuild();
}

public abstract class ExternalRules
{
    public string Name { get; set; } = string.Empty;
    public string Repo { get; set; } = string.Empty;
    public string Branch { get; set; } = string.Empty;

    public bool BuildWithCMake { get; set; } = false;
    public string CMakeArguments { get; set; } = string.Empty;
    public List<string> SourceFiles { get; set; } = new();
    public List<string> IncludePaths { get; set; } = new();
    public List<string> Defines { get; set; } = new();
    public List<string> Libraries { get; set; } = new();
    public List<string> LibraryPaths { get; set; } = new();
}
