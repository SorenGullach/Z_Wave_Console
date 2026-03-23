using System.Collections.ObjectModel;
using System.Text.Json;
using System.Windows.Threading;
using Z_Wave_UI.Models;
using static Z_Wave_UI.ViewModels.NodeListViewModel;

namespace Z_Wave_UI.Models
{
    public class NodeListInfo
    {
        public int NodeId { get; }
        public string Floor { get; }
        public string Room { get; }
        public string State { get; }
        public string InterviewState { get; }

        public NodeListInfo(int nodeId, string floor, string room, string state, string interviewState)
        {
            NodeId = nodeId;
            Floor = floor;
            Room = room;
            State = state;
            InterviewState = interviewState;
        }
    }
}

namespace Z_Wave_UI.ViewModels
{
    public class FloorGroup
    {
        public string Name { get; }
        public ObservableCollection<RoomGroup> Rooms { get; } = new();

        public FloorGroup(string name) => Name = name;
    }

    public class RoomGroup
    {
        public string Name { get; }
        public ObservableCollection<Z_Wave_UI.Models.NodeListInfo> Nodes { get; } = new();

        public RoomGroup(string name) => Name = name;
    }

    public class NodeListViewModel : ViewModelBase
    {
        private readonly Dispatcher dispatcher;
        private readonly Action<string> setStatusMessage;

        public ObservableCollection<FloorGroup> Tree { get; } = new();

        public NodeListViewModel(Dispatcher dispatcher, Action<string> setStatusMessage)
        {
            this.dispatcher = dispatcher;
            this.setStatusMessage = setStatusMessage;
        }

        public void HandleLine(string line, Func<Task> RefreshNodeListAsync)
        {
            try
            {
                using var doc = JsonDocument.Parse(line);
                if (!doc.RootElement.TryGetProperty("type", out var typeElement))
                    return;

                var type = typeElement.GetString();
                if (string.Equals(type, "nodes_list", StringComparison.OrdinalIgnoreCase))
                {
                    var list = ParseNodeListInfo(doc.RootElement);
                    if (list is null)
                        return;

                    _ = dispatcher.InvokeAsync(() =>
                    {
                        ApplyNodeListInfo(list);
                        setStatusMessage("Node list updated");
                    });

                    return;
                }

                if (string.Equals(type, "node_list_changed", StringComparison.OrdinalIgnoreCase))
                {
                    _ = RefreshNodeListAsync();
                    return;
                }
            }
            catch
            {
                // ignore malformed JSON
            }
        }

        private static List<Z_Wave_UI.Models.NodeListInfo>? ParseNodeListInfo(JsonElement root)
        {
            if (!root.TryGetProperty("nodes", out var nodesElement) ||
                nodesElement.ValueKind != JsonValueKind.Array)
                return null;

            var list = new List<Z_Wave_UI.Models.NodeListInfo>();

            foreach (var nodeElement in nodesElement.EnumerateArray())
            {
                var nodeId = nodeElement.GetProperty("nodeId").GetInt32();
                var floor = nodeElement.GetProperty("floor").GetString() ?? "Unknown";
                var room = nodeElement.GetProperty("room").GetString() ?? "Unknown";
                var state = nodeElement.GetProperty("state").GetString() ?? "Unknown";
                var interviewState = nodeElement.GetProperty("interviewState").GetString() ?? "Unknown";

                list.Add(new Z_Wave_UI.Models.NodeListInfo(nodeId, floor, room, state, interviewState));
            }

            return list;
        }

        private void ApplyNodeListInfo(List<Z_Wave_UI.Models.NodeListInfo> list)
        {
            Tree.Clear();

            var floors = list
                .GroupBy(n => n.Floor)
                .OrderBy(g => g.Key);

            foreach (var floorGroup in floors)
            {
                var floor = new FloorGroup(floorGroup.Key);

                var rooms = floorGroup
                    .GroupBy(n => n.Room)
                    .OrderBy(g => g.Key);

                foreach (var roomGroup in rooms)
                {
                    var room = new RoomGroup(roomGroup.Key);

                    foreach (var node in roomGroup.OrderBy(n => n.NodeId))
                        room.Nodes.Add(node);

                    floor.Rooms.Add(room);
                }

                Tree.Add(floor);
            }
        }
    }
}
