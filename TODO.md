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
    * a 0dB taper is obtained by means of a conditional check on the panning value, with one channel always with x1 gain (i.e. the
      center channel has always value 1)
  * on a stereo source we should mix the channels? That is, when panning left should some of the right channel "bleed" to the left channel?
    * due to the complexity of the real stereo source, probably we should limit to panning w/o bleeding
    * stereo balance, mono panning? truth is that if mono are threated as stereo with L equal to R, then balance is panning!
    * given this, what if we split a stereo track into two mono tracks, then we apply panning individually? cool! (http://www.cloneensemble.com/sb_main.htm)
    * can we automate the stereo track split and independent panning? A panning matrix can do this...
      * always use -3dB for mono and 0dB for stereo?
    * we could also support *dual-pan* mode, i.e. we are using a pair of pan pots (one per channel). The single pot case can
      be reduced to it (the pots are synched and only the corresponding gain is used)
  * [2020-04-08] decided for a minimal approach. The audio system expose a generic "mix" structure than can implement
    almost every type of pan/dual-pan/balance. The actual values will be fed from the Lua API.
  * [2020-04-10] it works! initially not working, apparently because on my office PC the audio card has an internal sample-rate of 48KHz that
    makes the audio stutter. decided to detect and use the card internal sample rate (TBD) to avoid confusion. tried waveform synthesis, using the
    already written wave functions... and decided that they are simpler (faster?) than miniaudio's due to less overhead. should I opt for audio
    synthesis only? should also enumerate device and let the user pick the wanted one (in case multiple outputs are available).
  * filed a possible bug found on a Linux Mint 19.3 VM. I should try on more machines.
    * it turned out as a bug, fixed! cool!
  * can't decide sample frequence, 44100 probably (no more than 48000, anyway).
  * did some study on FM synthesis. interesting stuff! way too complex to handle for the final product, I don't want to
    waste a lot of time in writing music (cool part is the near to zero memory usage). maybe on a second step.
  * panning laws. are they worth? doesn't linear panning (i.e. constant-gain) suit (or at most constant power sincos)?
    * found a very strange sincos constant power formula, basically a rotation scaled by sqrt(2)/2... which is the same a using plain sin/cos! WTF!
  * can be worth using wavetables and pantables? I don't think so...
  * creating sound groups can be crucial to handle them collectively, e.g. lower the volume of the group to leave room for other sounds
    * in this sense, the "group" is the audio equivalent of the "canvas"
    * "Bus" can be a valid name, too.
  * the more I work on it, the more I want to simplify
    * I'm using miniaudio only to access the device, somehow like I do with GLFW to access OpenGL
    * mixing and streaming is done by native engine code
    * I can luckily exploit the sound loaders that converts to float during the processing
    * WAV will be the format of choice, for the moment, since the `dr_wav` library integrates well with my FS abstraction (`std_vorbis` will
      require some adaptation)
  * trying always-streaming data. will some buffering be needed? new data could be pulled in the update call.
  * don't need to write a custom SRC, miniaudio has it all.
    * it also support sample-rate throttling passing a "ratio" value
    * definitely buffering will be needed.
  * hail to valgrind, was about to rewrite everything but thanks to it (once) again I found a very small distraction that caused the engine to go crazy.
  * OMG!!! it works!!!
    * need to cap the gain/pan values, however
    * also, it seems really heavy on the performances, should the device be started only when required?
  * remove the group idea from the sound API. Not needed, since if we want to handle group of sound we can just make a Lua table of them.
  * (BUG) fixed the pcall stack dump not working, being relative to the caller not the called function.
  * re-adding group support, but this time in form of a table of "mix values" (with balance a gain control).
  * switching to FLAC format, compressed lossless analogue of the PNG format.
  * fixed bug that cause individual sources not to add-up values in the final mix.
  * definitely ditched cross-mixing... or not?
  * fixed warning in Windows build for miniaudio.
  * definitely I need to use a ring-buffer to stream audio. We could be tempted to always stream w/o buffers, having SDD and such... but that's not the case in general. Miniaudio has a ring-buffer API.
    * tried miniaudio ringbuffer, but it's easier to buffer myself.
  * no OOP, just grouping the shared properties.
  * naming: create/destroy, register/deregister, init(ialize)/deinit(ialize). "un-" prefix for the adjectives, "de-" for verbs.
  * at last I'm going for a full opaque approach, like for the file-system (somewhat hybrid)
    * cool part is that I can have a single "Sound.Source" in Lua!
  * updated the miniaudio library, only to find out that an audio-buffer has been implemented, pretty much identical to mine (which was already designed similar to the ring-buffer). Tiny hack to read the data, at first.
  * little stuttering in the Linux audio, will check if it depends on eOS.
    * turned out it depended on the VM itself. Adding the line `sound.virtualDev = "hdaudio"` to the VMX file solved it.
    * VMWare fixed this bug (and an annoying CAPS lock issue) in the new release. that explains a lot.
  * sample mixing was bugged, due to an unnecessary channel expansion during mix (1->2, while buffer was for 1 ch only).
    * added `sanitize` build that uses AddressSanitizer to detect bug.
  * handling error codes when updating audio. Still missing when rewinding/resetting a source.
  * updating the audio sub-system in the fixed steps loop, just as it was INT based.

```java
  //do panning
  double pan = music.getPan();
  if (pan != 0.0) {
    double ll = (pan <= 0.0) ? 1.0 : (1.0 - pan);
    double lr = (pan <= 0.0) ? Math.abs(pan) : 0.0;
    double rl = (pan >= 0.0) ? pan : 0.0;
    double rr = (pan >= 0.0) ? 1.0 : (1.0 - Math.abs(pan));
    double tmpL = (ll * leftCurr) + (lr * rightCurr);
    double tmpR = (rl * leftCurr) + (rr * rightCurr);
    leftCurr = tmpL;
    rightCurr = tmpR;
  }
```