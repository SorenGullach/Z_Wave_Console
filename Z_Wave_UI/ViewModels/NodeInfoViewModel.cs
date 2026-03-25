using System.Text.Json;
using System.Windows.Input;
using System.Windows.Threading;
using Z_Wave_UI.Commands;
using Z_Wave_UI.Models;

namespace Z_Wave_UI.Models
{
    public class NodeInfo
    {
        public int NodeId { get; set; }
        public string Floor { get; set; } = "Unknown";
        public string Room { get; set; } = "Unknown";

        public string State { get; set; } = "Unknown";
        public string InterviewState { get; set; } = "Unknown";

        public ProtocolInfo Protocol { get; set; } = new();
        public ManufacturerInfo Manufacturer { get; set; } = new();
        public WakeUpInfo WakeUp { get; set; } = new();
        public NodeValues Values { get; set; } = new();

        public class CCType
        {
            public int cc { get; set; }
            public string Name { get; set; } = "";
            public int Version { get; set; }
        };
        public List<CCType> SupportedCCs { get; set; } = new();
        public MultiChannelInfo MultiChannel { get; set; } = new();

        public List<AssociationGroup> AssociationGroups { get; set; } = new();
        public List<MultiChannelAssociationGroup> MultiChannelAssociations { get; set; } = new();
        public List<ConfigurationParameter> Configuration { get; set; } = new();
    }

    public class ProtocolInfo
    {
        public int Basic { get; set; }
        public int Generic { get; set; }
        public int Specific { get; set; }
        public bool Listening { get; set; }
        public bool Routing { get; set; }
        public int Speed { get; set; }
        public int ProtocolVersion { get; set; }
        public bool OptionalFunctionality { get; set; }
        public bool Sensor1000ms { get; set; }
        public bool Sensor250ms { get; set; }
        public bool BeamCapable { get; set; }
        public bool RoutingEndNode { get; set; }
        public bool SpecificDevice { get; set; }
        public bool ControllerNode { get; set; }
        public bool Security { get; set; }
    }

    public class ManufacturerInfo
    {
        public int Id { get; set; }
        public int ProductType { get; set; }
        public int ProductId { get; set; }
        public int DeviceIdType { get; set; }
        public int DeviceIdFormat { get; set; }
        public bool HasDeviceId { get; set; }
        public bool HasManufacturerData { get; set; }
    }

    public class WakeUpInfo
    {
        public int Interval { get; set; }
        public int Min { get; set; }
        public int Max { get; set; }
        public int Default { get; set; }
        public bool HasLastReport { get; set; }
    }

    public class NodeValues
    {
        public int Battery { get; set; }
        public int Basic { get; set; }
        public int SwitchBinary { get; set; }
        public int SwitchMultilevel { get; set; }
        public int SensorBinary { get; set; }
        public int Protection { get; set; }
    }

    public class MultiChannelInfo
    {
        public int EndpointCount { get; set; }
        public bool HasEndpointReport { get; set; }
        public List<EndpointInfo> Endpoints { get; set; } = new();
    }

    public class EndpointInfo
    {
        public int EndpointId { get; set; }
        public int Generic { get; set; }
        public int Specific { get; set; }
        public List<int> SupportedCCs { get; set; } = new();
        public bool HasCapabilityReport { get; set; }
    }

    public class AssociationGroup
    {
        public int GroupId { get; set; }
        public int MaxNodes { get; set; }
        public List<int> Members { get; set; } = new();
        public bool HasLastReport { get; set; }
    }

    public class MultiChannelAssociationGroup
    {
        public int GroupId { get; set; }
        public int MaxNodes { get; set; }
        public List<MultiChannelAssociationMember> Members { get; set; } = new();
        public bool HasLastReport { get; set; }
    }

    public class MultiChannelAssociationMember
    {
        public int NodeId { get; set; }
        public int Endpoint { get; set; }
    }

    public class ConfigurationParameter
    {
        public int Param { get; set; }
        public int Size { get; set; }
        public int Value { get; set; }
        public bool Valid { get; set; }
    }
}

namespace Z_Wave_UI.ViewModels
{
    public class NodeInfoViewModel : ViewModelBase
    {
        private readonly Dispatcher dispatcher;
        private readonly Action<string> setStatusMessage;
        private readonly Func<int, int, int, int, Task> updateConfigAsync;

        private NodeInfo? node;
        public NodeInfo? Node
        {
            get => node;
            private set
            {
                node = value;
                OnPropertyChanged();
                updateConfigCommand.RaiseCanExecuteChanged();
            }
        }

        private readonly AsyncRelayCommand nodeSelectedCommand;
        private readonly RelayCommand<ConfigurationParameter> updateConfigCommand;

        public ICommand UpdateConfigCommand => updateConfigCommand;

