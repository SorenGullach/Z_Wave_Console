using System.ComponentModel;
using System.Runtime.CompilerServices;

namespace Z_Wave_UI.ViewModels;

public abstract class ViewModelBase : INotifyPropertyChanged
{
    public event PropertyChangedEventHandler? PropertyChanged;

    protected void OnPropertyChanged([CallerMemberName] string? propertyname = null)
        => PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyname));

    protected bool SetProperty<T>(ref T storage, T value, [CallerMemberName] string? propertyname = null)
    {
        if (EqualityComparer<T>.Default.Equals(storage, value))
            return false;

        storage = value;
        OnPropertyChanged(propertyname);
        return true;
    }
}
