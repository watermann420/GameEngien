// scripts/csharp/GameScriptTemplate.csx
// Template for game-side scripting in C#, with MusicEngine usage.
// Reference your built MusicEngine DLL and add game API calls as needed.
#r "..\\..\\..\\..\\MusicEngine\\bin\\Debug\\net10.0-windows\\MusicEngine.dll"

using System;
using MusicEngine;
using MusicEngine.Core.Synthesizers;

public static class GameScript
{
    static ISynth synth = new OrganSynth();

    public static void OnInit()
    {
        Console.WriteLine("GameScript initialized");
    }

    public static void OnClickBox()
    {
        synth.NoteOn(0, 60, 100);
        System.Threading.Thread.Sleep(200);
        synth.NoteOff(0, 60);
    }

    public static void OnShutdown()
    {
        synth.Dispose();
    }
}
