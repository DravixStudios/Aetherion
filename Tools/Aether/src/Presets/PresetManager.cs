using System.Text.Json;

namespace Aether.Presets;

public class Preset
{
    public string Name { get; set; } = string.Empty;
    public string? Inherits { get; set; }
    public string Platform { get; set; } = "Windows";
    public string Architecture { get; set; } = "x64";
    public string Configuration { get; set; } = "Debug";
    public List<string> Defines { get; set; } = new();
}

public class PresetsContainer
{
    public List<Preset> Presets { get; set; } = new();
}

public static class PresetManager
{
    public static Preset LoadPreset(string workspaceRoot, string presetName)
    {
        string presetsFile = Path.Combine(workspaceRoot, "AetherPresets.json");
        if (!File.Exists(presetsFile))
        {
            throw new FileNotFoundException($"AetherPresets.json not found at {workspaceRoot}");
        }

        string json = File.ReadAllText(presetsFile);
        var container = JsonSerializer.Deserialize<PresetsContainer>(json, new JsonSerializerOptions 
        {
            PropertyNameCaseInsensitive = true
        }) ?? throw new Exception("Failed deserializing AetherPresets.json");

        var map = container.Presets.ToDictionary(p => p.Name, StringComparer.OrdinalIgnoreCase);

        if (!map.TryGetValue(presetName, out var targetPreset))
        {
            Console.WriteLine($"[ERROR] Preset '{presetName}' not found. Available presets:");
            foreach (var p in map.Values.Where(p => !p.Name.EndsWith("-base")))
                Console.WriteLine($" - {p.Name}");
            throw new ArgumentException($"Unknown preset {presetName}");
        }

        var resolved = new Preset
        {
            Name = targetPreset.Name,
            Platform = targetPreset.Platform,
            Architecture = targetPreset.Architecture,
            Configuration = targetPreset.Configuration,
            Defines = new List<string>(targetPreset.Defines)
        };

        string? currentInherit = targetPreset.Inherits;
        while(!string.IsNullOrEmpty(currentInherit) && map.TryGetValue(currentInherit, out var parent))
        {
            if (string.IsNullOrEmpty(resolved.Platform)) resolved.Platform = parent.Platform;
            if (string.IsNullOrEmpty(resolved.Configuration)) resolved.Configuration = parent.Configuration;
            if (string.IsNullOrEmpty(resolved.Architecture)) resolved.Platform = parent.Architecture;

            foreach(var def in parent.Defines)
            {
                if (!resolved.Defines.Any(d => d.Split('=')[0] == def.Split('=')[0]))
                    resolved.Defines.Add(def);
            }

            currentInherit = parent.Inherits;
        }

        return resolved;
    }
}

