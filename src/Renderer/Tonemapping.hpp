#pragma once

using namespace glm;


#define TONE_MAPPING_UCS_ICTCP 0
#define TONE_MAPPING_UCS_JZAZBZ 1
#define TONE_MAPPING_UCS TONE_MAPPING_UCS_ICTCP
#define GRAN_TURISMO_SDR_PAPER_WHITE 250.0f // cd/m^2
#define REFERENCE_LUMINANCE 100.0f          // cd/m^2 <-> 1.0f
#define exponentScaleFactor 1.0f
#define JZAZBZ_EXPONENT_SCALE_FACTOR 1.7f

float frameBufferValueToPhysicalValue(float fbValue) {
    // Converts linear frame-buffer value to physical luminance (cd/m^2)
    // where 1.0 corresponds to REFERENCE_LUMINANCE (e.g., 100 cd/m^2).
    return fbValue * REFERENCE_LUMINANCE;
}

float physicalValueToFrameBufferValue(float physical) {
    // Converts physical luminance (cd/m^2) to a linear frame-buffer value,
    // where 1.0 corresponds to REFERENCE_LUMINANCE (e.g., 100 cd/m^2).
    return physical / REFERENCE_LUMINANCE;
}

float smoothStep(float x, float edge0, float edge1) {
    float t = (x - edge0) / (edge1 - edge0);

    if (x < edge0) {
        return 0.0f;
    }
    if (x > edge1) {
        return 1.0f;
    }

    return t * t * (3.0f - 2.0f * t);
}

float chromaCurve(float x, float a, float b) { return 1.0f - smoothStep(x, a, b); }

// EOTF / inverse-EOTF for ST-2084 (PQ)

float eotfSt2084(float n, float exponent) {
    if (n < 0.0f) {
        n = 0.0f;
    }
    if (n > 1.0f) {
        n = 1.0f;
    }

    // Base functions from SMPTE ST 2084:2014
    // Converts from normalized PQ (0-1) to absolute luminance in cd/m^2 (linear light)
    // Assumes float input; does not handle integer encoding (Annex)
    // Assumes full-range signal (0-1)
    const float m1 = 0.1593017578125f; // (2610 / 4096) / 4
    float m2 = exponent * 78.84375f;   // (2523 / 4096) * 128
    const float c1 = 0.8359375f;       // 3424 / 4096
    const float c2 = 18.8515625f;      // (2413 / 4096) * 32
    const float c3 = 18.6875f;         // (2392 / 4096) * 32
    const float pqC = 10000.0f;        // Maximum luminance supported by PQ (cd/m^2)

    // Does not handle signal range from 2084 - assumes full range (0-1)
    float np = pow(n, 1.0f / m2);
    float l = np - c1;

    if (l < 0.0f) {
        l = 0.0f;
    }

    l = l / (c2 - c3 * np);
    l = pow(l, 1.0f / m1);

    // Convert absolute luminance (cd/m^2) into the frame-buffer linear scale.
    return physicalValueToFrameBufferValue(l * pqC);
}

float inverseEotfSt2084(float v, float exponent) {
    const float m1 = 0.1593017578125f;
    float m2 = exponent * 78.84375f;
    const float c1 = 0.8359375f;
    const float c2 = 18.8515625f;
    const float c3 = 18.6875f;
    const float pqC = 10000.0f;

    // Convert the frame-buffer linear scale into absolute luminance (cd/m^2).
    float physical = frameBufferValueToPhysicalValue(v);
    float y = physical / pqC; // Normalize for the ST-2084 curve

    float ym = pow(y, m1);
    return exp2(m2 * (log2(c1 + c2 * ym) - log2(1.0f + c3 * ym)));
}

// ICtCp conversion

void rgbToICtCp(vec3 rgb, vec3& ictCp) // Input: linear Rec.2020
{
    float l = (rgb[0] * 1688.0f + rgb[1] * 2146.0f + rgb[2] * 262.0f) / 4096.0f;
    float m = (rgb[0] * 683.0f + rgb[1] * 2951.0f + rgb[2] * 462.0f) / 4096.0f;
    float s = (rgb[0] * 99.0f + rgb[1] * 309.0f + rgb[2] * 3688.0f) / 4096.0f;

    float lPQ = inverseEotfSt2084(l, exponentScaleFactor);
    float mPQ = inverseEotfSt2084(m, exponentScaleFactor);
    float sPQ = inverseEotfSt2084(s, exponentScaleFactor);

    ictCp[0] = (2048.0f * lPQ + 2048.0f * mPQ) / 4096.0f;
    ictCp[1] = (6610.0f * lPQ - 13613.0f * mPQ + 7003.0f * sPQ) / 4096.0f;
    ictCp[2] = (17933.0f * lPQ - 17390.0f * mPQ - 543.0f * sPQ) / 4096.0f;
}

