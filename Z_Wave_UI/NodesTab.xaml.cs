using System;
using System.Collections.Generic;
using System.Text;
using System.Text.RegularExpressions;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Windows.Threading;
using Z_Wave_UI.Models;
using Z_Wave_UI.ViewModels;

namespace Z_Wave_UI
{
    /// <summary>
    /// Interaction logic for NodesTab1.xaml
    /// </summary>
    public partial class NodesTab : UserControl
    {
        public NodesTab()
        {
            InitializeComponent();
        }

        private async void TreeView_SelectedItemChanged(object sender, RoutedPropertyChangedEventArgs<object> e)
        {
            if (e.NewValue is not NodeListInfo selectedNode)
                return;

            if (DataContext is not MainViewModel mainViewModel)
                return;

            await mainViewModel.Connection.RefreshNodeInfoAsync(selectedNode.NodeId);
        }

        private static readonly Regex _hexRegex = new Regex("^[0-9A-Fa-f]+$");

        private void HexTextBox_PreviewTextInput(object sender, TextCompositionEventArgs e)
        {
            e.Handled = !_hexRegex.IsMatch(e.Text);
        }

    }
}
