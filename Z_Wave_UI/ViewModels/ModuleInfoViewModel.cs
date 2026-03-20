using System.Collections.ObjectModel;
using System.Text.Json;
using System.Windows.Threading;

namespace Z_Wave_UI.ViewModels;

public sealed class ModuleInfoViewModel : ViewModelBase
{
    private readonly Dispatcher dispatcher;
    private readonly Action<string> setStatusMessage;

    private string initializationState = string.Empty;
    private int apiVersion;
    private int apiCapabilities;
    private string apiCapabilitiesDescription = string.Empty;

    private string libraryVersion = string.Empty;
    private int libraryType;
    private string libraryTypeName = string.Empty;

    private int appVersion;
    private int appRevision;

    private uint homeId;
    private int nodeId;

    public ModuleInfoViewModel(Dispatcher dispatcher, Action<string> setStatusMessage)
    {
        this.dispatcher = dispatcher;
        this.setStatusMessage = setStatusMessage;
    }

    public string InitializationState
    {
        get => initializationState;
        private set => SetProperty(ref initializationState, value);
    }

    public int ApiVersion
    {
        get => apiVersion;
        private set => SetProperty(ref apiVersion, value);
    }

    public int ApiCapabilities
    {
        get => apiCapabilities;
        private set => SetProperty(ref apiCapabilities, value);
    }

    public string ApiCapabilitiesDescription
    {
        get => apiCapabilitiesDescription;
        private set => SetProperty(ref apiCapabilitiesDescription, value);
    }

    public string LibraryVersion
    {
        get => libraryVersion;
        private set => SetProperty(ref libraryVersion, value);
    }

    public int LibraryType
    {
        get => libraryType;
        private set => SetProperty(ref libraryType, value);
    }

    public string LibraryTypeName
    {
        get => libraryTypeName;
        private set => SetProperty(ref libraryTypeName, value);
    }

    public int AppVersion
    {
        get => appVersion;
        private set => SetProperty(ref appVersion, value);
    }

    public int AppRevision
    {
        get => appRevision;
        private set => SetProperty(ref appRevision, value);
    }

    public uint HomeId
    {
        get => homeId;
        private set
        {
            if (SetProperty(ref homeId, value))
                OnPropertyChanged(nameof(HomeIdHex));
        }
    }

    public string HomeIdHex => $"0x{HomeId:X8}";

    public int NodeId
    {
        get => nodeId;
        private set => SetProperty(ref nodeId, value);
    }

    public ObservableCollection<int> NodeIds { get; } = new();

    public ObservableCollection<int> ApiCommands { get; } = new();

    public ObservableCollection<string> ControllerFlags { get; } = new();

    public ObservableCollection<string> Chip { get; } = new();

    public ObservableCollection<string> ProtocolVersion { get; } = new();

    public ObservableCollection<string> Manufacturer { get; } = new();

    public void Clear()
    {
        if (dispatcher.CheckAccess())
            ClearCore();
        else
            _ = dispatcher.InvokeAsync(ClearCore);
    }

    public void HandleLine(string line, Func<Task> refreshmoduleinfo)
    {
        try
        {
            using var doc = JsonDocument.Parse(line);
            if (!doc.RootElement.TryGetProperty("type", out var typeElement))
                return;

            var type = typeElement.GetString();
            if (string.Equals(type, "controller_info", StringComparison.OrdinalIgnoreCase))
            {
                var info = ParseControllerInfo(doc.RootElement);
                if (info is null)
                    return;

                _ = dispatcher.InvokeAsync(() =>
                {
                    ApplyControllerInfo(info);
                    setStatusMessage("Module info updated");
                });

                return;
            }

            if (string.Equals(type, "controller_changed", StringComparison.OrdinalIgnoreCase))
            {
                _ = refreshmoduleinfo();
                return;
            }
        }
        catch
        {
        }
    }

    private void ClearCore()
    {
        InitializationState = string.Empty;
        ApiVersion = 0;
        ApiCapabilities = 0;
        ApiCapabilitiesDescription = string.Empty;
        LibraryVersion = string.Empty;
        LibraryType = 0;
        LibraryTypeName = string.Empty;
        AppVersion = 0;
        AppRevision = 0;
        HomeId = 0;
        NodeId = 0;

        NodeIds.Clear();
        ApiCommands.Clear();
        ControllerFlags.Clear();
        Chip.Clear();
        ProtocolVersion.Clear();
        Manufacturer.Clear();
    }

    private sealed record ControllerInfo(
        string InitializationState,
        int ApiVersion,
        int ApiCapabilities,
        string ApiCapabilitiesDescription,
        IReadOnlyList<string> ControllerFlags,
        IReadOnlyList<string> Chip,
        IReadOnlyList<string> ProtocolVersion,
        IReadOnlyList<string> Manufacturer,
        string LibraryVersion,
        int LibraryType,
        string LibraryTypeName,
        int AppVersion,
        int AppRevision,
        uint HomeId,
        int NodeId,
        IReadOnlyList<int> NodeIds,
        IReadOnlyList<int> ApiCommands);

