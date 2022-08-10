# simple-services/wav-file

## Overview

The wav-file service is a utility for reading and writing to wave files.

## Required components

- None

## Recommended components

- umm_malloc

## Integrate the source

- Copy the 'src' directory into an appropriate place in the host project
- Copy the 'inc' directory into a project include directory.  The header
  file in the 'inc' directory contain the configurable options for the
  wav-file service.

## Configure

See 'inc/wav_file_cfg.h' for an example utilizing 'umm_malloc' along with
the other configurable parameters.

## Reading WAV files

- Create a new WAV_FILE object and zero it out
- Set the 'fname' element with the name of the file and 'isSrc' to
  true.
- Call openWave().  If successful, call readWave() to read audio from
  the file.
- Call closeWave() when finished.

## Writing WAV files

- Create a new WAV_FILE object and zero it out
- Set the 'fname' element with the name of the file and 'isSrc' to
  false.
- Additionally set 'channels', 'sampleRate', 'wordSizeBytes', and
  'frameSizeBytes' (normally channels * wordSizeBytes)
- Call openWave().  If successful, call writeWave() to write audio to
  the file.
- Call closeWave() when finished.

## Info

- Wave files opened for read will loop once the end of file is reached.
- Wave files greater then 4 GiB are not supported
- The 'samples' parameter in both waveRead() and waveWrite() is the
  size of the supplied data buffer in samples, noe frames. The application
  should write data in 'channels' sized sample chunks to ensure full frames
  are always written.