void iCtCpToRgb(vec3 ictCp, vec3& rgb) // Output: linear Rec.2020
{
    float l = ictCp[0] + 0.00860904f * ictCp[1] + 0.11103f * ictCp[2];
    float m = ictCp[0] - 0.00860904f * ictCp[1] - 0.11103f * ictCp[2];
    float s = ictCp[0] + 0.560031f * ictCp[1] - 0.320627f * ictCp[2];

    float lLin = eotfSt2084(l, exponentScaleFactor);
    float mLin = eotfSt2084(m, exponentScaleFactor);
    float sLin = eotfSt2084(s, exponentScaleFactor);

    rgb[0] = max(3.43661f * lLin - 2.50645f * mLin + 0.0698454f * sLin, 0.0f);
    rgb[1] = max(-0.79133f * lLin + 1.9836f * mLin - 0.192271f * sLin, 0.0f);
    rgb[2] = max(-0.0259499f * lLin - 0.0989137f * mLin + 1.12486f * sLin, 0.0f);
}

// Jzazbz conversion

void rgbToJzazbz(vec3 rgb, vec3& jab) // Input: linear Rec.2020
{
    float l = rgb[0] * 0.530004f + rgb[1] * 0.355704f + rgb[2] * 0.086090f;
    float m = rgb[0] * 0.289388f + rgb[1] * 0.525395f + rgb[2] * 0.157481f;
    float s = rgb[0] * 0.091098f + rgb[1] * 0.147588f + rgb[2] * 0.734234f;

    float lPQ = inverseEotfSt2084(l, JZAZBZ_EXPONENT_SCALE_FACTOR);
    float mPQ = inverseEotfSt2084(m, JZAZBZ_EXPONENT_SCALE_FACTOR);
    float sPQ = inverseEotfSt2084(s, JZAZBZ_EXPONENT_SCALE_FACTOR);

    float iz = 0.5f * lPQ + 0.5f * mPQ;

    jab[0] = (0.44f * iz) / (1.0f - 0.56f * iz) - 1.6295499532821566e-11f;
    jab[1] = 3.524000f * lPQ - 4.066708f * mPQ + 0.542708f * sPQ;
    jab[2] = 0.199076f * lPQ + 1.096799f * mPQ - 1.295875f * sPQ;
}

void jzazbzToRgb(vec3 jab, vec3& rgb) // Output: linear Rec.2020
{
    float jz = jab[0] + 1.6295499532821566e-11f;
    float iz = jz / (0.44f + 0.56f * jz);
    float a = jab[1];
    float b = jab[2];

    float l = iz + a * 1.386050432715393e-1f + b * 5.804731615611869e-2f;
    float m = iz + a * -1.386050432715393e-1f + b * -5.804731615611869e-2f;
    float s = iz + a * -9.601924202631895e-2f + b * -8.118918960560390e-1f;

    float lLin = eotfSt2084(l, JZAZBZ_EXPONENT_SCALE_FACTOR);
    float mLin = eotfSt2084(m, JZAZBZ_EXPONENT_SCALE_FACTOR);
    float sLin = eotfSt2084(s, JZAZBZ_EXPONENT_SCALE_FACTOR);

    rgb[0] = lLin * 2.990669f + mLin * -2.049742f + sLin * 0.088977f;
    rgb[1] = lLin * -1.634525f + mLin * 3.145627f + sLin * -0.483037f;
    rgb[2] = lLin * -0.042505f + mLin * -0.377983f + sLin * 1.448019f;
}

// Unified color space (UCS): ICtCp or Jzazbz

#if TONE_MAPPING_UCS == TONE_MAPPING_UCS_ICTCP
void rgbToUcs(vec3 rgb, vec3& ucs) { rgbToICtCp(rgb, ucs); }
void ucsToRgb(vec3 ucs, vec3& rgb) { iCtCpToRgb(ucs, rgb); }
#elif TONE_MAPPING_UCS == TONE_MAPPING_UCS_JZAZBZ
void rgbToUcs(vec3 rgb, vec3& ucs) { rgbToJzazbz(rgb, ucs); }
void ucsToRgb(vec3 ucs, vec3& rgb) { jzazbzToRgb(ucs, rgb); }
#else
#error                                                                                                                 \
    "Unsupported TONE_MAPPING_UCS value. Please define TONE_MAPPING_UCS as either TONE_MAPPING_UCS_ICTCP or TONE_MAPPING_UCS_JZAZBZ."
#endif

// "GT Tone Mapping" curve with convergent shoulder

struct GTToneMappingCurveV2 {
    float peakIntensity_;
    float alpha_;
    float midPoint_;
    float linearSection_;
    float toeStrength_;
    float kA_, kB_, kC_;
};

