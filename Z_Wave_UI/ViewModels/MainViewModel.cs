using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Text.Json;
using System.Windows;
using System.Windows.Data;
using System.Windows.Threading;

using Z_Wave_UI.Commands;
using Z_Wave_UI.Models;
using Z_Wave_UI.Services;

namespace Z_Wave_UI.ViewModels;

public sealed class MainViewModel : ViewModelBase, IDisposable
{
    private readonly ZWaveTcpClientService tcpService;
    private readonly Dispatcher dispatcher;

    private string statusMessage = "Disconnected";

    public MainViewModel()
    {
        dispatcher = Application.Current.Dispatcher;
        tcpService = new ZWaveTcpClientService();
        tcpService.LineReceived += TcpService_LineReceived;
        tcpService.ConnectionClosed += TcpService_ConnectionClosed;

        Logging = new LoggingViewModel(dispatcher, SetStatusMessage);
        ModuleInfo = new ModuleInfoViewModel(dispatcher, SetStatusMessage);
        NodeList = new NodeListViewModel(dispatcher, SetStatusMessage);

        Connection = new ConnectionViewModel(tcpService, dispatcher, SetStatusMessage, () => StatusMessage, Logging.Clear, ModuleInfo.Clear);
    }

    public string StatusMessage
    {
        get => statusMessage;
        private set => SetProperty(ref statusMessage, value);
    }

    public ConnectionViewModel Connection { get; }

    public LoggingViewModel Logging { get; }

    public ModuleInfoViewModel ModuleInfo { get; }
    
    public NodeListViewModel NodeList { get; }

    private void TcpService_ConnectionClosed(object? sender, EventArgs e)
    {
        Connection.HandleConnectionClosed();
        ModuleInfo.Clear();
    }

    private void TcpService_LineReceived(object? sender, string line)
    {
        Logging.HandleLine(line, Connection.RefreshLogsAsync);
        ModuleInfo.HandleLine(line, Connection.RefreshModuleInfoAsync);
        NodeList.HandleLine(line, Connection.RefreshNodeListAsync);
    }

    public void Dispose()
    {
        tcpService.LineReceived -= TcpService_LineReceived;
        tcpService.ConnectionClosed -= TcpService_ConnectionClosed;
        tcpService.Dispose();
    }

    private void SetStatusMessage(string message) => StatusMessage = message;
}
