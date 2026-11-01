#!/usr/bin/env python3
"""
vox2table - turn real vocal recordings into NEBULA sampled wavetables.

Two inputs:
  --wav FILE [FILE ...]   sustained vocal recordings (mono/stereo WAV, any rate). Each file becomes one table:
                          the voice is pitch-tracked, 32 harmonic snapshots are taken across the stable, voiced
                          part of the take (ordered in time, so POSITION replays the singer's own motion), and the
                          residual breath spectrum is measured.
  --cycles DIR            a folder of single-cycle waveforms (AKWF style). Cycles are ordered into a smooth morph
                          path by spectral similarity and split into tables of 32 frames.

Output: a C++ header (default plugins/nebula/src/SampledTables.h) holding, per table, the harmonic spectrum of
each frame at the recording's reference pitch (uint8, 0.5 dB steps) plus a breath (noise) envelope. NEBULA
re-samples these spectra at the played pitch per octave band, so the singer's formants stay where they were
recorded instead of transposing with the note.

Only use recordings you have the rights to turn into a product (your own sessions, hired singers, CC0 material).
"""
import argparse, glob, math, os, struct, sys, wave
import numpy as np

PARTIALS = 96
FRAMES = 32
BREATH_BANDS = 16   # log-spaced 200 Hz .. 12 kHz


def read_wav(path):
    with wave.open(path, 'rb') as w:
        sr, ch, sw, n = w.getframerate(), w.getnchannels(), w.getsampwidth(), w.getnframes()
        raw = w.readframes(n)
    if sw == 2:
        x = np.frombuffer(raw, dtype=np.int16).astype(np.float64) / 32768.0
    elif sw == 3:
        a = np.frombuffer(raw, dtype=np.uint8).reshape(-1, 3).astype(np.int32)
        v = a[:, 0] | (a[:, 1] << 8) | (a[:, 2] << 16)
        v = np.where(v >= 2 ** 23, v - 2 ** 24, v)
        x = v.astype(np.float64) / 2 ** 23
    elif sw == 4:
        x = np.frombuffer(raw, dtype=np.int32).astype(np.float64) / 2 ** 31
    else:
        raise SystemExit(f'{path}: unsupported sample width {sw}')
    if ch > 1:
        x = x.reshape(-1, ch).mean(axis=1)
    return x, sr


# ---------------------------------------------------------------- pitch tracking (YIN-lite) --------------
def yin_pitch(frame, sr, fmin=70.0, fmax=1200.0):
    n = len(frame)
    tau_min, tau_max = int(sr / fmax), int(sr / fmin)
    if tau_max >= n // 2:
        tau_max = n // 2 - 1
    f = frame - frame.mean()
    # difference function via autocorrelation
    ac = np.fft.irfft(np.abs(np.fft.rfft(f, 2 * n)) ** 2)[:n]
    energy = np.cumsum(f * f)
    d = np.zeros(n)
    for tau in range(1, tau_max + 1):
        d[tau] = energy[n - tau - 1] + (energy[-1] - energy[tau - 1]) - 2 * ac[tau]
    cmnd = np.ones(n)
    run = 0.0
    for tau in range(1, tau_max + 1):
        run += d[tau]
        cmnd[tau] = d[tau] * tau / (run + 1e-12)
    best, best_val = -1, 1.0
    for tau in range(tau_min, tau_max):
        if cmnd[tau] < 0.15 and cmnd[tau] <= cmnd[tau + 1]:
            best, best_val = tau, cmnd[tau]
            break
    if best < 0:
        best = int(np.argmin(cmnd[tau_min:tau_max]) + tau_min)
        best_val = cmnd[best]
    # parabolic refine
    if 1 < best < n - 1:
        a, b, c = cmnd[best - 1], cmnd[best], cmnd[best + 1]
        den = a - 2 * b + c
        if abs(den) > 1e-12:
            best += 0.5 * (a - c) / den
    return sr / best, best_val


