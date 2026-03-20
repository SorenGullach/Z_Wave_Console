using System.Collections.Specialized;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Threading;

namespace Z_Wave_UI.Helpers;

public static class DataGridAutoScrollBehavior
{
    public static readonly DependencyProperty AutoScrollToEndProperty =
        DependencyProperty.RegisterAttached(
            "AutoScrollToEnd",
            typeof(bool),
            typeof(DataGridAutoScrollBehavior),
            new PropertyMetadata(false, OnAutoScrollToEndChanged));

    private static readonly DependencyProperty CollectionChangedHandlerProperty =
        DependencyProperty.RegisterAttached(
            "CollectionChangedHandler",
            typeof(NotifyCollectionChangedEventHandler),
            typeof(DataGridAutoScrollBehavior),
            new PropertyMetadata(null));

    public static bool GetAutoScrollToEnd(DependencyObject obj) => (bool)obj.GetValue(AutoScrollToEndProperty);

    public static void SetAutoScrollToEnd(DependencyObject obj, bool value) => obj.SetValue(AutoScrollToEndProperty, value);

    private static void OnAutoScrollToEndChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d is not DataGrid dataGrid)
            return;

        if (e.NewValue is true)
        {
            dataGrid.Loaded += DataGrid_Loaded;
            dataGrid.Unloaded += DataGrid_Unloaded;

            if (dataGrid.IsLoaded)
                Attach(dataGrid);
        }
        else
        {
            Detach(dataGrid);
            dataGrid.Loaded -= DataGrid_Loaded;
            dataGrid.Unloaded -= DataGrid_Unloaded;
        }
    }

    private static void DataGrid_Loaded(object sender, RoutedEventArgs e)
    {
        if (sender is DataGrid dataGrid)
            Attach(dataGrid);
    }

    private static void DataGrid_Unloaded(object sender, RoutedEventArgs e)
    {
        if (sender is DataGrid dataGrid)
            Detach(dataGrid);
    }

    private static void Attach(DataGrid dataGrid)
    {
        if (dataGrid.GetValue(CollectionChangedHandlerProperty) is not null)
            return;

        if (dataGrid.Items is not INotifyCollectionChanged notify)
            return;

        NotifyCollectionChangedEventHandler handler = (_, _) => ScrollToEnd(dataGrid);
        dataGrid.SetValue(CollectionChangedHandlerProperty, handler);
        notify.CollectionChanged += handler;

        ScrollToEnd(dataGrid);
    }

    private static void Detach(DataGrid dataGrid)
    {
        if (dataGrid.GetValue(CollectionChangedHandlerProperty) is not NotifyCollectionChangedEventHandler handler)
            return;

        if (dataGrid.Items is INotifyCollectionChanged notify)
            notify.CollectionChanged -= handler;

        dataGrid.ClearValue(CollectionChangedHandlerProperty);
    }

    private static void ScrollToEnd(DataGrid dataGrid)
    {
        if (!GetAutoScrollToEnd(dataGrid))
            return;

        if (dataGrid.Items.Count == 0)
            return;

        var lastItem = dataGrid.Items[dataGrid.Items.Count - 1];
        dataGrid.Dispatcher.BeginInvoke(() => dataGrid.ScrollIntoView(lastItem), DispatcherPriority.Background);
    }
}
