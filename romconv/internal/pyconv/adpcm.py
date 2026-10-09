#!/usr/bin/env python3
# Shared 4-bit IMA-ADPCM encoder for machine sample sets (Vanguard, Fantasy,
# Zaxxon, ...). Output format: 2-byte little-endian initial predictor
# (int16, matches the first PCM sample << 8), then one nibble per remaining
# PCM sample, low nibble first. Decoded inline per-sample in audio.cpp
# (ima_adpcm_decode) -- no heap, no whole-buffer decompression.
import numpy as np

STEP_TABLE = [7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,
    50,55,60,66,73,80,88,97,107,118,130,143,157,173,190,209,230,253,
    279,307,337,371,408,449,494,544,598,658,724,796,876,963,1060,1166,
    1282,1411,1552,1707,1878,2066,2272,2499,2749,3024,3327,3660,4026,
    4428,4871,5358,5894,6484,7132,7845,8630,9493,10442,11487,12635,
    13899,15289,16818,18500,20350,22385,24623,27086,29794,32767]
INDEX_TABLE = [-1,-1,-1,-1,2,4,6,8,-1,-1,-1,-1,2,4,6,8]

def ima_encode(pcm: np.ndarray) -> bytes:
    predictor = int(pcm[0]) << 8
    first = predictor & 0xffff
    output = bytearray((first & 255, first >> 8))
    step_index, pending = 0, None
    for value in pcm[1:]:
        target, step = int(value) << 8, STEP_TABLE[step_index]
        difference = target - predictor
        code = 8 if difference < 0 else 0
        difference = abs(difference)
        delta = step >> 3
        if difference >= step: code |= 4; difference -= step; delta += step
        if difference >= step >> 1: code |= 2; difference -= step >> 1; delta += step >> 1
        if difference >= step >> 2: code |= 1; delta += step >> 2
        predictor += -delta if code & 8 else delta
        predictor = max(-32768, min(32767, predictor))
        step_index = max(0, min(88, step_index + INDEX_TABLE[code]))
        if pending is None: pending = code
        else: output.append(pending | (code << 4)); pending = None
    if pending is not None: output.append(pending)
    return bytes(output)
