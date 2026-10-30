/* prayertimes.h - v0.1.0 - Islamic prayer times. Single-file C99 library; MIT licence, see the end of this file.

   Do this:
      #define PRAYERTIMES_IMPLEMENTATION
   before you include this file in *one* C or C++ file to create the implementation.
   Define PRAYERTIMES_STATIC as well to make all functions static.

   Times are Unix seconds (UTC, no leap seconds) held in a double. */
#ifndef PRAYERTIMES_H
#define PRAYERTIMES_H

#ifdef PRAYERTIMES_STATIC
#define PTDEF static
#else
#define PTDEF extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pt_sun {
    double ra_deg;    /* apparent right ascension, true equator and equinox of date, [0, 360) */
    double dec_deg;   /* apparent declination */
    double dist_au;   /* Sun-Earth distance */
    double gast_deg;  /* Greenwich apparent sidereal time, [0, 360) */
} pt_sun;

/* Convert a proleptic-Gregorian UTC civil date plus seconds of day to Unix seconds. */
PTDEF double pt_unix_from_civil(int year, int month, int day, double seconds_of_day);
/* Convert Unix seconds to a proleptic-Gregorian UTC civil date plus seconds of day. */
PTDEF void   pt_civil_from_unix(double unix_s, int *year, int *month, int *day, double *seconds_of_day);
/* Return TT - UT in seconds (Espenak-Meeus polynomial interpolation). */
PTDEF double pt_delta_t(double unix_s);
/* Compute the geocentric apparent position of the Sun at a Unix time. */
PTDEF void   pt_sun_at(double unix_s, pt_sun *out);
/* Return the topocentric, airless altitude of the Sun's centre in degrees; longitude is east-positive. */
PTDEF double pt_sun_altitude(double unix_s, double latitude_deg, double longitude_deg);

#ifdef __cplusplus
}
#endif
#endif /* PRAYERTIMES_H */

#ifdef PRAYERTIMES_IMPLEMENTATION
#include <math.h>

#define PT__PI 3.14159265358979323846

static double pt__rad(double d) { return d * (PT__PI / 180.0); }
static double pt__deg(double r) { return r * (180.0 / PT__PI); }

/* Civil dates (Howard Hinnant's algorithms, days since 1970-01-01, proleptic Gregorian). */
static long long pt__days_from_civil(long long y, unsigned m, unsigned d) {
    y -= m <= 2;
    const long long era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (long long)doe - 719468;
}
static void pt__civil_from_days(long long z, int *y, int *m, int *d) {
    z += 719468;
    const long long era = (z >= 0 ? z : z - 146096) / 146097;
    const unsigned doe = (unsigned)(z - era * 146097);
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const long long yy = (long long)yoe + era * 400;
    const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const unsigned mp = (5 * doy + 2) / 153;
    *d = (int)(doy - (153 * mp + 2) / 5 + 1);
    *m = (int)(mp < 10 ? mp + 3 : mp - 9);
    *y = (int)(yy + (*m <= 2));
}

PTDEF double pt_unix_from_civil(int year, int month, int day, double seconds_of_day) {
    return (double)pt__days_from_civil((long long)year, (unsigned)month, (unsigned)day) * 86400.0 + seconds_of_day;
}

PTDEF void pt_civil_from_unix(double unix_s, int *year, int *month, int *day, double *seconds_of_day) {
    long long days = (long long)floor(unix_s / 86400.0);
    pt__civil_from_days(days, year, month, day);
    *seconds_of_day = unix_s - (double)days * 86400.0;
}