        public NodeInfoViewModel(
            Dispatcher dispatcher,
            Action<string> setStatusMessage,
            Func<int, int, int, int, Task> updateconfigasync)
        {
            this.dispatcher = dispatcher;
            this.setStatusMessage = setStatusMessage;
            updateConfigAsync = updateconfigasync;
            nodeSelectedCommand = new AsyncRelayCommand(() => Task.CompletedTask, () => Node is not null);
            updateConfigCommand = new RelayCommand<ConfigurationParameter>(
                ExecuteUpdateConfig,
                param => Node is not null && param is not null);
        }

        private void ExecuteUpdateConfig(ConfigurationParameter? param)
        {
            _ = UpdateConfigAsync(param);
        }

        private async Task UpdateConfigAsync(ConfigurationParameter? param)
        {
            if (param is null || Node is null)
                return;

            try
            {
                await updateConfigAsync(Node.NodeId, param.Param, param.Value, param.Size);
                setStatusMessage($"Node {Node.NodeId} config {param.Param} updated");
            }
            catch (Exception ex)
            {
                setStatusMessage(ex.Message);
            }
        }

        internal void HandleLine(string line, Func<int, Task> refreshNodeInfoAsync)
        {
            try
            {
                using var doc = JsonDocument.Parse(line);
                if (!doc.RootElement.TryGetProperty("type", out var typeElement))
                    return;

                var type = typeElement.GetString();

                if (string.Equals(type, "node_info", StringComparison.OrdinalIgnoreCase))
                {
                    var ni = ParseNodeInfo(doc.RootElement);
                    if (ni is null)
                        return;

                    _ = dispatcher.InvokeAsync(() =>
                    {
                        ApplyNodeInfo(ni);
                        setStatusMessage($"Node {ni.NodeId} info updated");
                    });

                    return;
                }

                if (string.Equals(type, "node_changed", StringComparison.OrdinalIgnoreCase))
                {
                    if (doc.RootElement.TryGetProperty("nodeId", out var idProp))
                    {
                        int nodeId = idProp.GetInt32();
                        _ = refreshNodeInfoAsync(nodeId);
                    }
                    return;
                }
            }
            catch
            {
                // ignore malformed JSON
            }
        }

        private void ApplyNodeInfo(NodeInfo info)
        {
            Node = info;
        }

