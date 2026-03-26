using System.Text.Json;
using System.Windows.Threading;

using Z_Wave_UI.Commands;
using Z_Wave_UI.Services;

namespace Z_Wave_UI.ViewModels;

public sealed class ConnectionViewModel : ViewModelBase
{
    private readonly ZWaveTcpClientService tcpService;
    private readonly Dispatcher dispatcher;
    private readonly Action<string> setStatusMessage;
    private readonly Func<string> getStatusMessage;
    private readonly Action clearLogs;
    private readonly Action clearModuleInfo;

    private string host = "127.0.0.1";
    private int port = 5555;
    private bool autoConnect;
    private bool suppressSettingsSave;

    private readonly AsyncRelayCommand connectCommand;
    private readonly AsyncRelayCommand disconnectCommand;
    private readonly AsyncRelayCommand refreshLogsCommand;

    public ConnectionViewModel(
        ZWaveTcpClientService tcpService,
        Dispatcher dispatcher,
        Action<string> setStatusMessage,
        Func<string> getStatusMessage,
        Action clearLogs,
        Action clearModuleInfo)
    {
        this.tcpService = tcpService;
        this.dispatcher = dispatcher;
        this.setStatusMessage = setStatusMessage;
        this.getStatusMessage = getStatusMessage;
        this.clearLogs = clearLogs;
        this.clearModuleInfo = clearModuleInfo;

        connectCommand = new AsyncRelayCommand(() => ConnectAsync(), () => !IsConnected);
        disconnectCommand = new AsyncRelayCommand(() => DisconnectAsync(), () => IsConnected);
        refreshLogsCommand = new AsyncRelayCommand(() => RefreshLogsAsync(100), () => IsConnected);

        LoadSettings();
    }

    public string Host
    {
        get => host;
        set
        {
            if (SetProperty(ref host, value))
                SaveSettings();
        }
    }

    public int Port
    {
        get => port;
        set
        {
            if (SetProperty(ref port, value))
                SaveSettings();
        }
    }

    public bool IsConnected => tcpService.IsConnected;

    public AsyncRelayCommand ConnectCommand => connectCommand;
    public AsyncRelayCommand DisconnectCommand => disconnectCommand;
    public AsyncRelayCommand RefreshLogsCommand => refreshLogsCommand;

    public Task TryAutoConnectAsync()
    {
        if (!autoConnect)
            return Task.CompletedTask;

        return ConnectAsync();
    }

    public async Task ConnectAsync()
    {
        SetStatusMessage($"Connecting to {Host}:{Port}...");

        try
        {
            await tcpService.ConnectAsync(Host, Port, CancellationToken.None).ConfigureAwait(false);

            await dispatcher.InvokeAsync(() =>
            {
                NotifyConnectionStateChanged();
            });

            SetStatusMessage($"Connected to {Host}:{Port}");

            autoConnect = true;
            SaveSettings();

            await RefreshLogsAsync().ConfigureAwait(false);
            await RefreshModuleInfoAsync().ConfigureAwait(false);
            await RefreshNodeListAsync().ConfigureAwait(false);
        }
        catch (Exception ex)
        {
            SetStatusMessage(ex.Message);
            await DisconnectAsync().ConfigureAwait(false);
        }
    }

    public async Task DisconnectAsync()
    {
        await tcpService.DisconnectAsync().ConfigureAwait(false);

        autoConnect = false;
        SaveSettings();

        await dispatcher.InvokeAsync(() =>
        {
            clearLogs();
            clearModuleInfo();
            SetStatusMessage("Disconnected");
            NotifyConnectionStateChanged();
        });
    }

    public async Task RefreshLogsAsync(int count=10000)
    {
        if (!IsConnected)
            return;

        try
        {
            var payload = new
            {
                type = "get_logs",
                count = count
            };

            var json = JsonSerializer.Serialize(payload);
            await tcpService.SendLineAsync(json, CancellationToken.None).ConfigureAwait(false);
        }
        catch (Exception ex)
        {
            SetStatusMessage(ex.Message);
        }
    }

    public async Task RefreshModuleInfoAsync()
    {
        if (!IsConnected)
            return;

        try
        {
            var payload = new
            {
                type = "get_controller_info",
            };

            var json = JsonSerializer.Serialize(payload);
            await tcpService.SendLineAsync(json, CancellationToken.None).ConfigureAwait(false);
        }
        catch (Exception ex)
        {
            SetStatusMessage(ex.Message);
        }
    }

    public async Task RefreshNodeListAsync()
    {
        if (!IsConnected)
            return;

        try
        {
            var payload = new
            {
                type = "list_nodes",
            };

            var json = JsonSerializer.Serialize(payload);
            await tcpService.SendLineAsync(json, CancellationToken.None).ConfigureAwait(false);
        }
        catch (Exception ex)
        {
            SetStatusMessage(ex.Message);
        }
    }

    public async Task RefreshNodeInfoAsync(int nodeId)
    {
        if (!IsConnected)
            return;

        try
        {
            var payload = new
            {
                type = "get_node",
                node_id = nodeId
            };

            var json = JsonSerializer.Serialize(payload);
            await tcpService.SendLineAsync(json, CancellationToken.None).ConfigureAwait(false);
        }
        catch (Exception ex)
        {
            SetStatusMessage(ex.Message);
        }
    }

    public async Task UpdateConfigAsync(int nodeid, int param, int value, int size)
    {
        if (!IsConnected)
            return;

        try
        {
            var payload = new
            {
                type = "set_config",
                node_id = nodeid,
                param,
                value,
                size
            };

            var json = JsonSerializer.Serialize(payload);
            await tcpService.SendLineAsync(json, CancellationToken.None).ConfigureAwait(false);
        }
        catch (Exception ex)
        {
            SetStatusMessage(ex.Message);
        }
    }

    public void HandleConnectionClosed()
    {
        _ = dispatcher.InvokeAsync(() =>
        {
            NotifyConnectionStateChanged();

            if (!string.Equals(getStatusMessage(), "Disconnected", StringComparison.OrdinalIgnoreCase))
                SetStatusMessage("Connection closed");
        });
    }

    private void NotifyConnectionStateChanged()
    {
        OnPropertyChanged(nameof(IsConnected));
        connectCommand.RaiseCanExecuteChanged();
        disconnectCommand.RaiseCanExecuteChanged();
        refreshLogsCommand.RaiseCanExecuteChanged();
    }

    private void SetStatusMessage(string message)
    {
        if (dispatcher.CheckAccess())
            setStatusMessage(message);
        else
            _ = dispatcher.InvokeAsync(() => setStatusMessage(message));
    }

    private void LoadSettings()
    {
        suppressSettingsSave = true;
        try
        {
            var settings = ConnectionSettingsStore.Load(host, port);
            Host = settings.Host;
            Port = settings.Port;
            autoConnect = settings.AutoConnect;
        }
        finally
        {
            suppressSettingsSave = false;
        }
    }

    private void SaveSettings()
    {
        if (suppressSettingsSave)
            return;

        ConnectionSettingsStore.Save(new ConnectionSettings(Host, Port, autoConnect));
    }
}