void initializeCurve(float monitorIntensity, float alpha, float grayPoint, float linearSection, float toeStrength,
    GTToneMappingCurveV2& Curve) {
    Curve.peakIntensity_ = monitorIntensity;
    Curve.alpha_ = alpha;
    Curve.midPoint_ = grayPoint;
    Curve.linearSection_ = linearSection;
    Curve.toeStrength_ = toeStrength;

    // Pre-compute constants for the shoulder region.
    float k = (Curve.linearSection_ - 1.0f) / (Curve.alpha_ - 1.0f);
    Curve.kA_ = Curve.peakIntensity_ * Curve.linearSection_ + Curve.peakIntensity_ * k;
    Curve.kB_ = -Curve.peakIntensity_ * k * exp(Curve.linearSection_ / k);
    Curve.kC_ = -1.0f / (k * Curve.peakIntensity_);
}

float evaluateCurve(float x, GTToneMappingCurveV2 Curve) {
    if (x < 0.0f) {
        return 0.0f;
    }

    float weightLinear = smoothStep(x, 0.0f, Curve.midPoint_);
    float weightToe = 1.0f - weightLinear;

    // Shoulder mapping for highlights.
    float shoulder = Curve.kA_ + Curve.kB_ * exp(x * Curve.kC_);

    if (x < Curve.linearSection_ * Curve.peakIntensity_) {
        float toeMapped = Curve.midPoint_ * pow(x / Curve.midPoint_, Curve.toeStrength_);
        return weightToe * toeMapped + weightLinear * x;
    } else {
        return shoulder;
    }
}

// GT7ToneMapping

struct GT7ToneMapping {
    float sdrCorrectionFactor_;

    float framebufferLuminanceTarget_;
    float framebufferLuminanceTargetUcs_; // Target luminance in UCS space
    GTToneMappingCurveV2 curve_;

    float blendRatio_;
    float fadeStart_;
    float fadeEnd_;
};

void initializeParameters(float physicalTargetLuminance, GT7ToneMapping& TM) {
    TM.framebufferLuminanceTarget_ = physicalValueToFrameBufferValue(physicalTargetLuminance);

    // Initialize the curve (slightly different parameters from GT Sport).
    initializeCurve(TM.framebufferLuminanceTarget_, 0.25f, 0.538f, 0.444f, 1.280f, TM.curve_);

    // Default parameters.
    TM.blendRatio_ = 0.6f;
    TM.fadeStart_ = 0.98f;
    TM.fadeEnd_ = 1.16f;

    vec3 ucs;
    vec3 rgb = vec3(TM.framebufferLuminanceTarget_, TM.framebufferLuminanceTarget_, TM.framebufferLuminanceTarget_);
    rgbToUcs(rgb, ucs);
    TM.framebufferLuminanceTargetUcs_ = ucs[0]; // Use the first UCS component (I or Jz) as luminance
}

void initializeAsHDR(float physicalTargetLuminance, GT7ToneMapping& TM) {
    TM.sdrCorrectionFactor_ = 1.0f;
    initializeParameters(physicalTargetLuminance, TM);
}

void initializeAsSDR(GT7ToneMapping& TM) {

    TM.sdrCorrectionFactor_ = 1.0f / physicalValueToFrameBufferValue(GRAN_TURISMO_SDR_PAPER_WHITE);
    initializeParameters(GRAN_TURISMO_SDR_PAPER_WHITE, TM);
}

void applyToneMapping(vec3& rgb, GT7ToneMapping TM) {
    // Convert to UCS to separate luminance and chroma.
    vec3 ucs;
    rgbToUcs(rgb, ucs);

    // Per-channel tone mapping ("skewed" color).
    vec3 skewedRgb =
        vec3(evaluateCurve(rgb[0], TM.curve_), evaluateCurve(rgb[1], TM.curve_), evaluateCurve(rgb[2], TM.curve_));

    vec3 skewedUcs;
    rgbToUcs(skewedRgb, skewedUcs);

    float chromaScale = chromaCurve(ucs[0] / TM.framebufferLuminanceTargetUcs_, TM.fadeStart_, TM.fadeEnd_);

    vec3 scaledUcs = vec3(skewedUcs[0], // Luminance from skewed color
        ucs[1] * chromaScale,           // Scaled chroma components
        ucs[2] * chromaScale);

    // Convert back to RGB.
    vec3 scaledRgb;
    ucsToRgb(scaledUcs, scaledRgb);

    // Final blend between per-channel and UCS-scaled results.
    for (int i = 0; i < 3; ++i) {
        float blended = (1.0f - TM.blendRatio_) * skewedRgb[i] + TM.blendRatio_ * scaledRgb[i];
        // When using SDR, apply the correction factor.
        // When using HDR, sdrCorrectionFactor_ is 1.0f, so it has no effect.
        rgb[i] = TM.sdrCorrectionFactor_ * min(blended, TM.framebufferLuminanceTarget_);
    }
}
