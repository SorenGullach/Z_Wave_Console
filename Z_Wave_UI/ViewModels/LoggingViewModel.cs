using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Text.Json;
using System.Windows;
using System.Windows.Data;
using System.Windows.Threading;

using Z_Wave_UI.Commands;
using Z_Wave_UI.Models;

namespace Z_Wave_UI.ViewModels;

public sealed class LoggingViewModel : ViewModelBase
{
    private readonly Dispatcher dispatcher;
    private readonly Action<string> setStatusMessage;

    private readonly Dictionary<string, LevelFilterItem> levelFilterMap = new(StringComparer.OrdinalIgnoreCase);

    private readonly RelayCommand copyLogsCommand;

    public LoggingViewModel(Dispatcher dispatcher, Action<string> setStatusMessage)
    {
        this.dispatcher = dispatcher;
        this.setStatusMessage = setStatusMessage;

        copyLogsCommand = new RelayCommand(CopyLogsToClipboard, () => Logs.Count > 0);
        Logs.CollectionChanged += (_, _) => copyLogsCommand.RaiseCanExecuteChanged();

        LogsView = CollectionViewSource.GetDefaultView(Logs);
        LogsView.Filter = FilterLogEntry;
    }

    public ObservableCollection<LogEntryModel> Logs { get; } = new();

    public ICollectionView LogsView { get; }

    public ObservableCollection<LevelFilterItem> LevelFilters { get; } = new();

    public RelayCommand CopyLogsCommand => copyLogsCommand;

    public void Clear()
    {
        if (dispatcher.CheckAccess())
            ClearCore();
        else
            _ = dispatcher.InvokeAsync(ClearCore);
    }

    public void HandleLine(string line, Func<Task> refreshlogs)
    {
        try
        {
            using var doc = JsonDocument.Parse(line);
            if (!doc.RootElement.TryGetProperty("type", out var typeElement))
                return;

            var type = typeElement.GetString();
            if (string.Equals(type, "logs", StringComparison.OrdinalIgnoreCase))
            {
                if (!doc.RootElement.TryGetProperty("entries", out var entriesElement) || entriesElement.ValueKind != JsonValueKind.Array)
                    return;

                var entries = new List<LogEntryModel>();
                foreach (var entry in entriesElement.EnumerateArray())
                {
                    if (entry.ValueKind != JsonValueKind.Object)
                        continue;

                    long time = 0;
                    if (entry.TryGetProperty("time", out var timeElement) && timeElement.ValueKind == JsonValueKind.Number)
                        timeElement.TryGetInt64(out time);

                    var level = entry.TryGetProperty("level", out var levelElement) ? levelElement.GetString() ?? string.Empty : string.Empty;
                    var tag = entry.TryGetProperty("tag", out var tagElement) ? tagElement.GetString() ?? string.Empty : string.Empty;
                    var msg = entry.TryGetProperty("msg", out var msgElement) ? msgElement.GetString() ?? string.Empty : string.Empty;

                    entries.Add(new LogEntryModel
                    {
                        Time = DateTimeOffset.FromUnixTimeSeconds(time),
                        Level = level,
                        Tag = tag,
                        Message = msg,
                    });
                }

                var levels = entries
                    .Select(e => e.Level)
                    .Where(l => !string.IsNullOrWhiteSpace(l))
                    .Distinct(StringComparer.OrdinalIgnoreCase)
                    .OrderBy(l => l, StringComparer.OrdinalIgnoreCase)
                    .ToList();

                _ = dispatcher.InvokeAsync(() =>
                {
                    Logs.Clear();
                    foreach (var entry in entries)
                        Logs.Add(entry);

                    UpdateLevelFilters(levels);
                    LogsView.Refresh();

                    copyLogsCommand.RaiseCanExecuteChanged();

                    setStatusMessage($"Loaded {entries.Count} log entries");
                });

                return;
            }

            if (string.Equals(type, "log_changed", StringComparison.OrdinalIgnoreCase))
            {
                _ = refreshlogs();
                return;
            }
        }
        catch
        {
        }
    }

    private bool FilterLogEntry(object obj)
    {
        if (obj is not LogEntryModel entry)
            return false;

        if (levelFilterMap.Count == 0)
            return true;

        if (!levelFilterMap.TryGetValue(entry.Level, out var filter))
            return true;

        return filter.IsEnabled;
    }

    private void ClearCore()
    {
        Logs.Clear();
        foreach (var filter in LevelFilters)
            filter.PropertyChanged -= LevelFilterItem_PropertyChanged;

        LevelFilters.Clear();
        levelFilterMap.Clear();
        LogsView.Refresh();

        copyLogsCommand.RaiseCanExecuteChanged();
    }

    private void CopyLogsToClipboard()
    {
        if (Logs.Count == 0)
            return;

        var text = string.Join(Environment.NewLine, Logs.Select(e => $"{e.DisplayTime}\t{e.Level}\t{e.Tag}\t{e.Message}"));

        try
        {
            Clipboard.SetText(text);
            setStatusMessage($"Copied {Logs.Count} log entries");
        }
        catch (Exception ex)
        {
            setStatusMessage(ex.Message);
        }
    }

    private void UpdateLevelFilters(IReadOnlyList<string> levels)
    {
        var previousState = LevelFilters.ToDictionary(f => f.Level, f => f.IsEnabled, StringComparer.OrdinalIgnoreCase);

        foreach (var filter in LevelFilters)
            filter.PropertyChanged -= LevelFilterItem_PropertyChanged;

        LevelFilters.Clear();
        levelFilterMap.Clear();

        foreach (var level in levels)
        {
            var isEnabled = !previousState.TryGetValue(level, out var prevEnabled) || prevEnabled;
            var filter = new LevelFilterItem(level, isEnabled);
            filter.PropertyChanged += LevelFilterItem_PropertyChanged;
            LevelFilters.Add(filter);
            levelFilterMap[level] = filter;
        }
    }

    private void LevelFilterItem_PropertyChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (e.PropertyName == nameof(LevelFilterItem.IsEnabled))
            LogsView.Refresh();
    }

    public sealed class LevelFilterItem : ViewModelBase
    {
        private bool isEnabled;

        public LevelFilterItem(string level, bool isenabled)
        {
            Level = level;
            isEnabled = isenabled;
        }

        public string Level { get; }

        public bool IsEnabled
        {
            get => isEnabled;
            set => SetProperty(ref isEnabled, value);
        }
    }
}