    private static ControllerInfo? ParseControllerInfo(JsonElement root)
    {
        var initializationState = string.Empty;
        if (root.TryGetProperty("initialization", out var initializationElement) && initializationElement.ValueKind == JsonValueKind.Object)
        {
            initializationState = GetString(initializationElement, "state");
        }

        var apiVersion = 0;
        var apiCapabilities = 0;
        var apiCapabilitiesDescription = string.Empty;
        if (root.TryGetProperty("api", out var apiElement) && apiElement.ValueKind == JsonValueKind.Object)
        {
            apiVersion = GetInt(apiElement, "version");
            apiCapabilities = GetInt(apiElement, "capabilities");
            apiCapabilitiesDescription = GetString(apiElement, "capabilitiesDescription");
        }

        var controllerFlags = GetStringArray(root, "controllerFlags");
        var chip = GetStringArray(root, "chip");
        var protocolVersion = GetStringArray(root, "protocolVersion");
        var manufacturer = GetStringArray(root, "manufacturer");

        var libraryVersion = string.Empty;
        var libraryType = 0;
        var libraryTypeName = string.Empty;
        if (root.TryGetProperty("library", out var libraryElement) && libraryElement.ValueKind == JsonValueKind.Object)
        {
            libraryVersion = GetString(libraryElement, "version");
            libraryType = GetInt(libraryElement, "type");
            libraryTypeName = GetString(libraryElement, "typeName");
        }

        var appVersion = 0;
        var appRevision = 0;
        if (root.TryGetProperty("application", out var applicationElement) && applicationElement.ValueKind == JsonValueKind.Object)
        {
            appVersion = GetInt(applicationElement, "version");
            appRevision = GetInt(applicationElement, "revision");
        }

        var homeId = 0u;
        var nodeId = 0;
        if (root.TryGetProperty("network", out var networkElement) && networkElement.ValueKind == JsonValueKind.Object)
        {
            homeId = GetUInt(networkElement, "homeId");
            nodeId = GetInt(networkElement, "nodeId");
        }

        var nodeIds = GetIntArray(root, "nodeIds");
        var apiCommands = GetIntArray(root, "apiCommands");

        return new ControllerInfo(
            initializationState,
            apiVersion,
            apiCapabilities,
            apiCapabilitiesDescription,
            controllerFlags,
            chip,
            protocolVersion,
            manufacturer,
            libraryVersion,
            libraryType,
            libraryTypeName,
            appVersion,
            appRevision,
            homeId,
            nodeId,
            nodeIds,
            apiCommands);
    }

    private void ApplyControllerInfo(ControllerInfo info)
    {
        InitializationState = info.InitializationState;
        ApiVersion = info.ApiVersion;
        ApiCapabilities = info.ApiCapabilities;
        ApiCapabilitiesDescription = info.ApiCapabilitiesDescription;

        LibraryVersion = info.LibraryVersion;
        LibraryType = info.LibraryType;
        LibraryTypeName = info.LibraryTypeName;

        AppVersion = info.AppVersion;
        AppRevision = info.AppRevision;

        HomeId = info.HomeId;
        NodeId = info.NodeId;

        ReplaceAll(ControllerFlags, info.ControllerFlags);
        ReplaceAll(Chip, info.Chip);
        ReplaceAll(ProtocolVersion, info.ProtocolVersion);
        ReplaceAll(Manufacturer, info.Manufacturer);
        ReplaceAll(NodeIds, info.NodeIds);
        ReplaceAll(ApiCommands, info.ApiCommands);
    }

    private static void ReplaceAll<T>(ObservableCollection<T> target, IReadOnlyList<T> items)
    {
        target.Clear();
        foreach (var item in items)
            target.Add(item);
    }

    private static string GetString(JsonElement obj, string propertyname)
    {
        if (!obj.TryGetProperty(propertyname, out var element) || element.ValueKind != JsonValueKind.String)
            return string.Empty;

        return element.GetString() ?? string.Empty;
    }

    private static int GetInt(JsonElement obj, string propertyname)
    {
        if (!obj.TryGetProperty(propertyname, out var element) || element.ValueKind != JsonValueKind.Number)
            return 0;

        return element.TryGetInt32(out var value) ? value : 0;
    }

    private static uint GetUInt(JsonElement obj, string propertyname)
    {
        if (!obj.TryGetProperty(propertyname, out var element) || element.ValueKind != JsonValueKind.Number)
            return 0;

        return element.TryGetUInt32(out var value) ? value : 0;
    }

    private static IReadOnlyList<string> GetStringArray(JsonElement obj, string propertyname)
    {
        if (!obj.TryGetProperty(propertyname, out var element) || element.ValueKind != JsonValueKind.Array)
            return Array.Empty<string>();

        var items = new List<string>();
        foreach (var item in element.EnumerateArray())
        {
            if (item.ValueKind == JsonValueKind.String)
                items.Add(item.GetString() ?? string.Empty);
        }

        return items;
    }

    private static IReadOnlyList<int> GetIntArray(JsonElement obj, string propertyname)
    {
        if (!obj.TryGetProperty(propertyname, out var element) || element.ValueKind != JsonValueKind.Array)
            return Array.Empty<int>();

        var items = new List<int>();
        foreach (var item in element.EnumerateArray())
        {
            if (item.ValueKind != JsonValueKind.Number)
                continue;

            if (item.TryGetInt32(out var value))
                items.Add(value);
        }

        return items;
    }
}
