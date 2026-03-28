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
        public bool NeedsUpdate { get; }

        public NodeListInfo(int nodeId, string floor, string room, string state, string interviewState, bool needsUpdate)
        {
            NodeId = nodeId;
            Floor = floor;
            Room = room;
            State = state;
            InterviewState = interviewState;
            NeedsUpdate = needsUpdate;
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
        public string test
        {
            get { return "Hello from NodeListViewModel"; }
        }

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

                if (string.Equals(type, "node_changed", StringComparison.OrdinalIgnoreCase))
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
                var needsUpdate = nodeElement.GetProperty("needsUpdate").GetBoolean();

                list.Add(new Z_Wave_UI.Models.NodeListInfo(nodeId, floor, room, state, interviewState, needsUpdate));
            }

            return list;
        }

        private void ApplyNodeListInfo(List<Z_Wave_UI.Models.NodeListInfo> list)
        {
            var floors = list
                .GroupBy(n => n.Floor)
                .OrderBy(g => g.Key)
                .ToList();

            var floorByName = Tree.ToDictionary(f => f.Name, StringComparer.OrdinalIgnoreCase);
            var incomingFloorNames = new HashSet<string>(floors.Select(f => f.Key), StringComparer.OrdinalIgnoreCase);

            foreach (var floorGroup in floors)
            {
                if (!floorByName.TryGetValue(floorGroup.Key, out var floor))
                {
                    floor = new FloorGroup(floorGroup.Key);
                    Tree.Add(floor);
                    floorByName[floorGroup.Key] = floor;
                }

                var rooms = floorGroup
                    .GroupBy(n => n.Room)
                    .OrderBy(g => g.Key)
                    .ToList();

                var roomByName = floor.Rooms.ToDictionary(r => r.Name, StringComparer.OrdinalIgnoreCase);
                var incomingRoomNames = new HashSet<string>(rooms.Select(r => r.Key), StringComparer.OrdinalIgnoreCase);

                foreach (var roomGroup in rooms)
                {
                    if (!roomByName.TryGetValue(roomGroup.Key, out var room))
                    {
                        room = new RoomGroup(roomGroup.Key);
                        floor.Rooms.Add(room);
                        roomByName[roomGroup.Key] = room;
                    }

                    var orderedNodes = roomGroup
                        .OrderBy(n => n.NodeId)
                        .ToList();

                    var existingById = room.Nodes.ToDictionary(n => n.NodeId);
                    var incomingIds = new HashSet<int>(orderedNodes.Select(n => n.NodeId));

                    for (int i = room.Nodes.Count - 1; i >= 0; i--)
                    {
                        if (!incomingIds.Contains(room.Nodes[i].NodeId))
                            room.Nodes.RemoveAt(i);
                    }

                    foreach (var node in orderedNodes)
                    {
                        if (!existingById.TryGetValue(node.NodeId, out var existingNode))
                        {
                            room.Nodes.Add(node);
                            continue;
                        }

                        if (!string.Equals(existingNode.Floor, node.Floor, StringComparison.Ordinal) ||
                            !string.Equals(existingNode.Room, node.Room, StringComparison.Ordinal) ||
                            !string.Equals(existingNode.State, node.State, StringComparison.Ordinal) ||
                            !string.Equals(existingNode.InterviewState, node.InterviewState, StringComparison.Ordinal) ||
                            existingNode.NeedsUpdate != node.NeedsUpdate)
                        {
                            var index = room.Nodes.IndexOf(existingNode);
                            if (index >= 0)
                                room.Nodes[index] = node;
                        }
                    }
                }

                for (int i = floor.Rooms.Count - 1; i >= 0; i--)
                {
                    if (!incomingRoomNames.Contains(floor.Rooms[i].Name))
                        floor.Rooms.RemoveAt(i);
                }
            }

            for (int i = Tree.Count - 1; i >= 0; i--)
            {
                if (!incomingFloorNames.Contains(Tree[i].Name))
                    Tree.RemoveAt(i);
            }
        }
    }
}
