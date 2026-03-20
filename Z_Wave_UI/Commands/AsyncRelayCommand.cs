using System.Windows.Input;

namespace Z_Wave_UI.Commands;

public sealed class AsyncRelayCommand : ICommand
{
    private readonly Func<Task> execute;
    private readonly Func<bool>? canExecute;
    private bool isExecuting;

    public AsyncRelayCommand(Func<Task> execute, Func<bool>? canexecute = null)
    {
        this.execute = execute;
        canExecute = canexecute;
    }

    public event EventHandler? CanExecuteChanged;

    public bool CanExecute(object? parameter) => !isExecuting && (canExecute?.Invoke() ?? true);

    public async void Execute(object? parameter)
    {
        if (!CanExecute(parameter))
            return;

        try
        {
            isExecuting = true;
            RaiseCanExecuteChanged();
            await execute();
        }
        finally
        {
            isExecuting = false;
            RaiseCanExecuteChanged();
        }
    }

    public void RaiseCanExecuteChanged() => CanExecuteChanged?.Invoke(this, EventArgs.Empty);
}
