using System;
using System.Collections.Generic;
using System.Text;

namespace Z_Wave_UI
{
    class FileName
    {
    }
}

< Grid Margin = "6" >
        < Grid.ColumnDefinitions >
            < ColumnDefinition Width = "Auto" MinWidth = "200" />
            < ColumnDefinition Width = "*" />
        </ Grid.ColumnDefinitions >
        < TreeView Margin = "6" Grid.Column = "0" DataContext = "{Binding NodeList}" ItemsSource = "{Binding Tree}" >
            < TreeView.Resources >
                < HierarchicalDataTemplate DataType = "{x:Type viewmodels:FloorGroup}" ItemsSource = "{Binding Rooms}" >
                    < TextBlock Text = "{Binding Name}" FontWeight = "Bold" />
                </ HierarchicalDataTemplate >
                < HierarchicalDataTemplate DataType = "{x:Type viewmodels:RoomGroup}" ItemsSource = "{Binding Nodes}" >
                    < TextBlock Text = "{Binding Name}" />
                </ HierarchicalDataTemplate >
                < DataTemplate DataType = "{x:Type models:NodeListInfo}" >
                    < StackPanel Orientation = "Horizontal" >
                        < TextBlock Text = "{Binding NodeId}" FontWeight = "Bold" />
                        < TextBlock Text = " (" Foreground = "Gray" />
                        < TextBlock Text = "{Binding State}" Foreground = "Gray" />
                        < TextBlock Text = ")" Foreground = "Gray" />
                        < TextBlock Text = "{Binding InterviewState}" Foreground = "Gray" />
                        < TextBlock Text = ")" Foreground = "Gray" />
                    </ StackPanel >
                </ DataTemplate >
            </ TreeView.Resources >
        </ TreeView >
        < ScrollViewer Grid.Column = "1" VerticalScrollBarVisibility = "Auto" DataContext = "{Binding NodeInfo}" >
            < StackPanel Margin = "10" Orientation = "Vertical" >
                < !--BASIC INFO-- >
                < GroupBox Header = "Basic Information" >
                    < Grid Margin = "6" >
                        < Grid.RowDefinitions >
                            < RowDefinition />
                            < RowDefinition />
                            < RowDefinition />
                            < RowDefinition />
                            < RowDefinition />
                        </ Grid.RowDefinitions >
                        < Grid.ColumnDefinitions >
                            < ColumnDefinition Width = "150" />
                            < ColumnDefinition Width = "*" />
                        </ Grid.ColumnDefinitions >

                        < TextBlock Text = "Node ID:" Grid.Row = "0" Grid.Column = "0" />
                        < TextBlock Text = "{Binding NodeId}" Grid.Row = "0" Grid.Column = "1" />

                        < TextBlock Text = "Floor:" Grid.Row = "1" Grid.Column = "0" />
                        < TextBlock Text = "{Binding Floor}" Grid.Row = "1" Grid.Column = "1" />

                        < TextBlock Text = "Room:" Grid.Row = "2" Grid.Column = "0" />
                        < TextBlock Text = "{Binding Room}" Grid.Row = "2" Grid.Column = "1" />

                        < TextBlock Text = "State:" Grid.Row = "3" Grid.Column = "0" />
                        < TextBlock Text = "{Binding State}" Grid.Row = "3" Grid.Column = "1" />

                        < TextBlock Text = "Interview State:" Grid.Row = "4" Grid.Column = "0" />
                        < TextBlock Text = "{Binding InterviewState}" Grid.Row = "4" Grid.Column = "1" />
                    </ Grid >
                </ GroupBox >

                < !--PROTOCOL INFO-- >
                < GroupBox Header = "Protocol Information" >
                    < Grid Margin = "6" >
                        < Grid.RowDefinitions >
                            < RowDefinition />
                            < RowDefinition />
                            < RowDefinition />
                            < RowDefinition />
                            < RowDefinition />
                            < RowDefinition />
                            < RowDefinition />
                        </ Grid.RowDefinitions >
                        < Grid.ColumnDefinitions >
                            < ColumnDefinition Width = "150" />
                            < ColumnDefinition Width = "*" />
                        </ Grid.ColumnDefinitions >

                        < TextBlock Text = "Basic:" Grid.Row = "0" Grid.Column = "0" />
                        < TextBlock Text = "{Binding Protocol.Basic}" Grid.Row = "0" Grid.Column = "1" />

                        < TextBlock Text = "Generic:" Grid.Row = "1" Grid.Column = "0" />
                        < TextBlock Text = "{Binding Protocol.Generic}" Grid.Row = "1" Grid.Column = "1" />

                        < TextBlock Text = "Specific:" Grid.Row = "2" Grid.Column = "0" />
                        < TextBlock Text = "{Binding Protocol.Specific}" Grid.Row = "2" Grid.Column = "1" />

                        < TextBlock Text = "Listening:" Grid.Row = "3" Grid.Column = "0" />
                        < TextBlock Text = "{Binding Protocol.Listening}" Grid.Row = "3" Grid.Column = "1" />

                        < TextBlock Text = "Routing:" Grid.Row = "4" Grid.Column = "0" />
                        < TextBlock Text = "{Binding Protocol.Routing}" Grid.Row = "4" Grid.Column = "1" />

                        < TextBlock Text = "Speed:" Grid.Row = "5" Grid.Column = "0" />
                        < TextBlock Text = "{Binding Protocol.Speed}" Grid.Row = "5" Grid.Column = "1" />

                        < TextBlock Text = "Protocol Version:" Grid.Row = "6" Grid.Column = "0" />
                        < TextBlock Text = "{Binding Protocol.ProtocolVersion}" Grid.Row = "6" Grid.Column = "1" />
                    </ Grid >
                </ GroupBox >

                < !--MANUFACTURER-- >
                < GroupBox Header = "Manufacturer" >
                    < Grid Margin = "6" >
                        < Grid.RowDefinitions >
                            < RowDefinition />
                            < RowDefinition />
                            < RowDefinition />
                            < RowDefinition />
                            < RowDefinition />
                            < RowDefinition />
                            < RowDefinition />
                        </ Grid.RowDefinitions >
                        < Grid.ColumnDefinitions >
                            < ColumnDefinition Width = "150" />
                            < ColumnDefinition Width = "*" />
                        </ Grid.ColumnDefinitions >

                        < TextBlock Text = "Manufacturer ID:" Grid.Row = "0" Grid.Column = "0" />
                        < TextBlock Text = "{Binding Manufacturer.Id}" Grid.Row = "0" Grid.Column = "1" />

                        < TextBlock Text = "Product Type:" Grid.Row = "1" Grid.Column = "0" />
                        < TextBlock Text = "{Binding Manufacturer.ProductType}" Grid.Row = "1" Grid.Column = "1" />

                        < TextBlock Text = "Product ID:" Grid.Row = "2" Grid.Column = "0" />
                        < TextBlock Text = "{Binding Manufacturer.ProductId}" Grid.Row = "2" Grid.Column = "1" />

                        < TextBlock Text = "Has Device ID:" Grid.Row = "3" Grid.Column = "0" />
                        < TextBlock Text = "{Binding Manufacturer.HasDeviceId}" Grid.Row = "3" Grid.Column = "1" />
                    </ Grid >
                </ GroupBox >

                < !--VALUES-- >
                < GroupBox Header = "Values" >
                    < Grid Margin = "6" >
                        < Grid.RowDefinitions >
                            < RowDefinition />
                            < RowDefinition />
                            < RowDefinition />
                            < RowDefinition />
                            < RowDefinition />
                            < RowDefinition />
                            < RowDefinition />
                        </ Grid.RowDefinitions >
                        < Grid.ColumnDefinitions >
                            < ColumnDefinition Width = "150" />
                            < ColumnDefinition Width = "*" />
                        </ Grid.ColumnDefinitions >

                        < TextBlock Text = "Battery:" Grid.Row = "0" Grid.Column = "0" />
                        < TextBlock Text = "{Binding Values.Battery}" Grid.Row = "0" Grid.Column = "1" />

                        < TextBlock Text = "Basic:" Grid.Row = "1" Grid.Column = "0" />
                        < TextBlock Text = "{Binding Values.Basic}" Grid.Row = "1" Grid.Column = "1" />

                        < TextBlock Text = "Switch Binary:" Grid.Row = "2" Grid.Column = "0" />
                        < TextBlock Text = "{Binding Values.SwitchBinary}" Grid.Row = "2" Grid.Column = "1" />

                        < TextBlock Text = "Switch Multilevel:" Grid.Row = "3" Grid.Column = "0" />
                        < TextBlock Text = "{Binding Values.SwitchMultilevel}" Grid.Row = "3" Grid.Column = "1" />

                        < TextBlock Text = "Sensor Binary:" Grid.Row = "4" Grid.Column = "0" />
                        < TextBlock Text = "{Binding Values.SensorBinary}" Grid.Row = "4" Grid.Column = "1" />

                        < TextBlock Text = "Protection:" Grid.Row = "5" Grid.Column = "0" />
                        < TextBlock Text = "{Binding Values.Protection}" Grid.Row = "5" Grid.Column = "1" />
                    </ Grid >
                </ GroupBox >

                < !--SUPPORTED CCs-- >
                < GroupBox Header = "Supported Command Classes" >
                    < ItemsControl ItemsSource = "{Binding SupportedCCs}" >
                        < ItemsControl.ItemTemplate >
                            < DataTemplate >
                                < TextBlock Text = "{Binding}" Margin = "4" />
                            </ DataTemplate >
                        </ ItemsControl.ItemTemplate >
                    </ ItemsControl >
                </ GroupBox >

                < !--MULTI CHANNEL-- >
                < GroupBox Header = "Multi-Channel Endpoints" >
                    < ItemsControl ItemsSource = "{Binding MultiChannel.Endpoints}" >
                        < ItemsControl.ItemTemplate >
                            < DataTemplate >
                                < StackPanel Margin = "4" >
                                    < TextBlock Text = "Endpoint:" FontWeight = "Bold" />
                                    < TextBlock Text = "{Binding EndpointId}" />

                                    < TextBlock Text = "Generic:" />
                                    < TextBlock Text = "{Binding Generic}" />

                                    < TextBlock Text = "Specific:" />
                                    < TextBlock Text = "{Binding Specific}" />

                                    < TextBlock Text = "Supported CCs:" />
                                    < ItemsControl ItemsSource = "{Binding SupportedCCs}" >
                                        < ItemsControl.ItemTemplate >
                                            < DataTemplate >
                                                < TextBlock Text = "{Binding}" Margin = "2" />
                                            </ DataTemplate >
                                        </ ItemsControl.ItemTemplate >
                                    </ ItemsControl >
                                </ StackPanel >
                            </ DataTemplate >
                        </ ItemsControl.ItemTemplate >
                    </ ItemsControl >
                </ GroupBox >

                < !--ASSOCIATIONS-- >
                < GroupBox Header = "Association Groups" >
                    < ItemsControl ItemsSource = "{Binding AssociationGroups}" >
                        < ItemsControl.ItemTemplate >
                            < DataTemplate >
                                < StackPanel Margin = "4" >
                                    < TextBlock Text = "{Binding GroupId, StringFormat=Group {0}}" FontWeight = "Bold" />
                                    < TextBlock Text = "Members:" />
                                    < ItemsControl ItemsSource = "{Binding Members}" >
                                        < ItemsControl.ItemTemplate >
                                            < DataTemplate >
                                                < TextBlock Text = "{Binding}" Margin = "2" />
                                            </ DataTemplate >
                                        </ ItemsControl.ItemTemplate >
                                    </ ItemsControl >
                                </ StackPanel >
                            </ DataTemplate >
                        </ ItemsControl.ItemTemplate >
                    </ ItemsControl >
                </ GroupBox >

                < !--CONFIGURATION-- >
                < GroupBox Header = "Configuration Parameters" >
                    < ItemsControl ItemsSource = "{Binding Configuration}" >
                        < ItemsControl.ItemTemplate >
                            < DataTemplate >
                                < StackPanel Margin = "4" >
                                    < TextBlock Text = "{Binding Param, StringFormat=Parameter {0}}" FontWeight = "Bold" />
                                    < TextBlock Text = "{Binding Value, StringFormat=Value: {0}}" />
                                    < TextBlock Text = "{Binding Size, StringFormat=Size: {0}}" />
                                </ StackPanel >
                            </ DataTemplate >
                        </ ItemsControl.ItemTemplate >
                    </ ItemsControl >
                </ GroupBox >

            </ StackPanel >
        </ ScrollViewer >
    </ Grid >
