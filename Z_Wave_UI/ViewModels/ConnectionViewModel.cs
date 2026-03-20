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

        connectCommand = new AsyncRelayCommand(ConnectAsync, () => !IsConnected);
        disconnectCommand = new AsyncRelayCommand(DisconnectAsync, () => IsConnected);
        refreshLogsCommand = new AsyncRelayCommand(RefreshLogsAsync, () => IsConnected);
    }

    public string Host
    {
        get => host;
        set => SetProperty(ref host, value);
    }

    public int Port
    {
        get => port;
        set => SetProperty(ref port, value);
    }

    public bool IsConnected => tcpService.IsConnected;

    public AsyncRelayCommand ConnectCommand => connectCommand;
    public AsyncRelayCommand DisconnectCommand => disconnectCommand;
    public AsyncRelayCommand RefreshLogsCommand => refreshLogsCommand;

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
            await RefreshLogsAsync().ConfigureAwait(false);
            await RefreshModuleInfoAsync().ConfigureAwait(false);
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

        await dispatcher.InvokeAsync(() =>
        {
            clearLogs();
            clearModuleInfo();
            SetStatusMessage("Disconnected");
            NotifyConnectionStateChanged();
        });
    }

    public async Task RefreshLogsAsync()
    {
        if (!IsConnected)
            return;

        try
        {
            await tcpService.SendLineAsync("{\"type\":\"get_logs\",\"count\":200}", CancellationToken.None).ConfigureAwait(false);
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
            await tcpService.SendLineAsync("{\"type\":\"get_controller_info\"}", CancellationToken.None).ConfigureAwait(false);
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
}
