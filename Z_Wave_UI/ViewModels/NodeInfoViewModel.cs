using System.Text.Json;
using System.Windows.Threading;
using Z_Wave_UI.Models;

namespace Z_Wave_UI.Models
{
    public record NodeInfo(
        int NodeId,
        string NodeName,
        string ManufacturerName,
        string ProductName,
        string ProductLabel,
        string ManufacturerId,
        string ProductId
        );
}

namespace Z_Wave_UI.ViewModels
{
    public class NodeInfoViewModel : ViewModelBase
    {
        private readonly Dispatcher dispatcher;
        private readonly Action<string> setStatusMessage;

        public NodeInfoViewModel(Dispatcher dispatcher, Action<string> setStatusMessage)
        {
            this.dispatcher = dispatcher;
            this.setStatusMessage = setStatusMessage;
        }

        internal void HandleLine(string line, Func<int, Task> refreshNodeInfoAsync)
        {
            try
            {
                using var doc = JsonDocument.Parse(line);
                if (!doc.RootElement.TryGetProperty("type", out var typeElement))
                    return;

                var type = typeElement.GetString();
                if (string.Equals(type, "nodes_list", StringComparison.OrdinalIgnoreCase))
                {
                    var list = ParseNodeInfo(doc.RootElement);
                    if (list is null)
                        return;

                    _ = dispatcher.InvokeAsync(() =>
                    {
                        //ApplyNodeInfo(list);
                        setStatusMessage("Node list updated");
                    });

                    return;
                }

                if (string.Equals(type, "node_changed", StringComparison.OrdinalIgnoreCase))
                {
                    // "{\"type\":\"node_changed\",\"nodeId\":" + std::to_string(nodeId.value) + "}"
                    int nodeId = 0; // TODO get from message
                    _ = refreshNodeInfoAsync(nodeId);
                    return;
                }
            }
            catch
            {
                // ignore malformed JSON
            }

        }

        private static Z_Wave_UI.Models.NodeInfo? ParseNodeInfo(JsonElement root)
        {
            return null;
        }

        private void ApplyNodeInfo(int nodeId)
        {

        }
    }
}
