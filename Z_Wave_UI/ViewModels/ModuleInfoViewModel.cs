using System.Collections.ObjectModel;
using System.Net.Sockets;
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

    private float appVersion;

    private uint homeId;
    private int nodeId;

    private int chipType;
    private string chipName = string.Empty;
    private int chipVersion;

    private int manufacturerId;
    private string manufacturerName = string.Empty;
    private int productType;
    private string productTypeName = string.Empty;
    private int productId;
    private string productIdName = string.Empty;

    private int protocolType;
    private string protocolVersion = string.Empty;

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

    public float AppVersion
    {
        get => appVersion;
        private set => SetProperty(ref appVersion, value);
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

    public int ChipType
    {
        get => chipType;
        private set => SetProperty(ref chipType, value);
    }

    public string ChipName
    {
        get => chipName;
        private set => SetProperty(ref chipName, value);
    }

    public int ChipVersion
    {
        get => chipVersion;
        private set => SetProperty(ref chipVersion, value);
    }

    public int ManufacturerId
    {
        get => manufacturerId;
        private set => SetProperty(ref manufacturerId, value);
    }

    public string ManufacturerName
    {
        get => manufacturerName;
        private set => SetProperty(ref manufacturerName, value);
    }

    public int ProductType
    {
        get => productType;
        private set => SetProperty(ref productType, value);
    }

    public string ProductTypeName
    {
        get => productTypeName;
        private set => SetProperty(ref productTypeName, value);
    }

    public int ProductId
    {
        get => productId;
        private set => SetProperty(ref productId, value);
    }

    public string ProductIdName
    {
        get => productIdName;
        private set => SetProperty(ref productIdName, value);
    }

    public int ProtocolType
    { get => protocolType; private set => SetProperty(ref protocolType, value); }
    public string ProtocolVersion
    {
        get => protocolVersion;
        private set => SetProperty(ref protocolVersion, value);
    }

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
        HomeId = 0;
        NodeId = 0;
        ProtocolType = 0;
        ProtocolVersion = string.Empty;
        ChipType = 0;
        ChipName = string.Empty;
        ChipVersion = 0;
        ManufacturerId = 0;
        ManufacturerName = string.Empty;
        ProductType = 0;
        ProductTypeName = string.Empty;
        ProductId = 0;
        ProductIdName = string.Empty;

        NodeIds.Clear();
        ApiCommands.Clear();
        ControllerFlags.Clear();
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
        float AppVersion,
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

        float appVersion = 0;
        if (root.TryGetProperty("application", out var applicationElement) && applicationElement.ValueKind == JsonValueKind.Object)
        {
            int Version = GetInt(applicationElement, "version");
            int Revision = GetInt(applicationElement, "revision");
            if(Revision<10)
                appVersion = Version + (Revision / 10f);
            else
                appVersion = Version + (Revision / 100f);
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

        HomeId = info.HomeId;
        NodeId = info.NodeId;

        ReplaceAll(ControllerFlags, info.ControllerFlags);

        var chipTypeText = TryGetValue(info.Chip, "type");
        ChipType = int.TryParse(chipTypeText, out var chipTypeValue) ? chipTypeValue : 0;
        ChipName = TryGetValue(info.Chip, "typeName") ?? string.Empty;
        var chipVersionText = TryGetValue(info.Chip, "version");
        ChipVersion = int.TryParse(chipVersionText, out var chipVersionValue) ? chipVersionValue : 0;

        var protocolTypeText = TryGetValue(info.ProtocolVersion, "type");
        ProtocolType = int.TryParse(protocolTypeText, out var protocolType) ? protocolType : 0;
        ProtocolVersion = BuildProtocolVersionDisplay(info.ProtocolVersion);

        var manufacturerIdText = TryGetValue(info.Manufacturer, "id");
        ManufacturerId = int.TryParse(manufacturerIdText, out var manufacturerIdValue) ? manufacturerIdValue : 0;
        ManufacturerName = TryGetValue(info.Manufacturer, "idName") ?? string.Empty;

        var productTypeText = TryGetValue(info.Manufacturer, "productType");
        ProductType = int.TryParse(productTypeText, out var productTypeValue) ? productTypeValue : 0;
        ProductTypeName = TryGetValue(info.Manufacturer, "productTypeName") ?? string.Empty;

        var productIdText = TryGetValue(info.Manufacturer, "productId");
        ProductId = int.TryParse(productIdText, out var productIdValue) ? productIdValue : 0;
        ProductIdName = TryGetValue(info.Manufacturer, "productIdName") ?? string.Empty;

        ReplaceAll(NodeIds, info.NodeIds);
        ReplaceAll(ApiCommands, info.ApiCommands);
    }

    private static string BuildProtocolVersionDisplay(IReadOnlyList<string> protocolVersion)
    {
        var major = TryGetValue(protocolVersion, "major");
        var minor = TryGetValue(protocolVersion, "minor");
        var revision = TryGetValue(protocolVersion, "revision");

        if (major is null || minor is null || revision is null)
            return string.Empty;

        return $"{major}.{minor}.{revision}";
    }

    private static string? TryGetValue(IReadOnlyList<string> items, string key)
    {
        foreach (var item in items)
        {
            if (string.IsNullOrWhiteSpace(item))
                continue;

            var idx = item.IndexOf('=');
            if (idx <= 0)
                continue;

            var k = item[..idx];
            if (!string.Equals(k, key, StringComparison.OrdinalIgnoreCase))
                continue;

            var v = item[(idx + 1)..];
            return v;
        }

        return null;
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
