namespace Z_Wave_UI.Models;

public sealed class LogEntryModel
{
    public DateTimeOffset Time { get; init; }
    public string Level { get; init; } = string.Empty;
    public string Tag { get; init; } = string.Empty;
    public string Message { get; init; } = string.Empty;

    public string DisplayTime => Time.ToLocalTime().ToString("HH:mm:ss");
}
