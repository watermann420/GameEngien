// scripts/csharp/SoundSampler.csx
// Dependency-optional sound test script.
// If MusicEngine DLL is present, it will play C4 via OrganSynth.
// Otherwise it falls back to Console.Beep so the pipeline stays runnable without MusicEngine.

using System;
using System.IO;
using System.Reflection;
using System.Threading;

static class SoundSampler
{
    public static void Main()
    {
        if (TryPlayWithMusicEngine())
            return;

        // Fallback: simple beep so scripts still work without MusicEngine
        Console.WriteLine("MusicEngine not available. Using fallback beep.");
        Console.Beep(523, 400); // C5 for audibility
    }

    private static bool TryPlayWithMusicEngine()
    {
        try
        {
            // Adjust path as needed; we attempt a few common build outputs.
            string[] candidates = new[]
            {
                "..\\..\\MusicEngine\\bin\\Debug\\net10.0-windows\\MusicEngine.dll",
                "..\\..\\MusicEngine\\bin\\Release\\net10.0-windows\\MusicEngine.dll",
                "..\\..\\MusicEngine\\bin\\Debug\\net10.0-windows\\MusicEngine.exe",
                "..\\..\\MusicEngine\\bin\\Release\\net10.0-windows\\MusicEngine.exe"
            };

            string path = Array.Find(candidates, File.Exists);
            if (path == null)
                return false;

            var asm = Assembly.LoadFrom(path);
            var synthType = asm.GetType("MusicEngine.Core.Synthesizers.OrganSynth");
            if (synthType == null)
                return false;

            using dynamic synth = Activator.CreateInstance(synthType)!;
            synth.NoteOn(0, 60, 100);
            Thread.Sleep(300);
            synth.NoteOff(0, 60);
            return true;
        }
        catch (Exception ex)
        {
            Console.WriteLine($"MusicEngine load failed: {ex.Message}");
            return false;
        }
    }
}

SoundSampler.Main();