/* Time scales: TT - UT in seconds (Espenak-Meeus). */
PTDEF double pt_delta_t(double unix_s) {
    double jd_ut = unix_s / 86400.0 + 2440587.5;
    double y = 2000.0 + (jd_ut - 2451545.0 - 14.0) / 365.24217;
    double u, dt;
    if (y < 1941.0) {
        u = (y - 1820.0) / 100.0;
        dt = -20.0 + 32.0 * u * u;
    } else if (y < 1961.0) {
        u = y - 1950.0;
        dt = 29.07 + 0.407 * u - u * u / 233.0 + u * u * u / 2547.0;
    } else if (y < 1986.0) {
        u = y - 1975.0;
        dt = 45.45 + 1.067 * u - u * u / 260.0 - u * u * u / 718.0;
    } else if (y < 2005.0) {
        u = y - 2000.0;
        dt = 63.86 + 0.3345 * u - 0.060374 * u * u + 0.0017275 * u * u * u
             + 0.000651814 * u * u * u * u + 0.00002373599 * u * u * u * u * u;
    } else if (y < 2050.0) {
        u = y - 2000.0;
        dt = 62.92 + 0.32217 * u + 0.005589 * u * u;
    } else if (y < 2150.0) {
        u = (y - 1820.0) / 100.0;
        dt = -20.0 + 32.0 * u * u - 0.5628 * (2150.0 - y);
    } else {
        u = (y - 1820.0) / 100.0;
        dt = -20.0 + 32.0 * u * u;
    }
    return dt;
}

static const double pt__earth_l0[][3] = {
    {1.75347045673, 0.00000000000, 0.00000000000},
    {0.03341656453, 4.66925680415, 6283.07584999140},
    {0.00034894275, 4.62610242189, 12566.15169998280},
    {0.00003417572, 2.82886579754, 3.52311834900},
    {0.00003497056, 2.74411783405, 5753.38488489680},
    {0.00003135899, 3.62767041756, 77713.77146812050},
    {0.00002676218, 4.41808345438, 7860.41939243920},
    {0.00002342691, 6.13516214446, 3930.20969621960},
    {0.00001273165, 2.03709657878, 529.69096509460},
    {0.00001324294, 0.74246341673, 11506.76976979360},
    {0.00000901854, 2.04505446477, 26.29831979980},
    {0.00001199167, 1.10962946234, 1577.34354244780},
    {0.00000857223, 3.50849152283, 398.14900340820},
    {0.00000779786, 1.17882681962, 5223.69391980220},
    {0.00000990250, 5.23268072088, 5884.92684658320},
    {0.00000753141, 2.53339052847, 5507.55323866740},
    {0.00000505267, 4.58292599973, 18849.22754997420},
    {0.00000492392, 4.20505711826, 775.52261132400},
    {0.00000356672, 2.91954114478, 0.06731030280},
    {0.00000284125, 1.89869240932, 796.29800681640},
    {0.00000242879, 0.34481445893, 5486.77784317500},
    {0.00000317087, 5.84901948512, 11790.62908865880},
    {0.00000271112, 0.31486255375, 10977.07880469900},
    {0.00000206217, 4.80646631478, 2544.31441988340},
    {0.00000205478, 1.86953770281, 5573.14280143310},
    {0.00000202318, 2.45767790232, 6069.77675455340},
    {0.00000126225, 1.08295459501, 20.77539549240},
    {0.00000155516, 0.83306084617, 213.29909543800},
};
static const double pt__earth_l1[][3] = {
    {6283.07584999140, 0.00000000000, 0.00000000000},
    {0.00206058863, 2.67823455808, 6283.07584999140},
    {0.00004303419, 2.63512233481, 12566.15169998280},
};
static const double pt__earth_l2[][3] = {
    {0.00008721859, 1.07253635559, 6283.07584999140},
};
static const double pt__earth_b1[][3] = {
    {0.00227777722, 3.41376620530, 6283.07584999140},
    {0.00003805678, 3.37063423795, 12566.15169998280},
};
static const double pt__earth_r0[][3] = {
    {1.00013988784, 0.00000000000, 0.00000000000},
    {0.01670699632, 3.09846350258, 6283.07584999140},
    {0.00013956024, 3.05524609456, 12566.15169998280},
    {0.00003083720, 5.19846674381, 77713.77146812050},
    {0.00001628463, 1.17387558054, 5753.38488489680},
    {0.00001575572, 2.84685214877, 7860.41939243920},
    {0.00000924799, 5.45292236722, 11506.76976979360},
    {0.00000542439, 4.56409151453, 3930.20969621960},
    {0.00000472110, 3.66100022149, 5884.92684658320},
    {0.00000085831, 1.27079125277, 161000.68573767410},
    {0.00000057056, 2.01374292245, 83996.84731811189},
    {0.00000055736, 5.24159799170, 71430.69561812909},
    {0.00000174844, 3.01193636733, 18849.22754997420},
    {0.00000243181, 4.27349530790, 11790.62908865880},
};
static const double pt__earth_r1[][3] = {
    {0.00103018607, 1.10748968172, 6283.07584999140},
    {0.00001721238, 1.06442300386, 12566.15169998280},
};
static const double pt__earth_r2[][3] = {
    {0.00004359385, 5.78455133808, 6283.07584999140},
};

