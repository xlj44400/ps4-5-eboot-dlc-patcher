using Spectre.Console;

namespace ps4_eboot_dlc_patcher;
internal class ConsoleUi
{
    public static void LogError(string message)
    {
        AnsiConsole.MarkupLineInterpolated($"[red][[ERROR]] {message}[/]");
    }

    public static void LogWarning(string message)
    {
        AnsiConsole.MarkupLineInterpolated($"[yellow][[WARN]] {message}[/]");
    }

    public static void LogInfo(string message)
    {
        AnsiConsole.MarkupLineInterpolated($"[blue][[INFO]] {message}[/]");
    }

    public static void LogSuccess(string message)
    {
        AnsiConsole.MarkupLineInterpolated($"[green][[INFO]] {message}[/]");
    }

    public static void WriteLine(string message)
    {
        AnsiConsole.WriteLine(message);
    }

    public static bool Confirm(string message)
    {
        return AnsiConsole.Confirm(message);
    }

    public static List<string> MultilineInput(string message)
    {
        AnsiConsole.MarkupLineInterpolated($"[yellow]{message}[/]");
        AnsiConsole.MarkupLine($"Press enter on an empty line to finish");

        List<string> lines = new();
        while (true)
        {
            var line = Console.ReadLine();
            if (string.IsNullOrWhiteSpace(line))
            {
                break;
            }
            lines.Add(line);
        }
        return lines;
    }

    public static string? Input(string message)
    {
        AnsiConsole.MarkupLineInterpolated($"[yellow]{message}[/]");
        return Console.ReadLine();
    }

    public class ProgressBar
    {
        private TaskCompletionSource<double> progressBarTask = new();
        private TaskCompletionSource progressUpdated = new();
        private bool isFinished = false;
        public ProgressBar(string task)
        {
            var prog = AnsiConsole.Progress();
            prog.AutoClear(true)
                .AutoRefresh(true)
                .Columns(new TaskDescriptionColumn(), new ProgressBarColumn(), new PercentageColumn(), new RemainingTimeColumn(), new SpinnerColumn())
                .StartAsync(async ctx =>
                {
                    var task1 = ctx.AddTask(task);
                    task1.MaxValue = 100;
                    isFinished = ctx.IsFinished;
                    while (!isFinished)
                    {
                        var newProgress = await progressBarTask.Task;

                        task1.Value = newProgress;

                        ctx.Refresh();
                        isFinished = ctx.IsFinished;
                        if (!isFinished)
                        {
                            progressBarTask = new TaskCompletionSource<double>();
                            progressUpdated.SetResult();
                        }
                    }

                    ctx.Refresh();
                    progressBarTask = new TaskCompletionSource<double>();
                    progressUpdated.SetResult();
                });
        }

        /// <summary>
        /// Not thread safe
        /// </summary>
        /// <param name="newProgress"></param>
        /// <returns></returns>
        public async Task Update(double newProgress)
        {
            if (isFinished)
            { return; }

            progressUpdated = new();
            progressBarTask.SetResult(newProgress);
            await progressUpdated.Task;
        }


    }
}
