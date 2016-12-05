//
// sh.glsl
//

// Number off coefficients (5th order)
const int SH_COEFF_NUM = 25;

// Precalculated basis constants
const float SQRT_PI = 1.7724538509055160272981674833411451827975494561223871;
const float K0  =  0.28209479177;
const float K1  =  0.48860251190;
const float K2  =  1.09254843059;
const float K3  = -1.09254843059;
const float K4  =  0.31539156525;
const float K5  =  0.54627421529;
const float K6  = -0.59004358992;
const float K7  =  2.89061144264;
const float K8  = -0.64636036822;
const float K9  =  0.37317633259;
const float K10 = -0.45704579946;
const float K11 =  1.44530572132;
const float K12 = -0.59004358992;
const float K13 =  2.50334294180;
const float K14 = -1.77013076978;
const float K15 =  0.94617469575;
const float K16 = -1.33809308711;
const float K17 =  0.47308734787;
const float K18 =  0.62583573544;

void sh_eval_basis5(inout float sh_basis[SH_COEFF_NUM], vec3 dir)
{
    float x = dir.x;
    float y = dir.y;
    float z = dir.z;

    float x2 = x*x;
    float y2 = y*y;
    float z2 = z*z;

    float z3 = z*z*z;

    float x4 = x*x*x*x;
    float y4 = y*y*y*y;
    float z4 = z*z*z*z;

    sh_basis[0]  = K0;

    sh_basis[1]  = -K1 * y;
    sh_basis[2]  = K1 * z;
    sh_basis[3]  = -K1 * x;

    sh_basis[4]  = K2 * y * x;
    sh_basis[5]  = K3 * y * z;
    sh_basis[6]  = K4 * (3.0 * z2 - 1.0);
    sh_basis[7]  = K3 * x * z;
    sh_basis[8]  = K5 * (x2 - y2);

    sh_basis[9]  = K6 * y * (3 * x2 - y2);
    sh_basis[10] = K7 * y * x * z;
    sh_basis[11] = K8 * y * (-1.0 + 5.0 * z2);
    sh_basis[12] = K9 * (5.0 * z3 - 3.0 * z);
    sh_basis[13] = K10 * x * (-1.0 + 5.0 * z2);
    sh_basis[14] = K11 * (x2 - y2) * z;
    sh_basis[15] = K12 * x * (x2 - 3.0 * y2);

    sh_basis[16] = K13 * x * y * (x2 - y2);
    sh_basis[17] = K14 * y * z * (3.0 * x2 - y2);
    sh_basis[18] = K15 * y * x * (-1.0 + 7.0 * z2);
    sh_basis[19] = K16 * y * z * (-3.0 + 7.0 * z2);
    sh_basis[20] =  (105.0 * z4 -90.0 * z2 + 9.0) / (16.0 * SQRT_PI);
    sh_basis[21] = K16 * x * z * (-3.0 + 7.0 * z2);
    sh_basis[22] = K17 * (x2 - y2) * (-1.0 + 7.0 * z2);
    sh_basis[23] = K14 * x * z * (x2 - 3.0 * y2);
    sh_basis[24] = K18 * (x4 - 6.0 * y2 * x2 + y4);
}

vec3 sh_irradiance(vec3 dir, vec3 sh_coef[SH_COEFF_NUM])
{
    /* Eval basis for current direction */
    float sh_basis[SH_COEFF_NUM];
    sh_eval_basis5(sh_basis, dir);

    /* Accumulator */
    vec3 irr = vec3(0.0);

    /* Band 0 (factor 1.0) */
    irr += sh_coef[0] * sh_basis[0] * 1.0;
    /* Band 1 (factor 2/3). */
    int ii = 1;
    for (; ii < 4; ++ii) {
        irr += sh_coef[ii] * sh_basis[ii] * (2.0/3.0);
    }
    /* Band 2 (factor 1/4). */
    for (; ii < 9; ++ii) {
        irr += sh_coef[ii] * sh_basis[ii] * (1.0/4.0);
    }
    /* Band 3 (factor 0). */
    ii = 16;
    /* Band 4 (factor -1/24). */
    for (; ii < 25; ++ii) {
        irr += sh_coef[ii] * sh_basis[ii] * (-1.0/24.0);
    }

    return irr;
}
