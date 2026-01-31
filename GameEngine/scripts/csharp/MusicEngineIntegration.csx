// scripts/csharp/MusicEngineIntegration.csx
// Example: play a single note (C4) via MusicEngine when called.
// Adjust the reference path to your built DLL (Debug/Release AnyCPU).
#r "..\\..\\..\\..\\MusicEngine\\bin\\Debug\\net10.0-windows\\MusicEngine.dll"

using System;
using MusicEngine;
using MusicEngine.Core.Synthesizers;

public static class MusicEngineIntegration
{
    public static void PlayTestNote()
    {
        using var synth = new OrganSynth();
        synth.NoteOn(0, 60, 100); // channel 0, note 60 (C4), velocity 100
        System.Threading.Thread.Sleep(500);
        synth.NoteOff(0, 60);
    }
}

// Entry point for quick test
MusicEngineIntegration.PlayTestNote();