#define PT__NELEM(a) ((int)(sizeof(a) / sizeof((a)[0])))

/* Sum a VSOP87 series: value = sum of A*cos(B + C*t) over rows {A, B, C}. */
static double pt__series(const double (*s)[3], int n, double t) {
    double sum = 0.0;
    int i;
    for (i = 0; i < n; i++)
        sum += s[i][0] * cos(s[i][1] + s[i][2] * t);
    return sum;
}

/* Geocentric apparent Sun: truncated VSOP87 Earth, IAU 1976 precession,
   abridged nutation, annual aberration. */
PTDEF void pt_sun_at(double unix_s, pt_sun *out) {
    double jd_ut = unix_s / 86400.0 + 2440587.5;
    double jd_tt = jd_ut + pt_delta_t(unix_s) / 86400.0;
    double T = (jd_tt - 2451545.0) / 36525.0;   /* Julian centuries TT */
    double t = (jd_tt - 2451545.0) / 365250.0;  /* Julian millennia TT */
    double arc = PT__PI / (180.0 * 3600.0);     /* arcseconds to radians */

    /* Heliocentric ecliptic coordinates of the Earth (J2000 ecliptic), radians and AU. */
    double L = pt__series(pt__earth_l0, PT__NELEM(pt__earth_l0), t)
             + t * pt__series(pt__earth_l1, PT__NELEM(pt__earth_l1), t)
             + t * t * pt__series(pt__earth_l2, PT__NELEM(pt__earth_l2), t);
    double B = t * pt__series(pt__earth_b1, PT__NELEM(pt__earth_b1), t);
    double R = pt__series(pt__earth_r0, PT__NELEM(pt__earth_r0), t)
             + t * pt__series(pt__earth_r1, PT__NELEM(pt__earth_r1), t)
             + t * t * pt__series(pt__earth_r2, PT__NELEM(pt__earth_r2), t);

    /* Rectangular heliocentric, then rotate to the J2000 equator. */
    double x = R * cos(B) * cos(L);
    double y = R * cos(B) * sin(L);
    double z = R * sin(B);
    double X =  x + 0.000000440360 * y - 0.000000190919 * z;
    double Y = -0.000000479966 * x + 0.917482137087 * y - 0.397776982902 * z;
    double Z =  0.397776982902 * y + 0.917482137087 * z;

    /* Geocentric Sun, J2000 equator and equinox. */
    double a0 = atan2(-Y, -X);
    double d0 = atan2(-Z, hypot(X, Y));

    /* Precession to the mean equator of date (IAU 1976). */
    double zeta = (2306.2181 * T + 0.30188 * T * T + 0.017998 * T * T * T) * arc;
    double z_p  = (2306.2181 * T + 1.09468 * T * T + 0.018203 * T * T * T) * arc;
    double th   = (2004.3109 * T - 0.42665 * T * T - 0.041833 * T * T * T) * arc;
    double A = cos(d0) * sin(a0 + zeta);
    double Bp = cos(th) * cos(d0) * cos(a0 + zeta) - sin(th) * sin(d0);
    double Cp = sin(th) * cos(d0) * cos(a0 + zeta) + cos(th) * sin(d0);
    double am = atan2(A, Bp) + z_p;
    double dm = asin(Cp);

    /* To the mean ecliptic of date. */
    double epsA = (84381.448 - 46.8150 * T - 0.00059 * T * T + 0.001813 * T * T * T) * arc;
    double lam_m = atan2(sin(am) * cos(epsA) + tan(dm) * sin(epsA), cos(am));
    double bet_m = asin(sin(dm) * cos(epsA) - cos(dm) * sin(epsA) * sin(am));

    /* Nutation (abridged) and aberration. */
    double Om = pt__rad(125.04452 - 1934.136261 * T + 0.0020708 * T * T + T * T * T / 450000.0);
    double Ls = pt__rad(280.4665 + 36000.7698 * T);
    double Lm = pt__rad(218.3165 + 481267.8813 * T);
    double dpsi = (-17.20 * sin(Om) - 1.32 * sin(2.0 * Ls) - 0.23 * sin(2.0 * Lm) + 0.21 * sin(2.0 * Om)) * arc;
    double deps = (  9.20 * cos(Om) + 0.57 * cos(2.0 * Ls) + 0.10 * cos(2.0 * Lm) - 0.09 * cos(2.0 * Om)) * arc;

    double lam = lam_m + dpsi - (20.4898 / R) * arc;
    double bet = bet_m;
    double eps = epsA + deps;

    /* Apparent equatorial place. */
    double ra = atan2(sin(lam) * cos(eps) - tan(bet) * sin(eps), cos(lam));
    double dec = asin(sin(bet) * cos(eps) + cos(bet) * sin(eps) * sin(lam));
    ra = pt__deg(ra);
    ra = fmod(ra, 360.0);
    if (ra < 0.0) ra += 360.0;

    /* Greenwich apparent sidereal time (UT based). */
    double d = jd_ut - 2451545.0;
    double Tu = d / 36525.0;
    double gmst = 280.46061837 + 360.98564736629 * d + 0.000387933 * Tu * Tu - Tu * Tu * Tu / 38710000.0;
    double gast = gmst + pt__deg(dpsi) * cos(eps);
    gast = fmod(gast, 360.0);
    if (gast < 0.0) gast += 360.0;

    out->ra_deg = ra;
    out->dec_deg = pt__deg(dec);
    out->dist_au = R;
    out->gast_deg = gast;
}

/* Topocentric, airless altitude of the Sun's centre in degrees; longitude east-positive. */
PTDEF double pt_sun_altitude(double unix_s, double latitude_deg, double longitude_deg) {
    pt_sun s;
    pt_sun_at(unix_s, &s);
    double phi = pt__rad(latitude_deg);
    double dec = pt__rad(s.dec_deg);
    double H = pt__rad(s.gast_deg + longitude_deg - s.ra_deg);
    double h = asin(sin(phi) * sin(dec) + cos(phi) * cos(dec) * cos(H));
    double h_topo = h - pt__rad(8.794143 / 3600.0) / s.dist_au * cos(h);
    return pt__deg(h_topo);
}
#endif /* PRAYERTIMES_IMPLEMENTATION */

/*
MIT License

Copyright (c) 2026 zbahhah

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

-------------------------------------------------------------------------------
Third-party notice

The truncated VSOP87 Earth series in prayertimes.h (term values and the
ecliptic-to-equatorial rotation) come from Astronomy Engine:

MIT License

Copyright (c) 2019-2025 Don Cross <cosinekitty@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