        private static NodeInfo? ParseNodeInfo(JsonElement root)
        {
            if (!root.TryGetProperty("nodeId", out var idElement))
                return null;

            var info = new NodeInfo
            {
                NodeId = idElement.GetInt32(),
                Floor = root.GetProperty("floor").GetString() ?? "Unknown",
                Room = root.GetProperty("room").GetString() ?? "Unknown",
                State = root.GetProperty("state").GetString() ?? "Unknown",
                InterviewState = root.GetProperty("interviewState").GetString() ?? "Unknown"
            };

            // Protocol
            if (root.TryGetProperty("protocol", out var protocol))
            {
                info.Protocol = new ProtocolInfo
                {
                    Basic = protocol.GetProperty("basic").GetInt32(),
                    Generic = protocol.GetProperty("generic").GetInt32(),
                    Specific = protocol.GetProperty("specific").GetInt32(),
                    Listening = protocol.GetProperty("listening").GetBoolean(),
                    Routing = protocol.GetProperty("routing").GetBoolean(),
                    Speed = protocol.GetProperty("speed").GetInt32(),
                    ProtocolVersion = protocol.GetProperty("protocolVersion").GetInt32(),
                    OptionalFunctionality = protocol.GetProperty("optionalFunctionality").GetBoolean(),
                    Sensor1000ms = protocol.GetProperty("sensor1000ms").GetBoolean(),
                    Sensor250ms = protocol.GetProperty("sensor250ms").GetBoolean(),
                    BeamCapable = protocol.GetProperty("beamCapable").GetBoolean(),
                    RoutingEndNode = protocol.GetProperty("routingEndNode").GetBoolean(),
                    SpecificDevice = protocol.GetProperty("specificDevice").GetBoolean(),
                    ControllerNode = protocol.GetProperty("controllerNode").GetBoolean(),
                    Security = protocol.GetProperty("security").GetBoolean()
                };
            }

            // Manufacturer
            if (root.TryGetProperty("manufacturer", out var m))
            {
                info.Manufacturer = new ManufacturerInfo
                {
                    Id = m.GetProperty("id").GetInt32(),
                    ProductType = m.GetProperty("productType").GetInt32(),
                    ProductId = m.GetProperty("productId").GetInt32(),
                    DeviceIdType = m.GetProperty("deviceIdType").GetInt32(),
                    DeviceIdFormat = m.GetProperty("deviceIdFormat").GetInt32(),
                    HasDeviceId = m.GetProperty("hasDeviceId").GetBoolean(),
                    HasManufacturerData = m.GetProperty("hasManufacturerData").GetBoolean()
                };
            }

            // WakeUp
            if (root.TryGetProperty("wakeUp", out var w))
            {
                info.WakeUp = new WakeUpInfo
                {
                    Interval = w.GetProperty("interval").GetInt32(),
                    Min = w.GetProperty("min").GetInt32(),
                    Max = w.GetProperty("max").GetInt32(),
                    Default = w.GetProperty("default").GetInt32(),
                    HasLastReport = w.GetProperty("hasLastReport").GetBoolean()
                };
            }

            // Values
            if (root.TryGetProperty("values", out var v))
            {
                info.Values = new NodeValues
                {
                    Battery = v.GetProperty("battery").GetInt32(),
                    Basic = v.GetProperty("basic").GetInt32(),
                    SwitchBinary = v.GetProperty("switchBinary").GetInt32(),
                    SwitchMultilevel = v.GetProperty("switchMultilevel").GetInt32(),
                    SensorBinary = v.GetProperty("sensorBinary").GetInt32(),
                    Protection = v.GetProperty("protection").GetInt32()
                };
            }

            // Supported CCs
            if (root.TryGetProperty("supportedCCs", out var ccArray))
            {
                info.SupportedCCs.Clear();
                foreach (var cc in ccArray.EnumerateArray())
                {
                    var ccInfo = new Models.NodeInfo.CCType
                    {
                        cc = cc.GetProperty("cc").GetInt32(),
                        Version = cc.GetProperty("version").GetInt32(),
                        Name = cc.GetProperty("name").GetString() ?? "Unknown"
                    };
                    info.SupportedCCs.Add(ccInfo);
                }
            }

            // MultiChannel
            if (root.TryGetProperty("multiChannel", out var mc))
            {
                var mcInfo = new MultiChannelInfo
                {
                    EndpointCount = mc.GetProperty("endpointCount").GetInt32(),
                    HasEndpointReport = mc.GetProperty("hasEndpointReport").GetBoolean()
                };

                foreach (var ep in mc.GetProperty("endpoints").EnumerateArray())
                {
                    var epInfo = new EndpointInfo
                    {
                        EndpointId = ep.GetProperty("endpointId").GetInt32(),
                        Generic = ep.GetProperty("generic").GetInt32(),
                        Specific = ep.GetProperty("specific").GetInt32(),
                        HasCapabilityReport = ep.GetProperty("hasCapabilityReport").GetBoolean()
                    };

                    foreach (var cc in ep.GetProperty("supportedCCs").EnumerateArray())
                        epInfo.SupportedCCs.Add(cc.GetInt32());

                    mcInfo.Endpoints.Add(epInfo);
                }

                info.MultiChannel = mcInfo;
            }

            // Association Groups
            if (root.TryGetProperty("associationGroups", out var agArray))
            {
                info.AssociationGroups.Clear();
                foreach (var g in agArray.EnumerateArray())
                {
                    var group = new AssociationGroup
                    {
                        GroupId = g.GetProperty("groupId").GetInt32(),
                        MaxNodes = g.GetProperty("maxNodes").GetInt32(),
                        HasLastReport = g.GetProperty("hasLastReport").GetBoolean()
                    };

                    foreach (var mbr in g.GetProperty("members").EnumerateArray())
                        group.Members.Add(mbr.GetInt32());

                    info.AssociationGroups.Add(group);
                }
            }

            // MultiChannel Associations
            if (root.TryGetProperty("multiChannelAssociations", out var mcaArray))
            {
                info.MultiChannelAssociations.Clear();
                foreach (var g in mcaArray.EnumerateArray())
                {
                    var group = new MultiChannelAssociationGroup
                    {
                        GroupId = g.GetProperty("groupId").GetInt32(),
                        MaxNodes = g.GetProperty("maxNodes").GetInt32(),
                        HasLastReport = g.GetProperty("hasLastReport").GetBoolean()
                    };

                    foreach (var mbr in g.GetProperty("members").EnumerateArray())
                    {
                        group.Members.Add(new MultiChannelAssociationMember
                        {
                            NodeId = mbr.GetProperty("nodeId").GetInt32(),
                            Endpoint = mbr.GetProperty("endpoint").GetInt32()
                        });
                    }

                    info.MultiChannelAssociations.Add(group);
                }
            }

            // Configuration
            if (root.TryGetProperty("configuration", out var cfgArray))
            {
                info.Configuration.Clear();
                foreach (var c in cfgArray.EnumerateArray())
                {
                    var param = new ConfigurationParameter
                    {
                        Param = c.GetProperty("param").GetInt32(),
                        Size = c.GetProperty("size").GetInt32(),
                        Value = c.GetProperty("value").GetInt32(),
                        Valid = c.GetProperty("valid").GetBoolean()
                    };

                    info.Configuration.Add(param);
                }
            }

            return info;
        }
    }
}
