/*
 * Airwindows Reverb DSP port.
 * Original plugin source copyright Airwindows, MIT license.
 * Source: https://github.com/airwindows/airwindows/tree/master/plugins/LinuxVST/src/Reverb
 */
#pragma once

#include <array>
#include <cmath>
#include <cstdint>

class AirwindowsReverb {
public:
    explicit AirwindowsReverb(double sampleRate);

    void setSampleRate(double sampleRate);
    void set(double size, double wet);
    void clear();
    void process(double inL, double inR, double& outL, double& outR);

private:
    static constexpr double kPi = 3.14159265358979323846264338327950288;
    static constexpr double kGolden = 1.618033988749894848204586;
    static constexpr double kInvGolden = 0.618033988749894848204586;

    double sampleRate_ = 44100.0;
    double sizeParam_ = 0.5;
    double wetParam_ = 0.0;
    double regen_ = 0.0;
    double blend_ = 0.0;
    double dryMix_ = 1.0;
    double wetMix_ = 0.0;

    std::array<double, 11> biquadA_{};
    std::array<double, 11> biquadB_{};
    std::array<double, 11> biquadC_{};

    std::array<double, 8111> aAL_{};
    std::array<double, 7511> aBL_{};
    std::array<double, 7311> aCL_{};
    std::array<double, 6911> aDL_{};
    std::array<double, 6311> aEL_{};
    std::array<double, 6111> aFL_{};
    std::array<double, 5511> aGL_{};
    std::array<double, 4911> aHL_{};
    std::array<double, 4511> aIL_{};
    std::array<double, 4311> aJL_{};
    std::array<double, 3911> aKL_{};
    std::array<double, 3311> aLL_{};
    std::array<double, 3111> aML_{};

    std::array<double, 8111> aAR_{};
    std::array<double, 7511> aBR_{};
    std::array<double, 7311> aCR_{};
    std::array<double, 6911> aDR_{};
    std::array<double, 6311> aER_{};
    std::array<double, 6111> aFR_{};
    std::array<double, 5511> aGR_{};
    std::array<double, 4911> aHR_{};
    std::array<double, 4511> aIR_{};
    std::array<double, 4311> aJR_{};
    std::array<double, 3911> aKR_{};
    std::array<double, 3311> aLR_{};
    std::array<double, 3111> aMR_{};

    int countA_ = 1;
    int countB_ = 1;
    int countC_ = 1;
    int countD_ = 1;
    int countE_ = 1;
    int countF_ = 1;
    int countG_ = 1;
    int countH_ = 1;
    int countI_ = 1;
    int countJ_ = 1;
    int countK_ = 1;
    int countL_ = 1;
    int countM_ = 1;

    int delayA_ = 79;
    int delayB_ = 73;
    int delayC_ = 71;
    int delayD_ = 67;
    int delayE_ = 61;
    int delayF_ = 59;
    int delayG_ = 53;
    int delayH_ = 47;
    int delayI_ = 43;
    int delayJ_ = 41;
    int delayK_ = 37;
    int delayL_ = 31;
    int delayM_ = 29;

    double feedbackAL_ = 0.0;
    double feedbackBL_ = 0.0;
    double feedbackCL_ = 0.0;
    double feedbackDL_ = 0.0;
    double feedbackEL_ = 0.0;
    double feedbackFL_ = 0.0;
    double feedbackGL_ = 0.0;
    double feedbackHL_ = 0.0;
    double feedbackAR_ = 0.0;
    double feedbackBR_ = 0.0;
    double feedbackCR_ = 0.0;
    double feedbackDR_ = 0.0;
    double feedbackER_ = 0.0;
    double feedbackFR_ = 0.0;
    double feedbackGR_ = 0.0;
    double feedbackHR_ = 0.0;

    double vibAL_ = 0.11;
    double vibBL_ = 1.73;
    double vibCL_ = 2.91;
    double vibDL_ = 4.37;
    double vibEL_ = 5.19;
    double vibFL_ = 0.67;
    double vibGL_ = 2.23;
    double vibHL_ = 3.79;
    double vibAR_ = 4.71;
    double vibBR_ = 0.43;
    double vibCR_ = 1.37;
    double vibDR_ = 2.69;
    double vibER_ = 3.53;
    double vibFR_ = 5.77;
    double vibGR_ = 0.97;
    double vibHR_ = 4.13;

    uint32_t fpdL_ = 17;
    uint32_t fpdR_ = 29;

    void updateParameters();
    static double sanitize(double value, uint32_t& seed);
    static double readInterpolated(const double* buffer, int size, int index, double offset);
    static void advance(int& count, int delay);
};
