/*
 * Airwindows Reverb DSP port.
 *
 * Original source: https://github.com/airwindows/airwindows/tree/master/plugins/LinuxVST/src/Reverb
 * Copyright (c) 2016 airwindows, Airwindows uses the MIT license
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "airwindows_reverb.h"

#include <algorithm>

AirwindowsReverb::AirwindowsReverb(double sampleRate) {
    setSampleRate(sampleRate);
    clear();
}

void AirwindowsReverb::setSampleRate(double sampleRate) {
    sampleRate_ = std::max(1.0, sampleRate);
    updateParameters();
}

void AirwindowsReverb::set(double size, double wet) {
    sizeParam_ = std::clamp(size, 0.0, 1.0);
    wetParam_ = std::clamp(wet, 0.0, 1.0);
    updateParameters();
}

void AirwindowsReverb::clear() {
    biquadA_.fill(0.0);
    biquadB_.fill(0.0);
    biquadC_.fill(0.0);
    aAL_.fill(0.0); aBL_.fill(0.0); aCL_.fill(0.0); aDL_.fill(0.0); aEL_.fill(0.0); aFL_.fill(0.0); aGL_.fill(0.0); aHL_.fill(0.0); aIL_.fill(0.0); aJL_.fill(0.0); aKL_.fill(0.0); aLL_.fill(0.0); aML_.fill(0.0);
    aAR_.fill(0.0); aBR_.fill(0.0); aCR_.fill(0.0); aDR_.fill(0.0); aER_.fill(0.0); aFR_.fill(0.0); aGR_.fill(0.0); aHR_.fill(0.0); aIR_.fill(0.0); aJR_.fill(0.0); aKR_.fill(0.0); aLR_.fill(0.0); aMR_.fill(0.0);
    countA_ = countB_ = countC_ = countD_ = countE_ = countF_ = countG_ = countH_ = 1;
    countI_ = countJ_ = countK_ = countL_ = countM_ = 1;
    feedbackAL_ = feedbackBL_ = feedbackCL_ = feedbackDL_ = feedbackEL_ = feedbackFL_ = feedbackGL_ = feedbackHL_ = 0.0;
    feedbackAR_ = feedbackBR_ = feedbackCR_ = feedbackDR_ = feedbackER_ = feedbackFR_ = feedbackGR_ = feedbackHR_ = 0.0;
    updateParameters();
}

void AirwindowsReverb::updateParameters() {
    const double big = sizeParam_;
    const double wet = wetParam_;
    const double size = (std::pow(big, 2.0) * 75.0) + 25.0;

    delayA_ = static_cast<int>(79 * size);
    delayB_ = static_cast<int>(73 * size);
    delayC_ = static_cast<int>(71 * size);
    delayD_ = static_cast<int>(67 * size);
    delayE_ = static_cast<int>(61 * size);
    delayF_ = static_cast<int>(59 * size);
    delayG_ = static_cast<int>(53 * size);
    delayH_ = static_cast<int>(47 * size);
    delayI_ = static_cast<int>(43 * size);
    delayJ_ = static_cast<int>(41 * size);
    delayK_ = static_cast<int>(37 * size);
    delayL_ = static_cast<int>(31 * size);
    delayM_ = static_cast<int>(29 * size);

    biquadC_[0] = biquadB_[0] = biquadA_[0] = (10000.0 - (big * wet * 3000.0)) / sampleRate_;
    biquadA_[1] = kGolden;
    biquadB_[1] = kInvGolden;
    biquadC_[1] = 0.5;

    for (auto* biquad : {&biquadA_, &biquadB_, &biquadC_}) {
        const double k = std::tan(kPi * (*biquad)[0]);
        const double norm = 1.0 / (1.0 + k / (*biquad)[1] + k * k);
        (*biquad)[2] = k * k * norm;
        (*biquad)[3] = 2.0 * (*biquad)[2];
        (*biquad)[4] = (*biquad)[2];
        (*biquad)[5] = 2.0 * (k * k - 1.0) * norm;
        (*biquad)[6] = (1.0 - k / (*biquad)[1] + k * k) * norm;
    }
}

void AirwindowsReverb::process(double inL, double inR, double& outL, double& outR) {
    const double big = sizeParam_;
    const double wet = wetParam_;
    const double vibSpeed = 0.1;
    const double vibDepth = 7.0;
    const double size = (std::pow(big, 2.0) * 75.0) + 25.0;
    const double depthFactor = 1.0 - std::pow((1.0 - (0.82 - (((1.0 - big) * 0.7) + (size * 0.002)))), 4.0);
    const double blend = 0.955 - (size * 0.007);
    const double regen = depthFactor * 0.5;

    double inputSampleL = sanitize(inL, fpdL_);
    double inputSampleR = sanitize(inR, fpdR_);
    const double drySampleL = inputSampleL;
    const double drySampleR = inputSampleR;

    aML_[countM_] = inputSampleL;
    aMR_[countM_] = inputSampleR;
    advance(countM_, delayM_);
    inputSampleL = aML_[countM_];
    inputSampleR = aMR_[countM_];

    double tempSampleL = (inputSampleL * biquadA_[2]) + biquadA_[7];
    biquadA_[7] = (inputSampleL * biquadA_[3]) - (tempSampleL * biquadA_[5]) + biquadA_[8];
    biquadA_[8] = (inputSampleL * biquadA_[4]) - (tempSampleL * biquadA_[6]);
    inputSampleL = tempSampleL;
    double tempSampleR = (inputSampleR * biquadA_[2]) + biquadA_[9];
    biquadA_[9] = (inputSampleR * biquadA_[3]) - (tempSampleR * biquadA_[5]) + biquadA_[10];
    biquadA_[10] = (inputSampleR * biquadA_[4]) - (tempSampleR * biquadA_[6]);
    inputSampleR = tempSampleR;

    inputSampleL = std::sin(inputSampleL * wet);
    inputSampleR = std::sin(inputSampleR * wet);

    double allpassIL = inputSampleL, allpassJL = inputSampleL, allpassKL = inputSampleL, allpassLL = inputSampleL;
    double allpassIR = inputSampleR, allpassJR = inputSampleR, allpassKR = inputSampleR, allpassLR = inputSampleR;

    int allpasstemp = countI_ + 1; if (allpasstemp > delayI_) allpasstemp = 0;
    allpassIL -= aIL_[allpasstemp] * 0.5; aIL_[countI_] = allpassIL; allpassIL *= 0.5;
    allpassIR -= aIR_[allpasstemp] * 0.5; aIR_[countI_] = allpassIR; allpassIR *= 0.5;
    advance(countI_, delayI_); allpassIL += aIL_[countI_]; allpassIR += aIR_[countI_];

    allpasstemp = countJ_ + 1; if (allpasstemp > delayJ_) allpasstemp = 0;
    allpassJL -= aJL_[allpasstemp] * 0.5; aJL_[countJ_] = allpassJL; allpassJL *= 0.5;
    allpassJR -= aJR_[allpasstemp] * 0.5; aJR_[countJ_] = allpassJR; allpassJR *= 0.5;
    advance(countJ_, delayJ_); allpassJL += aJL_[countJ_]; allpassJR += aJR_[countJ_];

    allpasstemp = countK_ + 1; if (allpasstemp > delayK_) allpasstemp = 0;
    allpassKL -= aKL_[allpasstemp] * 0.5; aKL_[countK_] = allpassKL; allpassKL *= 0.5;
    allpassKR -= aKR_[allpasstemp] * 0.5; aKR_[countK_] = allpassKR; allpassKR *= 0.5;
    advance(countK_, delayK_); allpassKL += aKL_[countK_]; allpassKR += aKR_[countK_];

    allpasstemp = countL_ + 1; if (allpasstemp > delayL_) allpasstemp = 0;
    allpassLL -= aLL_[allpasstemp] * 0.5; aLL_[countL_] = allpassLL; allpassLL *= 0.5;
    allpassLR -= aLR_[allpasstemp] * 0.5; aLR_[countL_] = allpassLR; allpassLR *= 0.5;
    advance(countL_, delayL_); allpassLL += aLL_[countL_]; allpassLR += aLR_[countL_];

    aAL_[countA_] = allpassLL + feedbackAL_; aBL_[countB_] = allpassKL + feedbackBL_; aCL_[countC_] = allpassJL + feedbackCL_; aDL_[countD_] = allpassIL + feedbackDL_;
    aEL_[countE_] = allpassIL + feedbackEL_; aFL_[countF_] = allpassJL + feedbackFL_; aGL_[countG_] = allpassKL + feedbackGL_; aHL_[countH_] = allpassLL + feedbackHL_;
    aAR_[countA_] = allpassLR + feedbackAR_; aBR_[countB_] = allpassKR + feedbackBR_; aCR_[countC_] = allpassJR + feedbackCR_; aDR_[countD_] = allpassIR + feedbackDR_;
    aER_[countE_] = allpassIR + feedbackER_; aFR_[countF_] = allpassJR + feedbackFR_; aGR_[countG_] = allpassKR + feedbackGR_; aHR_[countH_] = allpassLR + feedbackHR_;

    advance(countA_, delayA_); advance(countB_, delayB_); advance(countC_, delayC_); advance(countD_, delayD_);
    advance(countE_, delayE_); advance(countF_, delayF_); advance(countG_, delayG_); advance(countH_, delayH_);

    vibAL_ += (0.003251 * vibSpeed); vibBL_ += (0.002999 * vibSpeed); vibCL_ += (0.002917 * vibSpeed); vibDL_ += (0.002749 * vibSpeed);
    vibEL_ += (0.002503 * vibSpeed); vibFL_ += (0.002423 * vibSpeed); vibGL_ += (0.002146 * vibSpeed); vibHL_ += (0.002088 * vibSpeed);
    vibAR_ += (0.003251 * vibSpeed); vibBR_ += (0.002999 * vibSpeed); vibCR_ += (0.002917 * vibSpeed); vibDR_ += (0.002749 * vibSpeed);
    vibER_ += (0.002503 * vibSpeed); vibFR_ += (0.002423 * vibSpeed); vibGR_ += (0.002146 * vibSpeed); vibHR_ += (0.002088 * vibSpeed);

    const double offsetAL = (std::sin(vibAL_) + 1.0) * vibDepth; const double offsetBL = (std::sin(vibBL_) + 1.0) * vibDepth;
    const double offsetCL = (std::sin(vibCL_) + 1.0) * vibDepth; const double offsetDL = (std::sin(vibDL_) + 1.0) * vibDepth;
    const double offsetEL = (std::sin(vibEL_) + 1.0) * vibDepth; const double offsetFL = (std::sin(vibFL_) + 1.0) * vibDepth;
    const double offsetGL = (std::sin(vibGL_) + 1.0) * vibDepth; const double offsetHL = (std::sin(vibHL_) + 1.0) * vibDepth;
    const double offsetAR = (std::sin(vibAR_) + 1.0) * vibDepth; const double offsetBR = (std::sin(vibBR_) + 1.0) * vibDepth;
    const double offsetCR = (std::sin(vibCR_) + 1.0) * vibDepth; const double offsetDR = (std::sin(vibDR_) + 1.0) * vibDepth;
    const double offsetER = (std::sin(vibER_) + 1.0) * vibDepth; const double offsetFR = (std::sin(vibFR_) + 1.0) * vibDepth;
    const double offsetGR = (std::sin(vibGR_) + 1.0) * vibDepth; const double offsetHR = (std::sin(vibHR_) + 1.0) * vibDepth;

    double interpolAL = readInterpolated(aAL_.data(), delayA_, countA_, offsetAL);
    double interpolBL = readInterpolated(aBL_.data(), delayB_, countB_, offsetBL);
    double interpolCL = readInterpolated(aCL_.data(), delayC_, countC_, offsetCL);
    double interpolDL = readInterpolated(aDL_.data(), delayD_, countD_, offsetDL);
    double interpolEL = readInterpolated(aEL_.data(), delayE_, countE_, offsetEL);
    double interpolFL = readInterpolated(aFL_.data(), delayF_, countF_, offsetFL);
    double interpolGL = readInterpolated(aGL_.data(), delayG_, countG_, offsetGL);
    double interpolHL = readInterpolated(aHL_.data(), delayH_, countH_, offsetHL);
    double interpolAR = readInterpolated(aAR_.data(), delayA_, countA_, offsetAR);
    double interpolBR = readInterpolated(aBR_.data(), delayB_, countB_, offsetBR);
    double interpolCR = readInterpolated(aCR_.data(), delayC_, countC_, offsetCR);
    double interpolDR = readInterpolated(aDR_.data(), delayD_, countD_, offsetDR);
    double interpolER = readInterpolated(aER_.data(), delayE_, countE_, offsetER);
    double interpolFR = readInterpolated(aFR_.data(), delayF_, countF_, offsetFR);
    double interpolGR = readInterpolated(aGR_.data(), delayG_, countG_, offsetGR);
    double interpolHR = readInterpolated(aHR_.data(), delayH_, countH_, offsetHR);

    auto blendTap = [blend](double interpol, const double* buffer, int delay, int count, double offset) {
        const int working = count + static_cast<int>(offset);
        const int index = working > delay ? working - delay - 1 : working;
        return ((1.0 - blend) * interpol) + (buffer[index] * blend);
    };
    interpolAL = blendTap(interpolAL, aAL_.data(), delayA_, countA_, offsetAL); interpolBL = blendTap(interpolBL, aBL_.data(), delayB_, countB_, offsetBL);
    interpolCL = blendTap(interpolCL, aCL_.data(), delayC_, countC_, offsetCL); interpolDL = blendTap(interpolDL, aDL_.data(), delayD_, countD_, offsetDL);
    interpolEL = blendTap(interpolEL, aEL_.data(), delayE_, countE_, offsetEL); interpolFL = blendTap(interpolFL, aFL_.data(), delayF_, countF_, offsetFL);
    interpolGL = blendTap(interpolGL, aGL_.data(), delayG_, countG_, offsetGL); interpolHL = blendTap(interpolHL, aHL_.data(), delayH_, countH_, offsetHL);
    interpolAR = blendTap(interpolAR, aAR_.data(), delayA_, countA_, offsetAR); interpolBR = blendTap(interpolBR, aBR_.data(), delayB_, countB_, offsetBR);
    interpolCR = blendTap(interpolCR, aCR_.data(), delayC_, countC_, offsetCR); interpolDR = blendTap(interpolDR, aDR_.data(), delayD_, countD_, offsetDR);
    interpolER = blendTap(interpolER, aER_.data(), delayE_, countE_, offsetER); interpolFR = blendTap(interpolFR, aFR_.data(), delayF_, countF_, offsetFR);
    interpolGR = blendTap(interpolGR, aGR_.data(), delayG_, countG_, offsetGR); interpolHR = blendTap(interpolHR, aHR_.data(), delayH_, countH_, offsetHR);

    feedbackAL_ = (interpolAL - (interpolBL + interpolCL + interpolDL)) * regen;
    feedbackBL_ = (interpolBL - (interpolAL + interpolCL + interpolDL)) * regen;
    feedbackCL_ = (interpolCL - (interpolAL + interpolBL + interpolDL)) * regen;
    feedbackDL_ = (interpolDL - (interpolAL + interpolBL + interpolCL)) * regen;
    feedbackEL_ = (interpolEL - (interpolFL + interpolGL + interpolHL)) * regen;
    feedbackFL_ = (interpolFL - (interpolEL + interpolGL + interpolHL)) * regen;
    feedbackGL_ = (interpolGL - (interpolEL + interpolFL + interpolHL)) * regen;
    feedbackHL_ = (interpolHL - (interpolEL + interpolFL + interpolGL)) * regen;
    feedbackAR_ = (interpolAR - (interpolBR + interpolCR + interpolDR)) * regen;
    feedbackBR_ = (interpolBR - (interpolAR + interpolCR + interpolDR)) * regen;
    feedbackCR_ = (interpolCR - (interpolAR + interpolBR + interpolDR)) * regen;
    feedbackDR_ = (interpolDR - (interpolAR + interpolBR + interpolCR)) * regen;
    feedbackER_ = (interpolER - (interpolFR + interpolGR + interpolHR)) * regen;
    feedbackFR_ = (interpolFR - (interpolER + interpolGR + interpolHR)) * regen;
    feedbackGR_ = (interpolGR - (interpolER + interpolFR + interpolHR)) * regen;
    feedbackHR_ = (interpolHR - (interpolER + interpolFR + interpolGR)) * regen;

    inputSampleL = (interpolAL + interpolBL + interpolCL + interpolDL + interpolEL + interpolFL + interpolGL + interpolHL) / 8.0;
    inputSampleR = (interpolAR + interpolBR + interpolCR + interpolDR + interpolER + interpolFR + interpolGR + interpolHR) / 8.0;

    tempSampleL = (inputSampleL * biquadB_[2]) + biquadB_[7];
    biquadB_[7] = (inputSampleL * biquadB_[3]) - (tempSampleL * biquadB_[5]) + biquadB_[8];
    biquadB_[8] = (inputSampleL * biquadB_[4]) - (tempSampleL * biquadB_[6]);
    inputSampleL = tempSampleL;
    tempSampleR = (inputSampleR * biquadB_[2]) + biquadB_[9];
    biquadB_[9] = (inputSampleR * biquadB_[3]) - (tempSampleR * biquadB_[5]) + biquadB_[10];
    biquadB_[10] = (inputSampleR * biquadB_[4]) - (tempSampleR * biquadB_[6]);
    inputSampleR = tempSampleR;

    inputSampleL = std::asin(std::clamp(inputSampleL, -1.0, 1.0));
    inputSampleR = std::asin(std::clamp(inputSampleR, -1.0, 1.0));

    tempSampleL = (inputSampleL * biquadC_[2]) + biquadC_[7];
    biquadC_[7] = (inputSampleL * biquadC_[3]) - (tempSampleL * biquadC_[5]) + biquadC_[8];
    biquadC_[8] = (inputSampleL * biquadC_[4]) - (tempSampleL * biquadC_[6]);
    inputSampleL = tempSampleL;
    tempSampleR = (inputSampleR * biquadC_[2]) + biquadC_[9];
    biquadC_[9] = (inputSampleR * biquadC_[3]) - (tempSampleR * biquadC_[5]) + biquadC_[10];
    biquadC_[10] = (inputSampleR * biquadC_[4]) - (tempSampleR * biquadC_[6]);
    inputSampleR = tempSampleR;

    if (wet != 1.0) {
        inputSampleL += drySampleL * (1.0 - wet);
        inputSampleR += drySampleR * (1.0 - wet);
    }

    outL = std::clamp(inputSampleL, -1.0, 1.0);
    outR = std::clamp(inputSampleR, -1.0, 1.0);
}

double AirwindowsReverb::sanitize(double value, uint32_t& seed) {
    if (std::fabs(value) >= 1.18e-23) return value;
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    return static_cast<double>(seed) * 1.18e-17;
}

double AirwindowsReverb::readInterpolated(const double* buffer, int delay, int index, double offset) {
    const int offsetInt = static_cast<int>(offset);
    const double fraction = offset - std::floor(offset);
    int working = index + offsetInt;
    int i0 = working > delay ? working - delay - 1 : working;
    int i1 = working + 1 > delay ? working + 1 - delay - 1 : working + 1;
    return (buffer[i0] * (1.0 - fraction)) + (buffer[i1] * fraction);
}

void AirwindowsReverb::advance(int& count, int delay) {
    count++;
    if (count < 0 || count > delay) count = 0;
}
