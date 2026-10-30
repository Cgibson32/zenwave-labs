// Offline renderer: pushes test signals through DriftEngine / UmbraEngine and writes raw float32 stereo.
#include <zenwave/zenwave.h>
#include <cstdio>
#include <cstring>
#include <vector>
#include <string>
#include <cmath>
static void writeRaw(const char* path, const std::vector<float>& L, const std::vector<float>& R) {
    FILE* f = fopen(path, "wb");
    for (size_t i = 0; i < L.size(); ++i) { fwrite(&L[i], 4, 1, f); fwrite(&R[i], 4, 1, f); }
    fclose(f);
}
int main(int argc, char** argv) {
    const double sr = 48000.0; const int block = 256;
    std::string which = argc > 1 ? argv[1] : "drift";
    std::string sig   = argc > 2 ? argv[2] : "impulse";
    size_t N = size_t(sr * 6);
    std::vector<float> L(N, 0.0f), R(N, 0.0f);
    if (sig == "impulse") { L[100] = R[100] = 1.0f; }
    else if (sig == "drums") { // kick on beats, snare on 2/4, hats 8ths at 140 bpm, 2 bars
        const double beat = 60.0 / 140.0;
        for (int b = 0; b < 8; ++b) {
            size_t s0 = size_t(b * beat * sr);
            for (size_t i = 0; i < size_t(0.25 * sr) && s0 + i < N; ++i) { // kick
                double t = i / sr; float f = 150.0f * std::exp(-t * 30) + 45.0f;
                L[s0+i] += 0.8f * std::sin(2*M_PI*f*t) * std::exp(-t*8); }
            if (b % 2 == 1) for (size_t i = 0; i < size_t(0.15 * sr) && s0 + i < N; ++i) { // snare
                double t = i / sr; float n = (float(rand())/RAND_MAX*2-1);
                L[s0+i] += (0.5f*n + 0.4f*std::sin(2*M_PI*190*t)) * std::exp(-t*25); }
            for (int h = 0; h < 2; ++h) { size_t h0 = s0 + size_t(h * beat * 0.5 * sr);
                for (size_t i = 0; i < size_t(0.04 * sr) && h0 + i < N; ++i) { double t = i / sr;
                    L[h0+i] += 0.25f * (float(rand())/RAND_MAX*2-1) * std::exp(-t*80); } }
        }
        R = L;
    }
    else if (sig == "chord") { for (size_t i = 0; i < size_t(sr*1.0); ++i) { double t = i/sr;
        float env = std::min(1.0, t*50) * (t > 0.8 ? std::exp(-(t-0.8)*20) : 1.0);
        L[i] = R[i] = 0.3f*env*(std::sin(2*M_PI*220*t)+std::sin(2*M_PI*277.18*t)+std::sin(2*M_PI*329.63*t)); } }

    if (which == "dry") { }
    else if (which == "drift") {
        zw::DriftEngine e; e.prepare(sr, block); zw::DriftParams p; e.setParams(p);
        for (size_t i = 0; i < N; i += block) e.process(L.data()+i, R.data()+i, int(std::min<size_t>(block, N-i)));
    } else {
        zw::UmbraEngine e; e.prepare(sr, block); zw::UmbraParams p; e.setParams(p);
        for (size_t i = 0; i < N; i += block) e.process(L.data()+i, R.data()+i, int(std::min<size_t>(block, N-i)));
    }
    writeRaw(argc > 3 ? argv[3] : "out.f32", L, R);
    return 0;
}
