LED firmware
---

# Compiling
Compilation of the firmware is made using PlatformIO.

## Targets
Some predifined targets are avaiable.
As the final hardware is based on a Lilygo T7-S3, targets starting with this name is probably what you want :blush:.

# Beat detection
The beat detector is an experimental feature. It's not so easy to detect beats for every kind of music.
I made the detector algorithm mainly for EDM, techno and trance musics.

## Tuning
Four parameters are available for tuning the beat detector (available in configuration web page).

- Start frequency : Give the first FFT bin to compute the flux (lower means more bass)
- End frequency : Give the last FFT bin to compute the flux (lower means more bass). It should not be less than `start frequency`!
- Threshold : The spectral flux is computed using `start` and `end` frequencies. If the flux is more than `threshold` beat detection is enabled.
- Sensitivity : A difference between last value and actual is done. If this difference is < `sensitivity` and the flux is over  the `threshold`, a beat is detected.

Changing `start frequency` and/or `end frequency` will probably change the spectral flux. So, if you change these parameters, you may also adjust threshold.

### Working parameters
Here are some working parameters, I found (considering an audio input of maximum amplitude).

|Start freq|End freq|Threshold|Sensitivity|Remarks|
|----------|--------|---------|-----------|-------|
|20        |700     |7        |2          |       |
|20        |2000    |2        |1.5        | Not really reliable for EDM|
|80        |300     |12       |2          |       |