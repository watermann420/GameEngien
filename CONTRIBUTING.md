# Contributing to MusicEngineEditor

Welcome to **MusicEngineEditor**! This guide will help you understand the project structure, coding conventions, and how to contribute effectively.



---

## Project Overview

**MusicEngineEditor** is a visual code editor for the **MusicEngine** audio scripting system. It provides a professional IDE-like experience for creating music through code.

### Core Components

| Component | Description | Status |
|-----------|-------------|--------|
| **MusicEngine** (Core) | The audio engine - written by hand | Stable |
| **MusicEngineEditor** | Visual editor/IDE - AI-enhanced | Active Development |

> **Note:** The core MusicEngine was written manually. The Editor and many features are AI-assisted and may still be rough around the edges. Contributions to improve stability are welcome!

---

## Project Structure

```
MusicEngineEditor/
├── MusicEngineEditor/           # Main Editor Application
│   ├── Controls/                # UI Controls (meters, panels, visualizations)
│   ├── Editor/                  # Code editor components
│   │   ├── CompletionProvider.cs    # Autocomplete
│   │   ├── NumberDetector.cs        # Inline slider detection
│   │   ├── StrudelInlineSlider.cs   # Strudel-style sliders
│   │   └── LiveParameterSystem.cs   # Real-time parameter updates
│   ├── Models/                  # Data models
│   ├── Services/                # Business logic services
│   ├── ViewModels/              # MVVM ViewModels
│   ├── Views/                   # XAML Views
│   └── Themes/                  # Dark/Light themes
├── MusicEngineEditor.Tests/     # Unit tests
└── docs/                        # Documentation

MusicEngine/                     # Core Audio Engine (separate repo)
├── MusicEngine/
│   └── Core/
│       ├── Sequencer.cs         # Main sequencer
│       ├── Pattern.cs           # Pattern management
│       ├── NoteEvent.cs         # Note events
│       ├── AdvancedSynth.cs     # Synthesizers
│       ├── Effects/             # Audio effects
│       ├── Analysis/            # Audio analysis
│       └── Modulation/          # VCV Rack-style modulation
```

---

## Getting Started

### Prerequisites

- .NET 10 SDK
- Visual Studio 2022 / JetBrains Rider
- MusicEngine (referenced project)

### Build & Run

```bash
# Clone the repository
git clone https://github.com/watermann420/MusicEngineEditor.git

# Build
dotnet build

# Run
dotnet run --project MusicEngineEditor
```

Or simply double-click `StartEditor.bat`.

---

## Syntax Guide

### Basic Script Structure

```csharp
// Every MusicEngine script follows this pattern:

// 1. Create a sequencer
var seq = new Sequencer();
seq.Bpm = 120;

// 2. Create instruments
var synth = new SimpleSynth();
var drums = new DrumMachine();

// 3. Create patterns
var melody = seq.CreatePattern("melody", synth);
var beat = seq.CreatePattern("beat", drums);

// 4. Add notes
melody.Note(60, 0, 0.5, 100);    // C4 at beat 0, half beat, velocity 100
melody.Note(64, 0.5, 0.5, 100);  // E4 at beat 0.5
melody.Note(67, 1, 0.5, 100);    // G4 at beat 1

// 5. Play
seq.Play();
```

### Note Method Syntax

```csharp
// Full signature:
pattern.Note(pitch, beat, duration, velocity);

// pitch:    MIDI note number (0-127) or note name
// beat:     When to play (in beats, 0 = start)
// duration: How long to play (in beats)
// velocity: How hard to play (0-127)

// Examples:
pattern.Note(60, 0, 1, 100);      // C4, at beat 0, 1 beat long, velocity 100
pattern.Note("C4", 0, 1, 100);    // Same using note name
pattern.Note(60, 0, 0.25, 80);    // 16th note
pattern.Note(60, 0, 4, 127);      // Whole note, max velocity
```

### Chord Shorthand

```csharp
// Play multiple notes at once
pattern.Chord(new[] { 60, 64, 67 }, 0, 1, 100);  // C major chord

// Or use chord names
pattern.Chord("Cmaj", 0, 1, 100);
pattern.Chord("Am7", 1, 1, 100);
pattern.Chord("Dm", 2, 1, 100);
pattern.Chord("G7", 3, 1, 100);
```

### Loops and Patterns

```csharp
// Loop a section
for (int i = 0; i < 4; i++)
{
    pattern.Note(60, i * 4, 0.5, 100);
    pattern.Note(67, i * 4 + 1, 0.5, 100);
    pattern.Note(72, i * 4 + 2, 0.5, 100);
}

// Using pattern length
pattern.Length = 4;  // 4 beats per loop
pattern.Loop = true;
```

---

## Code Examples

### Example 1: Simple Melody

```csharp
var seq = new Sequencer { Bpm = 100 };
var piano = new SimpleSynth();
var melody = seq.CreatePattern("melody", piano);

// Twinkle Twinkle Little Star
int[] notes = { 60, 60, 67, 67, 69, 69, 67 };
for (int i = 0; i < notes.Length; i++)
{
    melody.Note(notes[i], i * 0.5, 0.5, 90);
}

seq.Play();
```

### Example 2: Drum Pattern

```csharp
var seq = new Sequencer { Bpm = 120 };
var drums = new DrumMachine();
var beat = seq.CreatePattern("drums", drums);

// 4-beat drum loop
beat.Length = 4;
beat.Loop = true;

// Kick on 1 and 3
beat.Note(36, 0, 0.5, 100);    // Kick
beat.Note(36, 2, 0.5, 100);

// Snare on 2 and 4
beat.Note(38, 1, 0.5, 100);    // Snare
beat.Note(38, 3, 0.5, 100);

// Hi-hat every half beat
for (double i = 0; i < 4; i += 0.5)
{
    beat.Note(42, i, 0.25, 60);  // Closed hi-hat
}

seq.Play();
```

