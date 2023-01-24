# Harmonizer

Jacob Collier-like harmonizer, written in C and open source free for use :sunglasses:

## Startup

At the moment the harmonizer can only run on Linux. This will change. It can't stay like this.

- Install jack (more about [here](docs/help.md#install-jack))
- Clone repository
```bash
git clone --recursive https://github.com/Bycob/harmonizer.git
```
- Build
```bash
mkdir build
cd build
cmake ..
make
```

## Test the software

```bash
cat /proc/asound/cards
# Select the sound card according to the output
scripts/start_jack.sh 1

# Run with audio interface & midi inferface
./harmonizer --midi_interface [the name of the interface]

# Run from audio sample with a midi file
./harmonizer --audio_input_file samples/test_sample_01.wav --midi_input_file samples/test_sample_01.mid

# If you don't support jack but still want to run the harmonizer,
# you can try the offline mode. Output will be saved to a file
./harmonizer --audio_input_file samples/test_sample_01.wav --midi_input_file samples/test_sample_01.mid --save_audio_output my_output.wav --no_play_audio
```

