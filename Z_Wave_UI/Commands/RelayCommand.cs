using System.Windows.Input;

namespace Z_Wave_UI.Commands;

public sealed class RelayCommand : ICommand
{
    private readonly Action execute;
    private readonly Func<bool>? canExecute;

    public RelayCommand(Action execute, Func<bool>? canexecute = null)
    {
        this.execute = execute;
        canExecute = canexecute;
    }

    public event EventHandler? CanExecuteChanged;

    public bool CanExecute(object? parameter) => canExecute?.Invoke() ?? true;

    public void Execute(object? parameter) => execute();

    public void RaiseCanExecuteChanged() => CanExecuteChanged?.Invoke(this, EventArgs.Empty);
}

public sealed class RelayCommand<T> : ICommand
{
    private readonly Action<T?> execute;
    private readonly Func<T?, bool>? canExecute;

    public RelayCommand(Action<T?> execute, Func<T?, bool>? canexecute = null)
    {
        this.execute = execute;
        canExecute = canexecute;
    }

    public event EventHandler? CanExecuteChanged;

    public bool CanExecute(object? parameter)
    {
        if (parameter is null)
            return canExecute?.Invoke(default) ?? true;

        if (parameter is T value)
            return canExecute?.Invoke(value) ?? true;

        return false;
    }

    public void Execute(object? parameter)
    {
        if (parameter is T value)
        {
            execute(value);
            return;
        }

        execute(default);
    }

    public void RaiseCanExecuteChanged() => CanExecuteChanged?.Invoke(this, EventArgs.Empty);
}
