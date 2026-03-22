using System.IO;
using System.Text.Json;

namespace Z_Wave_UI.Services;

internal sealed record ConnectionSettings(string Host, int Port, bool AutoConnect);

internal static class ConnectionSettingsStore
{
    private static readonly JsonSerializerOptions jsonOptions = new()
    {
        WriteIndented = true
    };

    public static ConnectionSettings Load(string defaulthost, int defaultport)
    {
        try
        {
            var path = GetSettingsPath();
            if (!File.Exists(path))
                return new ConnectionSettings(defaulthost, defaultport, AutoConnect: false);

            var json = File.ReadAllText(path);
            var settings = JsonSerializer.Deserialize<ConnectionSettings>(json, jsonOptions);
            if (settings is null)
                return new ConnectionSettings(defaulthost, defaultport, AutoConnect: false);

            var host = string.IsNullOrWhiteSpace(settings.Host) ? defaulthost : settings.Host;
            var port = settings.Port is > 0 and <= 65535 ? settings.Port : defaultport;

            return settings with { Host = host, Port = port };
        }
        catch
        {
            return new ConnectionSettings(defaulthost, defaultport, AutoConnect: false);
        }
    }

    public static void Save(ConnectionSettings settings)
    {
        try
        {
            var path = GetSettingsPath();
            Directory.CreateDirectory(Path.GetDirectoryName(path)!);

            var json = JsonSerializer.Serialize(settings, jsonOptions);
            File.WriteAllText(path, json);
        }
        catch
        {
        }
    }

    private static string GetSettingsPath()
    {
        var dir = Path.Combine(
            Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData),
            "Z_Wave_UI");

        return Path.Combine(dir, "settings.json");
    }
}