### Example 3: Synthesizer with Modulation

```csharp
var seq = new Sequencer { Bpm = 128 };

// Create modular synth
var synth = new AdvancedSynth(3);  // 3 oscillators
synth.SetOscillator(0, WaveType.Sawtooth, 0.6f, 0);
synth.SetOscillator(1, WaveType.Square, 0.4f, 7);   // Detuned
synth.FilterType = SynthFilterType.MoogLadder;
synth.FilterCutoff = 0.5f;
synth.FilterResonance = 0.4f;

// Envelope settings
synth.Attack = 0.01;
synth.Decay = 0.2;
synth.Sustain = 0.6;
synth.Release = 0.3;

var pattern = seq.CreatePattern("lead", synth);
pattern.Note(60, 0, 2, 100);

seq.Play();
```

### Example 4: Using Effects

```csharp
var seq = new Sequencer { Bpm = 90 };
var synth = new SimpleSynth();

// Add effects chain
var delay = new Delay { Time = 0.375, Feedback = 0.4, Mix = 0.3 };
var reverb = new Reverb { RoomSize = 0.7, Damping = 0.5, Mix = 0.25 };

synth.AddEffect(delay);
synth.AddEffect(reverb);

var pattern = seq.CreatePattern("ambient", synth);
pattern.Note(60, 0, 4, 80);

seq.Play();
```

### Example 5: VCV Rack-style Modulation

```csharp
// Using the modular parameter system
var synth = new ModularSynthBase();

// Get parameters
var cutoff = synth.GetParameter("cutoff");
var resonance = synth.GetParameter("resonance");

// Create LFO modulation source
var lfo = new ModularLFO("lfo1", "Filter LFO", 44100);
lfo.Rate.Value = 0.5;      // 0.5 Hz
lfo.Depth.Value = 0.8;     // 80% depth
lfo.Waveform.Value = 0;    // Sine

// Connect LFO to filter cutoff
synth.Connect(lfo, cutoff, 0.5);  // 50% modulation amount

// Create envelope for resonance
var env = new ModularEnvelope("env1", "Res Env", 44100);
env.Attack.Value = 0.1;
env.Decay.Value = 0.3;
env.Sustain.Value = 0.2;
env.Release.Value = 0.5;

synth.Connect(env, resonance, 0.7);
```

---

## Editor Features

### Inline Sliders (Strudel-style)

Numbers in your code are interactive:

```csharp
synth.FilterCutoff = 0.5f;  // <- Drag left/right to change!
seq.Bpm = 120;              // <- Drag to change BPM in real-time!
```

- **Hover** over numbers to see the slider
- **Drag left/right** to change values
- **Shift+Drag** for fine control
- **Ctrl+Drag** for coarse control
- **Escape** to cancel

### Slider Annotations

Add custom slider ranges with comments:

```csharp
var volume = 0.8;    // @slider(0, 1, 0.01, "Volume")
var bpm = 120;       // @slider(60, 200, 1, "BPM")
var cutoff = 1000;   // @slider(20, 20000, 1, "Cutoff Hz")
```

### Live Code Visualization

When patterns are playing:
- Active code blocks glow
- Playing notes highlight
- Inactive patterns are dimmed

### Autocomplete

The editor provides intelligent autocomplete for:
- MusicEngine classes and methods
- Parameter names
- Note names (C4, D#5, etc.)
- Effect types
- Synth presets

---

## Important Notes

### AI-Assisted Development

This project uses AI assistance for feature development. This means:

1. **Some features may be rough** - Please report bugs!
2. **Code quality varies** - Refactoring PRs are welcome
3. **Documentation may lag** - Help improve it!

### Known Areas for Improvement

- [ ] Error handling in some edge cases
- [ ] Performance optimization for large projects
- [ ] More comprehensive unit tests
- [ ] Better undo/redo support

### The Core Engine

The **MusicEngine** core library was written manually and is the stable foundation. The editor builds on top of it.

---

## Contribution Guidelines

### Code Style

```csharp
// Use clear, descriptive names
public class PatternEditor { }  // Good
public class PE { }             // Bad

// XML documentation for public APIs
/// <summary>
/// Creates a new pattern with the specified instrument.
/// </summary>
/// <param name="name">The pattern name.</param>
/// <param name="instrument">The instrument to use.</param>
/// <returns>The created pattern.</returns>
public Pattern CreatePattern(string name, ISynth instrument)

// Use var when type is obvious
var synth = new SimpleSynth();  // Good
SimpleSynth synth = new SimpleSynth();  // Also fine

// Prefer expression bodies for simple members
public string Name => _name;
public int Count => _items.Count;
```

### Pull Request Process

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/my-feature`
3. Make your changes
4. Write/update tests if applicable
5. Ensure all tests pass
6. Submit a pull request

### Commit Messages

```
feat: Add new drum pattern generator
fix: Resolve audio buffer overflow issue
docs: Update synth parameter documentation
refactor: Simplify pattern loop logic
test: Add unit tests for Sequencer
```

### Reporting Issues

When reporting bugs, please include:
- Steps to reproduce
- Expected vs actual behavior
- Error messages (if any)
- OS and .NET version

---

## Legal

By contributing code, you agree to the [Contributor Agreement](CONTRIBUTOR_AGREEMENT.md) which assigns code rights to MusicEngineEditor while allowing you to retain ownership of ideas.

---

## Questions?

- Open an issue for bugs or feature requests
- Check existing issues before creating new ones
- Be respectful and constructive

Thank you for contributing to MusicEngineEditor!