def harmonic_snapshot(x, sr, centre, f0, partials=PARTIALS, periods=8):
    """amplitude of harmonics 1..partials around sample 'centre' using a window of 'periods' periods"""
    n = int(periods * sr / f0)
    n = max(256, n)
    a, b = centre - n // 2, centre + n // 2
    if a < 0 or b > len(x):
        return None
    seg = x[a:b] * np.hanning(b - a)
    nfft = 1 << int(math.ceil(math.log2(len(seg) * 4)))
    sp = np.abs(np.fft.rfft(seg, nfft))
    freqs = np.fft.rfftfreq(nfft, 1.0 / sr)
    out = np.zeros(partials)
    for h in range(1, partials + 1):
        fh = h * f0
        if fh > sr * 0.45:
            break
        lo, hi = np.searchsorted(freqs, fh * (1 - 0.25 / h)), np.searchsorted(freqs, fh * (1 + 0.25 / h))
        if hi <= lo:
            hi = lo + 1
        out[h - 1] = sp[lo:hi].max()
    return out


def breath_envelope(x, sr, f0_track):
    """spectrum of what is left after the harmonics: median of the between-harmonic valleys per log band"""
    bands = np.geomspace(200.0, 12000.0, BREATH_BANDS + 1)
    acc = np.zeros(BREATH_BANDS)
    cnt = 0
    hop = int(sr * 0.05)
    n = 4096
    for i in range(0, len(x) - n, hop):
        f0 = f0_track(i + n // 2)
        if f0 is None:
            continue
        seg = x[i:i + n] * np.hanning(n)
        sp = np.abs(np.fft.rfft(seg))
        freqs = np.fft.rfftfreq(n, 1.0 / sr)
        # keep only bins that sit between harmonics (more than 35 % of f0 away from any harmonic)
        frac = (freqs / f0) % 1.0
        valley = (frac > 0.35) & (frac < 0.65)
        for b in range(BREATH_BANDS):
            m = valley & (freqs >= bands[b]) & (freqs < bands[b + 1])
            if m.any():
                acc[b] += np.median(sp[m])
        cnt += 1
    if cnt == 0:
        return np.zeros(BREATH_BANDS)
    acc /= cnt
    return acc / (acc.max() + 1e-12)


def table_from_recording(path, name=None):
    x, sr = read_wav(path)
    x = x / (np.abs(x).max() + 1e-9)
    win = int(sr * 0.04)
    hop = int(sr * 0.01)
    times, pitches, conf, rms = [], [], [], []
    for i in range(0, len(x) - win, hop):
        fr = x[i:i + win]
        r = math.sqrt(float(np.mean(fr * fr)))
        if r < 0.02:
            continue
        p, c = yin_pitch(fr, sr)
        times.append(i + win // 2); pitches.append(p); conf.append(c); rms.append(r)
    if not times:
        raise SystemExit(f'{path}: no voiced material found')
    times, pitches, conf, rms = map(np.array, (times, pitches, conf, rms))
    good = conf < 0.2
    if good.sum() < 8:
        good = conf < np.percentile(conf, 50)
    ref = float(np.exp(np.median(np.log(pitches[good]))))
    # stable = within 1.5 semitones of the reference and confident
    stable = good & (np.abs(12 * np.log2(pitches / ref)) < 1.5)
    idx = np.where(stable)[0]
    if len(idx) < FRAMES:
        idx = np.where(good)[0]
    pick = np.linspace(0, len(idx) - 1, FRAMES).round().astype(int)
    frames = []
    for k in pick:
        i = idx[k]
        snap = harmonic_snapshot(x, sr, int(times[i]), float(pitches[i]))
        if snap is None:
            snap = frames[-1] if frames else np.zeros(PARTIALS)
        frames.append(snap)
    frames = np.array(frames)
    # light smoothing along the frame axis so POSITION glides instead of flickering
    sm = frames.copy()
    for f in range(FRAMES):
        lo, hi = max(0, f - 1), min(FRAMES, f + 2)
        sm[f] = frames[lo:hi].mean(axis=0)
    frames = sm

    def f0_at(sample):
        j = int(np.argmin(np.abs(times - sample)))
        return float(pitches[j]) if stable[j] else None
    breath = breath_envelope(x, sr, f0_at)
    nm = name or os.path.splitext(os.path.basename(path))[0]
    print(f'{path}: ref {ref:.1f} Hz ({12*math.log2(ref/440)+69:.1f} MIDI), {int(stable.sum())} stable frames of {len(times)}')
    return nm, ref, frames, breath


def tables_from_cycles(folder, prefix, per_table=FRAMES, ref_hz=220.0, max_tables=None):
    files = sorted(glob.glob(os.path.join(folder, '*.wav')))
    specs = []
    for f in files:
        x, sr = read_wav(f)
        sp = np.abs(np.fft.rfft(x))[1:PARTIALS + 1]
        if len(sp) < PARTIALS:
            sp = np.pad(sp, (0, PARTIALS - len(sp)))
        specs.append(sp / (sp.max() + 1e-12))
    S = np.array(specs)
    L = np.log10(S + 1e-4)
    # greedy nearest-neighbour path through the set -> smooth morph order
    used = np.zeros(len(S), bool)
    order = [int(np.argmin((S * np.arange(1, PARTIALS + 1)).sum(1) / S.sum(1)))]   # start with the darkest
    used[order[0]] = True
    while not used.all():
        d = ((L - L[order[-1]]) ** 2).sum(1)
        d[used] = np.inf
        j = int(np.argmin(d)); order.append(j); used[j] = True
    S = S[order]
    n_tables = max(1, len(S) // per_table)
    if max_tables:
        n_tables = min(n_tables, max_tables)
    out = []
    for t in range(n_tables):
        chunk = S[t * len(S) // n_tables:(t + 1) * len(S) // n_tables]
        pos = np.linspace(0, len(chunk) - 1, per_table)
        frames = np.array([chunk[int(p)] * (1 - (p - int(p))) + chunk[min(len(chunk) - 1, int(p) + 1)] * (p - int(p)) for p in pos])
        out.append((f'{prefix} {["I", "II", "III", "IV", "V", "VI"][t]}', ref_hz, frames, np.zeros(BREATH_BANDS)))
    return out


def write_header(tables, path):
    def q(v):  # amplitude -> uint8 in 0.5 dB steps below 0 dB (0 = -127.5 dB and below)
        db = 20 * math.log10(max(v, 1e-7))
        return int(max(0, min(255, round(255 + db * 2))))
    lines = ['// Generated by tools/vox2table.py - sampled vocal wavetables (harmonic spectra per frame at a reference pitch).',
             '// Do not edit by hand; re-run the tool on the source recordings.', '#pragma once', '#include <cstdint>', '',
             'namespace nebula {', f'constexpr int kSampledPartials = {PARTIALS}, kSampledFrames = {FRAMES}, kBreathBands = {BREATH_BANDS};',
             'struct SampledTable { const char* name; float refHz; uint8_t db[kSampledFrames][kSampledPartials]; uint8_t breath[kBreathBands]; };',
             'inline const SampledTable kSampledTables[] = {']
    for name, ref, frames, breath in tables:
        lines.append(f'  {{ "{name}", {ref:.2f}f, {{')
        for fr in frames:
            fr = fr / (fr.max() + 1e-12)
            lines.append('    { ' + ', '.join(str(q(v)) for v in fr) + ' },')
        lines.append('  }, { ' + ', '.join(str(q(max(v, 1e-7))) for v in breath) + ' } },')
    lines += ['};', 'constexpr int kSampledTableCount = int(sizeof(kSampledTables) / sizeof(kSampledTables[0]));', '} // namespace nebula', '']
    with open(path, 'w') as f:
        f.write('\n'.join(lines))
    print(f'wrote {path}: {len(tables)} tables')


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--wav', nargs='*', default=[], help='sustained vocal recordings, one table each')
    ap.add_argument('--name', nargs='*', default=[], help='table names for --wav inputs (same order)')
    ap.add_argument('--cycles', help='folder of single-cycle WAVs')
    ap.add_argument('--cycles-prefix', default='Vox')
    ap.add_argument('--cycles-ref', type=float, default=220.0, help='assumed reference pitch of the cycles (Hz)')
    ap.add_argument('--max-tables', type=int, default=None)
    ap.add_argument('-o', '--out', default=os.path.join(os.path.dirname(__file__), '..', 'plugins', 'nebula', 'src', 'SampledTables.h'))
    a = ap.parse_args()
    tables = []
    for i, w in enumerate(a.wav):
        tables.append(table_from_recording(w, a.name[i] if i < len(a.name) else None))
    if a.cycles:
        tables += tables_from_cycles(a.cycles, a.cycles_prefix, ref_hz=a.cycles_ref, max_tables=a.max_tables)
    if not tables:
        ap.error('give --wav files and/or --cycles folder')
    write_header(tables, os.path.abspath(a.out))


if __name__ == '__main__':
    main()
