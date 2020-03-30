# Panning

* started with `miniaudio`, a bit convoluted
* consulted `raudio`, which isn't bad at all and uses `miniaudio`
* sounds are fed to the audio subsystem from a callback
  * the callback will feed from memory or from file, either case we are using a "reader"
* implement panning
  * panning a stereo source is called balance and is treated separately;
  * better, `balance` is a property of the master device;
  * all sources are either MONO and are converted to STEREO during loading or are STEREO by design;
  * we implement some different pan laws (linear, sin/cos, sqrt) with different dB levels.
    * linear taper is -6db on center
    * sin/cos and sqrt tapers are -3dB at the center
  * once a taper is devised, we can calculate the "center dB" value with the formula `dB = log(level) * 20`
  * a taper is defined as "constant power" if its sum of the squares is always equal to 1 and it's equal to -3dB
  * we can tweak each taper and get -1.5dB, -3dB, -4.5dB, and -6dB by using suitable powers
    * the sqrt taper is a tweaked version of the linear one
  * on a stereo source we should mix the channels? That is, when panning left should some of the right channel "bleed" to the left channel?

