# NEXUS Audio

NEXUS 0.7 uses two audio paths.

## WAV / Sound Effects

Standard PCM WAV files are parsed by the NEXUS runtime itself and sent to the SDL2 core audio device. This path does not require SDL2_mixer.

Supported without SDL2_mixer:

- PCM WAV
- 8-bit or 16-bit
- mono or stereo

Example:

```nx
let sound = gfx_sound_load("assets/beep.wav")
if sound == 0 {
    print(gfx_audio_error())
    return
}
gfx_sound_play(sound, false)
sleep(400)
gfx_sound_stop()
```

## Music

`gfx_music_load` first tries the optional SDL2_mixer backend. If it is unavailable, a PCM WAV is loaded using the core audio backend. MP3/OGG music therefore requires SDL2_mixer, while WAV remains dependency-light.

## Troubleshooting

Check whether the SDL2 runtime can be loaded:

```bash
ldconfig -p | grep libSDL2
```

Check the NEXUS backend:

```nx
print(str_i64(gfx_audio_available()))
print(gfx_audio_error())
```

On headless systems, use:

```bash
SDL_AUDIODRIVER=dummy ./your_program
```

This validates decoding and audio queueing without a physical speaker.
