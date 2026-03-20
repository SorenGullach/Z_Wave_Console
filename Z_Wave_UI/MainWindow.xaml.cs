using System.Windows;

using Z_Wave_UI.ViewModels;

namespace Z_Wave_UI
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        private readonly MainViewModel viewModel;

        public MainWindow()
        {
            InitializeComponent();

            viewModel = new MainViewModel();
            DataContext = viewModel;

            Closed += (_, _) => viewModel.Dispose();
        }
    }
}