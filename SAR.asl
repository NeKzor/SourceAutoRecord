/*
    SourceAutoRecord 1.8+

    Report issues at:
        https://github.com/NeKzor/SourceAutoRecord/issues
*/

state("portal2") { /* Portal 2 (7054) */ }
state("hl2") { /* Portal (1910503) */ }

startup
{
    vars.SAR = new MemoryWatcherList();
    vars.FindInterface = (Func<Process, bool>)((proc) =>
    {
        // TimerInterface
        var target = new SigScanTarget(16,
            "53 41 52 5F 54 49 4D 45 52 5F 53 54 41 52 54 00", // char start[16]
            "?? ?? ?? ??", // int total
            "?? ?? ?? ??", // float ipt
            "?? ?? ?? ??", // TimerAction action
            "53 41 52 5F 54 49 4D 45 52 5F 45 4E 44 00"); // char end[14]

        var result = IntPtr.Zero;
        foreach (var page in proc.MemoryPages(true))
        {
            var scanner = new SignatureScanner(proc, page.BaseAddress, (int)page.RegionSize);
            result = scanner.Scan(target);

            if (result != IntPtr.Zero)
            {
                print("[ASL] pubInterface = 0x" + result.ToString("X"));
                vars.Total = new MemoryWatcher<int>(result);
                vars.Ipt = new MemoryWatcher<float>(result + sizeof(int));
                vars.Action = new MemoryWatcher<int>(result + sizeof(int) + sizeof(float));

                vars.SAR.Clear();
                vars.SAR.AddRange(new MemoryWatcher[]
                {
                    vars.Total,
                    vars.Ipt,
                    vars.Action
                });
                vars.SAR.UpdateAll(proc);

                print("[ASL] pubInterface->ipt = " + vars.Ipt.Current.ToString());
                return true;
            }
        }

        print("[ASL] Memory scan failed!");
        return false;
    });

    // TimerAction
    vars.TimerActions = new Dictionary<string, int>()
    {
        { "DoNothing",  0 },
        { "Start",      1 },
        { "Restart",    2 },
        { "Split",      3 },
        { "End",        4 },
        { "Reset",      5 },
        { "Pause",      6 },
        { "Resume",     7 },
    };

    vars.TimerAction = (Func<string, int>)((key) =>
    {
        return (vars.TimerActions as Dictionary<string, int>)
            .First(x => x.Key == key).Value;
    });
}

init
{
    vars.Init = false;
}

update
{
    if (vars.Init)
    {
        timer.IsGameTimePaused = true;
        vars.SAR.UpdateAll(game);

        if (modules.FirstOrDefault(m => m.ModuleName == "sar.dll") == null)
        {
            vars.Init = false;
        }
    }
    else
    {
        if (modules.FirstOrDefault(m => m.ModuleName == "sar.dll") != null)
        {
            vars.Init = vars.FindInterface(game);
        }
    }

    return vars.Init;
}

gameTime
{
    return TimeSpan.FromSeconds(vars.Total.Current * vars.Ipt.Current);
}

start
{
    return vars.Action.Changed
        && (vars.Action.Current == vars.TimerAction("Start") || vars.Action.Current == vars.TimerAction("Restart"));
}

reset
{
    return vars.Action.Changed
        && (vars.Action.Current == vars.TimerAction("Restart")  || vars.Action.Current == vars.TimerAction("Reset"));
}

split
{
    return vars.Action.Changed
        && (vars.Action.Current == vars.TimerAction("Split") || vars.Action.Current == vars.TimerAction("End"));
}
