using System.Net.Sockets;
using System.Text;
using System.IO;

namespace Z_Wave_UI.Services;

public sealed class ZWaveTcpClientService : IDisposable
{
    private TcpClient? tcpClient;
    private StreamReader? reader;
    private StreamWriter? writer;
    private CancellationTokenSource? readLoopCts;
    private readonly SemaphoreSlim sendLock = new(1, 1);

    public event EventHandler<string>? LineReceived;
    public event EventHandler? ConnectionClosed;

    public bool IsConnected => tcpClient?.Connected == true;

    public async Task ConnectAsync(string host, int port, CancellationToken cancellationtoken)
    {
        await DisconnectAsync().ConfigureAwait(false);

        tcpClient = new TcpClient();
        await tcpClient.ConnectAsync(host, port, cancellationtoken).ConfigureAwait(false);

        var stream = tcpClient.GetStream();
        reader = new StreamReader(stream, Encoding.UTF8, detectEncodingFromByteOrderMarks: false, bufferSize: 4096, leaveOpen: true);
        writer = new StreamWriter(stream, new UTF8Encoding(encoderShouldEmitUTF8Identifier: false))
        {
            NewLine = "\n",
            AutoFlush = true,
        };

        readLoopCts = new CancellationTokenSource();
        _ = Task.Run(() => ReadLoopAsync(readLoopCts.Token), CancellationToken.None);
    }

    public async Task DisconnectAsync()
    {
        if (readLoopCts is not null)
        {
            try { readLoopCts.Cancel(); }
            catch { }
            readLoopCts.Dispose();
            readLoopCts = null;
        }

        if (tcpClient is not null)
        {
            try { tcpClient.Close(); }
            catch { }
            tcpClient = null;
        }

        if (reader is not null)
        {
            try { reader.Dispose(); }
            catch { }
            reader = null;
        }

        if (writer is not null)
        {
            try { writer.Dispose(); }
            catch { }
            writer = null;
        }
    }

    public async Task SendLineAsync(string line, CancellationToken cancellationtoken)
    {
        if (writer is null)
            throw new InvalidOperationException("Not connected");

        await sendLock.WaitAsync(cancellationtoken).ConfigureAwait(false);
        try
        {
            await writer.WriteLineAsync(line.AsMemory(), cancellationtoken).ConfigureAwait(false);
        }
        finally
        {
            sendLock.Release();
        }
    }

    private async Task ReadLoopAsync(CancellationToken cancellationtoken)
    {
        try
        {
            while (!cancellationtoken.IsCancellationRequested)
            {
                if (reader is null)
                    break;

                var line = await reader.ReadLineAsync(cancellationtoken).ConfigureAwait(false);
                if (line is null)
                    break;

                if (line.Length == 0)
                    continue;

                LineReceived?.Invoke(this, line);
            }
        }
        catch (OperationCanceledException)
        {
        }
        catch
        {
        }
        finally
        {
            ConnectionClosed?.Invoke(this, EventArgs.Empty);
        }
    }

    public void Dispose()
    {
        sendLock.Dispose();
        readLoopCts?.Cancel();
        readLoopCts?.Dispose();
        tcpClient?.Close();
    }
}
