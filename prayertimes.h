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

/* Error codes returned by the functions below; PT_OK (0) on success. */
typedef enum pt_error { PT_OK = 0, PT_ERR_LOCATION = -1, PT_ERR_SETTINGS = -2, PT_ERR_DATE = -3, PT_ERR_NO_NEXT = -4 } pt_error;
/* How a prayer time was obtained: computed, adjusted by a high-latitude rule, or never reached that day. */
typedef enum pt_status { PT_COMPUTED = 0, PT_ADJUSTED = 1, PT_NOT_REACHED = 2 } pt_status;

/* The nine times produced for one civil date; index pt_day with these. */
typedef enum pt_event { PT_FAJR = 0, PT_SUNRISE, PT_DHUHR, PT_ASR, PT_SUNSET, PT_MAGHRIB, PT_ISHA, PT_MIDNIGHT, PT_LAST_THIRD, PT_EVENT_COUNT } pt_event;

/* Kind of rule that fixes Fajr, Maghrib or Isha; `value` is degrees, minutes or a night fraction per kind. */
typedef enum pt_rule_kind {
    PT_RULE_ANGLE = 0,       /* Sun's centre `value` degrees below the horizon (Fajr, Maghrib, Isha) */
    PT_RULE_MINUTES,         /* Maghrib: `value` minutes after sunset. Isha: `value` minutes after Maghrib, `ramadan_value` in Ramadan */
    PT_RULE_NIGHT_FRACTION,  /* Fajr: `value` of the night before sunrise. Isha: `value` of the night after sunset */
    PT_RULE_SUNSET           /* Maghrib only: at sunset */
} pt_rule_kind;
typedef struct pt_rule { pt_rule_kind kind; double value; double ramadan_value; } pt_rule;

/* Dhuhr at solar transit, or after the disk's centre passes the meridian. */
typedef enum pt_dhuhr_rule { PT_DHUHR_TRANSIT = 0, PT_DHUHR_DISK_PAST_MERIDIAN } pt_dhuhr_rule;
/* Midnight splits the sunset-to-sunrise night or the sunset-to-Fajr night. */
typedef enum pt_midnight_rule { PT_MIDNIGHT_SUNSET_TO_SUNRISE = 0, PT_MIDNIGHT_SUNSET_TO_FAJR } pt_midnight_rule;
/* High-latitude fallback for angle rules: middle of night, one seventh, or the angle itself over 60. */
typedef enum pt_high_latitude { PT_HIGHLAT_NONE = 0, PT_HIGHLAT_MIDDLE_OF_NIGHT, PT_HIGHLAT_ONE_SEVENTH, PT_HIGHLAT_ANGLE_BASED } pt_high_latitude;
/* Rounding of times to whole minutes: none, nearest, or precautionary (sunrise/midnight down, others up). */
typedef enum pt_rounding { PT_ROUND_NONE = 0, PT_ROUND_NEAREST, PT_ROUND_PRECAUTIONARY } pt_rounding;
/* Asr shadow factor: 1 (Shafi'i/Standard) or 2 (Hanafi). */
typedef enum pt_asr { PT_ASR_STANDARD = 1, PT_ASR_HANAFI = 2 } pt_asr;   /* shadow factor */

/* Minutes added to each time. */
typedef struct pt_offsets { double fajr, sunrise, dhuhr, asr, maghrib, isha; } pt_offsets;

/* Identifiers of the 22 built-in method presets; PT_CUSTOM marks a user-built method. */
typedef enum pt_method_id {
    PT_MWL = 0, PT_ISNA, PT_EGYPT, PT_UMM_AL_QURA, PT_KARACHI, PT_JAFARI, PT_TEHRAN, PT_BAHRAIN,
    PT_DIYANET, PT_JAKIM, PT_MUIS, PT_KEMENAG, PT_TUNISIA, PT_ALGERIA, PT_DUBAI, PT_KUWAIT,
    PT_QATAR, PT_FRANCE12, PT_FRANCE15, PT_FRANCE18, PT_RUSSIA, PT_GULF,
    PT_METHOD_COUNT,          /* 22 presets */
    PT_CUSTOM = 100
} pt_method_id;

/* A calculation method: its parameters and the method's own precautionary minutes. */
typedef struct pt_method {
    pt_method_id id;
    const char *key;          /* short lowercase key, e.g. "mwl" */
    const char *name;
    const char *source;       /* where the parameters come from */
    int official;             /* 1 = published by the authority itself, 0 = secondary source */
    pt_rule fajr;
    pt_dhuhr_rule dhuhr;
    pt_rule maghrib;
    pt_rule isha;
    pt_midnight_rule midnight;
    pt_offsets precaution;    /* the method's own minutes (tamkin / temkin) */
} pt_method;

/* Everything pt_compute needs beyond location and date. */
typedef struct pt_settings {
    pt_method method;
    pt_asr asr;
    pt_high_latitude high_latitude;
    pt_rounding rounding;
    int ramadan;              /* 1 during Ramadan: selects the Isha ramadan_value */
    pt_offsets offsets;       /* user minutes, each -60..60 */
} pt_settings;

/* One prayer time in UTC Unix seconds; utc is NAN when status is PT_NOT_REACHED. */
typedef struct pt_time { double utc; pt_status status; } pt_time;
/* The nine times of one civil date, indexed by pt_event. */
typedef struct pt_day { pt_time t[PT_EVENT_COUNT]; } pt_day;          /* index with pt_event */

/* Fill *out with the preset for id; PT_OK, or PT_ERR_SETTINGS for PT_CUSTOM / unknown ids. */
PTDEF int  pt_method_preset(pt_method_id id, pt_method *out);        /* PT_OK, or PT_ERR_SETTINGS for PT_CUSTOM / unknown ids */
/* Fill *out with the preset whose key matches exactly (case-sensitive); PT_ERR_SETTINGS if none. */
PTDEF int  pt_method_by_key(const char *key, pt_method *out);        /* exact, case-sensitive key match; PT_ERR_SETTINGS if none */
/* Set *s to the given method with Standard Asr, no high-latitude rule, no rounding, ramadan 0, zero offsets. */
PTDEF void pt_settings_init(pt_settings *s, const pt_method *m);     /* Standard Asr, no high-latitude rule, no rounding, ramadan 0, offsets 0 */
/* Compute the nine times for the local civil date at utc_offset_min from UTC; 0 on success. */
PTDEF int  pt_compute(int year, int month, int day, int utc_offset_min,
                      double latitude_deg, double longitude_deg,
                      const pt_settings *s, pt_day *out);
/* First of Fajr, Dhuhr, Asr, Maghrib, Isha strictly after now_utc; *prayer and *utc name it. */
PTDEF int  pt_next_prayer(double now_utc, int utc_offset_min, double latitude_deg, double longitude_deg,
                          const pt_settings *s, pt_event *prayer, double *utc);
/* Lowercase name of an event ("fajr" ... "last_third"), "?" otherwise. */
PTDEF const char *pt_event_name(pt_event e);                          /* "fajr", "sunrise", "dhuhr", "asr", "sunset", "maghrib", "isha", "midnight", "last_third"; "?" otherwise */
/* Write the local civil time "HH:MM:SS" (or "--:--:--" when utc is NAN) into buf[9]; returns buf. */
PTDEF char *pt_format_hms(double utc, int utc_offset_min, char buf[9]); /* local "HH:MM:SS", or "--:--:--" when utc is NAN; returns buf */

#ifdef __cplusplus
}
#endif
#endif /* PRAYERTIMES_H */

#ifdef PRAYERTIMES_IMPLEMENTATION
#include <math.h>
#include <stdio.h>
#include <string.h>

/* The algorithms need a 64-bit double (Unix seconds and series terms); this fails to compile otherwise. */
typedef char pt__double_must_be_64_bit[(sizeof(double) >= 8) ? 1 : -1];

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

/* ------------------------------------------------------------------------ */
/* Methods, settings and the prayer-time engine.                             */
/* ------------------------------------------------------------------------ */

/* Built-in method presets, indexed by pt_method_id. */
static const pt_method pt__methods[PT_METHOD_COUNT] = {
    {PT_MWL, "mwl", "Muslim World League", "MWL Fiqh Council res. 6, 9th session (1986), quoted in Dar al-Ifta fatwa 4021 (2017)", 1,
     {PT_RULE_ANGLE, 18, 0}, PT_DHUHR_TRANSIT, {PT_RULE_SUNSET, 0, 0}, {PT_RULE_ANGLE, 17, 0}, PT_MIDNIGHT_SUNSET_TO_SUNRISE, {-2, -2, 2, 2, 2, 2}},
    {PT_ISNA, "isna", "Islamic Society of North America", "Common ISNA 15/15 convention (praytimes.org); FCNA 2011 position differs", 0,
     {PT_RULE_ANGLE, 15, 0}, PT_DHUHR_TRANSIT, {PT_RULE_SUNSET, 0, 0}, {PT_RULE_ANGLE, 15, 0}, PT_MIDNIGHT_SUNSET_TO_SUNRISE, {0, 0, 0, 0, 0, 0}},
    {PT_EGYPT, "egypt", "Egyptian General Authority of Survey", "Egyptian General Authority of Survey, quoted in Dar al-Ifta fatwa 4021 (2017)", 1,
     {PT_RULE_ANGLE, 19.5, 0}, PT_DHUHR_TRANSIT, {PT_RULE_SUNSET, 0, 0}, {PT_RULE_ANGLE, 17.5, 0}, PT_MIDNIGHT_SUNSET_TO_SUNRISE, {0, 0, 0, 0, 0, 0}},
    {PT_UMM_AL_QURA, "ummalqura", "Umm al-Qura, Makkah", "Umm al-Qura: 18.5 deg and 90 min secondary (praytimes.org); Ramadan 120 min Ministry of Islamic Affairs circular (2021)", 0,
     {PT_RULE_ANGLE, 18.5, 0}, PT_DHUHR_TRANSIT, {PT_RULE_SUNSET, 0, 0}, {PT_RULE_MINUTES, 90, 120}, PT_MIDNIGHT_SUNSET_TO_SUNRISE, {0, 0, 0, 0, 0, 0}},
    {PT_KARACHI, "karachi", "University of Islamic Sciences, Karachi", "University of Islamic Sciences, Karachi (praytimes.org)", 0,
     {PT_RULE_ANGLE, 18, 0}, PT_DHUHR_TRANSIT, {PT_RULE_SUNSET, 0, 0}, {PT_RULE_ANGLE, 18, 0}, PT_MIDNIGHT_SUNSET_TO_SUNRISE, {0, 0, 0, 0, 0, 0}},
    {PT_JAFARI, "jafari", "Shia Ithna Ashari, Leva Institute, Qum", "Leva Institute, Qum (praytimes.org)", 0,
     {PT_RULE_ANGLE, 16, 0}, PT_DHUHR_TRANSIT, {PT_RULE_ANGLE, 4, 0}, {PT_RULE_ANGLE, 14, 0}, PT_MIDNIGHT_SUNSET_TO_FAJR, {0, 0, 0, 0, 0, 0}},
    {PT_TEHRAN, "tehran", "Institute of Geophysics, University of Tehran", "Institute of Geophysics, Univ. of Tehran (Khabar Online 2010); Isha 14 deg secondary", 1,
     {PT_RULE_ANGLE, 17.7, 0}, PT_DHUHR_TRANSIT, {PT_RULE_ANGLE, 4.5, 0}, {PT_RULE_ANGLE, 14, 0}, PT_MIDNIGHT_SUNSET_TO_FAJR, {0, 0, 0, 0, 0, 0}},
    {PT_BAHRAIN, "bahrain", "Bahrain", "Supreme Committee for the Bahraini Calendar (2021): Isha 18 deg, Dhuhr after the disk passes the meridian; Fajr 18 deg secondary", 1,
     {PT_RULE_ANGLE, 18, 0}, PT_DHUHR_DISK_PAST_MERIDIAN, {PT_RULE_SUNSET, 0, 0}, {PT_RULE_ANGLE, 18, 0}, PT_MIDNIGHT_SUNSET_TO_SUNRISE, {0, 0, 0, 0, 0, 0}},
    {PT_DIYANET, "diyanet", "Diyanet, Turkey", "Diyanet press statement (2013) and vakithesaplama.diyanet.gov.tr temkin", 1,
     {PT_RULE_ANGLE, 18, 0}, PT_DHUHR_TRANSIT, {PT_RULE_SUNSET, 0, 0}, {PT_RULE_ANGLE, 17, 0}, PT_MIDNIGHT_SUNSET_TO_SUNRISE, {0, -7, 5, 4, 7, 0}},
    {PT_JAKIM, "jakim", "JAKIM, Malaysia", "JAKIM: Subuh 18 deg since 2020 (MKI Muzakarah 116, 2019); Isyak 18 deg secondary", 1,
     {PT_RULE_ANGLE, 18, 0}, PT_DHUHR_TRANSIT, {PT_RULE_SUNSET, 0, 0}, {PT_RULE_ANGLE, 18, 0}, PT_MIDNIGHT_SUNSET_TO_SUNRISE, {0, 0, 0, 0, 0, 0}},
    {PT_MUIS, "muis", "MUIS, Singapore", "MUIS Singapore 20/18 (praytimes.org)", 0,
     {PT_RULE_ANGLE, 20, 0}, PT_DHUHR_TRANSIT, {PT_RULE_SUNSET, 0, 0}, {PT_RULE_ANGLE, 18, 0}, PT_MIDNIGHT_SUNSET_TO_SUNRISE, {0, 0, 0, 0, 0, 0}},
    {PT_KEMENAG, "kemenag", "Kemenag, Indonesia", "Kemenag: Subuh 20 deg (2020); Isya 18 deg secondary", 1,
     {PT_RULE_ANGLE, 20, 0}, PT_DHUHR_TRANSIT, {PT_RULE_SUNSET, 0, 0}, {PT_RULE_ANGLE, 18, 0}, PT_MIDNIGHT_SUNSET_TO_SUNRISE, {0, 0, 0, 0, 0, 0}},
    {PT_TUNISIA, "tunisia", "Tunisia", "Aladhan method list", 0,
     {PT_RULE_ANGLE, 18, 0}, PT_DHUHR_TRANSIT, {PT_RULE_SUNSET, 0, 0}, {PT_RULE_ANGLE, 18, 0}, PT_MIDNIGHT_SUNSET_TO_SUNRISE, {0, 0, 0, 0, 0, 0}},
    {PT_ALGERIA, "algeria", "Algeria", "Aladhan method list", 0,
     {PT_RULE_ANGLE, 18, 0}, PT_DHUHR_TRANSIT, {PT_RULE_SUNSET, 0, 0}, {PT_RULE_ANGLE, 17, 0}, PT_MIDNIGHT_SUNSET_TO_SUNRISE, {0, 0, 0, 0, 0, 0}},
    {PT_DUBAI, "dubai", "Dubai", "adhan-js Dubai preset; not an official UAE method", 0,
     {PT_RULE_ANGLE, 18.2, 0}, PT_DHUHR_TRANSIT, {PT_RULE_SUNSET, 0, 0}, {PT_RULE_ANGLE, 18.2, 0}, PT_MIDNIGHT_SUNSET_TO_SUNRISE, {0, -3, 3, 3, 3, 0}},
    {PT_KUWAIT, "kuwait", "Kuwait", "Aladhan method list", 0,
     {PT_RULE_ANGLE, 18, 0}, PT_DHUHR_TRANSIT, {PT_RULE_SUNSET, 0, 0}, {PT_RULE_ANGLE, 17.5, 0}, PT_MIDNIGHT_SUNSET_TO_SUNRISE, {0, 0, 0, 0, 0, 0}},
    {PT_QATAR, "qatar", "Qatar", "Aladhan / adhan-js Qatar preset", 0,
     {PT_RULE_ANGLE, 18, 0}, PT_DHUHR_TRANSIT, {PT_RULE_SUNSET, 0, 0}, {PT_RULE_MINUTES, 90, 90}, PT_MIDNIGHT_SUNSET_TO_SUNRISE, {0, 0, 0, 0, 0, 0}},
    {PT_FRANCE12, "france12", "France 12 deg (UOIF)", "UOIF 12 deg (praytimes.org)", 0,
     {PT_RULE_ANGLE, 12, 0}, PT_DHUHR_TRANSIT, {PT_RULE_SUNSET, 0, 0}, {PT_RULE_ANGLE, 12, 0}, PT_MIDNIGHT_SUNSET_TO_SUNRISE, {0, 0, 0, 0, 0, 0}},
    {PT_FRANCE15, "france15", "France 15 deg", "France 15 deg variant (al-Kanz)", 0,
     {PT_RULE_ANGLE, 15, 0}, PT_DHUHR_TRANSIT, {PT_RULE_SUNSET, 0, 0}, {PT_RULE_ANGLE, 15, 0}, PT_MIDNIGHT_SUNSET_TO_SUNRISE, {0, 0, 0, 0, 0, 0}},
    {PT_FRANCE18, "france18", "France 18 deg", "France 18 deg variant", 0,
     {PT_RULE_ANGLE, 18, 0}, PT_DHUHR_TRANSIT, {PT_RULE_SUNSET, 0, 0}, {PT_RULE_ANGLE, 18, 0}, PT_MIDNIGHT_SUNSET_TO_SUNRISE, {0, 0, 0, 0, 0, 0}},
    {PT_RUSSIA, "russia", "Spiritual Administration of Muslims of Russia", "Spiritual Administration of Muslims of Russia (praytimes.org)", 0,
     {PT_RULE_ANGLE, 16, 0}, PT_DHUHR_TRANSIT, {PT_RULE_SUNSET, 0, 0}, {PT_RULE_ANGLE, 15, 0}, PT_MIDNIGHT_SUNSET_TO_SUNRISE, {0, 0, 0, 0, 0, 0}},
    {PT_GULF, "gulf", "Gulf region", "Gulf region fixed 90 min (software convention, no issuing authority)", 0,
     {PT_RULE_ANGLE, 19.5, 0}, PT_DHUHR_TRANSIT, {PT_RULE_SUNSET, 0, 0}, {PT_RULE_MINUTES, 90, 90}, PT_MIDNIGHT_SUNSET_TO_SUNRISE, {0, 0, 0, 0, 0, 0}},
};

/* Map an angle to (-180, 180]. */
static double pt__wrap180(double x) {
    x = fmod(x, 360.0);
    if (x <= -180.0) x += 360.0;
    else if (x > 180.0) x -= 360.0;
    return x;
}

/* Local solar transit near the guess t (Newton step on the hour angle). */
static double pt__transit(double t, double lon_deg) {
    int i;
    for (i = 0; i < 10; i++) {
        pt_sun s;
        pt_sun_at(t, &s);
        double H = pt__wrap180(s.gast_deg + lon_deg - s.ra_deg);
        double step = -H / 360.0 * 86400.0;
        t += step;
        if (fabs(step) < 0.001) break;
    }
    return t;
}

/* Time on the given side of transit T (sign -1 morning, +1 evening) when the
   Sun's centre reaches altitude h_deg; NAN when that altitude is never reached. */
static double pt__crossing(double T, double h_deg, int sign, double lat_deg, double lon_deg) {
    double t = T;
    int i;
    for (i = 0; i < 15; i++) {
        pt_sun s;
        pt_sun_at(t, &s);
        double hg = h_deg + (8.794143 / 3600.0) / s.dist_au * cos(pt__rad(h_deg)); /* geocentric target (parallax) */
        double phi = pt__rad(lat_deg), dec = pt__rad(s.dec_deg);
        double c = (sin(pt__rad(hg)) - sin(phi) * sin(dec)) / (cos(phi) * cos(dec));
        if (!(fabs(c) <= 1.0)) return NAN;
        double H0 = pt__deg(acos(c));
        double H = pt__wrap180(s.gast_deg + lon_deg - s.ra_deg);
        double step = pt__wrap180(sign * H0 - H) / 360.0 * 86400.0;
        t += step;
        if (fabs(step) < 0.001) break;
    }
    return t;
}

/* Rise/set altitude of the centre: standard refraction plus the semidiameter. */
static double pt__rs_altitude(double dist_au) {
    return -(34.0 / 60.0 + pt__deg(asin(695700.0 / (149597870.7 * dist_au))));
}

PTDEF int pt_method_preset(pt_method_id id, pt_method *out) {
    if (!out) return PT_ERR_SETTINGS;
    if (!((int)id >= 0 && (int)id < PT_METHOD_COUNT)) return PT_ERR_SETTINGS;
    *out = pt__methods[(int)id];
    return PT_OK;
}

PTDEF int pt_method_by_key(const char *key, pt_method *out) {
    int i;
    if (!key || !out) return PT_ERR_SETTINGS;
    for (i = 0; i < PT_METHOD_COUNT; i++) {
        if (strcmp(pt__methods[i].key, key) == 0) {
            *out = pt__methods[i];
            return PT_OK;
        }
    }
    return PT_ERR_SETTINGS;
}

PTDEF void pt_settings_init(pt_settings *s, const pt_method *m) {
    if (!s) return;
    if (m) s->method = *m;
    s->asr = PT_ASR_STANDARD;
    s->high_latitude = PT_HIGHLAT_NONE;
    s->rounding = PT_ROUND_NONE;
    s->ramadan = 0;
    s->offsets.fajr = s->offsets.sunrise = s->offsets.dhuhr = 0.0;
    s->offsets.asr = s->offsets.maghrib = s->offsets.isha = 0.0;
}

PTDEF const char *pt_event_name(pt_event e) {
    static const char *names[PT_EVENT_COUNT] = {
        "fajr", "sunrise", "dhuhr", "asr", "sunset", "maghrib", "isha", "midnight", "last_third"
    };
    if (!((int)e >= 0 && (int)e < PT_EVENT_COUNT)) return "?";
    return names[(int)e];
}

PTDEF char *pt_format_hms(double utc, int utc_offset_min, char buf[9]) {
    if (!buf) return buf;
    if (isnan(utc)) {
        snprintf(buf, 9, "--:--:--");
        return buf;
    }
    double local = floor(utc + utc_offset_min * 60.0 + 0.5);
    double sod = fmod(local, 86400.0);
    if (sod < 0.0) sod += 86400.0;
    snprintf(buf, 9, "%02d:%02d:%02d",
             (int)(sod / 3600.0), (int)(fmod(sod, 3600.0) / 60.0), (int)fmod(sod, 60.0));
    return buf;
}

/* Validation helpers. */
static int pt__valid_location(double lat_deg, double lon_deg) {
    return isfinite(lat_deg) && isfinite(lon_deg) && fabs(lat_deg) < 90.0
        && lon_deg >= -180.0 && lon_deg <= 180.0;
}
static int pt__leap_year(int y) {
    return y % 4 == 0 && (y % 100 != 0 || y % 400 == 0);
}
static int pt__days_in_month(int y, int m) {
    static const int len[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (m == 2 && pt__leap_year(y)) return 29;
    return len[m - 1];
}
static int pt__valid_date(int year, int month, int day, int utc_offset_min) {
    return year >= 1 && year <= 9999 && month >= 1 && month <= 12
        && day >= 1 && day <= pt__days_in_month(year, month)
        && utc_offset_min >= -1080 && utc_offset_min <= 1080;
}
static int pt__valid_offset(double v) { return v >= -60.0 && v <= 60.0; }
static int pt__valid_offsets(const pt_offsets *o) {
    return pt__valid_offset(o->fajr) && pt__valid_offset(o->sunrise) && pt__valid_offset(o->dhuhr)
        && pt__valid_offset(o->asr) && pt__valid_offset(o->maghrib) && pt__valid_offset(o->isha);
}
/* allowed is a bitmask of the pt_rule_kind bits this prayer accepts. */
static int pt__valid_rule(const pt_rule *r, int allowed) {
    if (!((int)r->kind >= 0 && (int)r->kind <= PT_RULE_SUNSET)) return 0;
    if (!(allowed & (1 << (int)r->kind))) return 0;
    switch ((int)r->kind) {
    case PT_RULE_ANGLE:        return r->value >= 0.0 && r->value <= 30.0;
    case PT_RULE_MINUTES:      return r->value >= 0.0 && r->value <= 240.0
                                 && r->ramadan_value >= 0.0 && r->ramadan_value <= 240.0;
    case PT_RULE_NIGHT_FRACTION: return r->value > 0.0 && r->value < 1.0;
    default:                   return 1; /* PT_RULE_SUNSET */
    }
}
static int pt__valid_settings(const pt_settings *s) {
    if (!s) return 0;
    if (!((int)s->method.id >= 0 && (int)s->method.id < PT_METHOD_COUNT) && s->method.id != PT_CUSTOM) return 0;
    if ((int)s->method.dhuhr < 0 || (int)s->method.dhuhr > PT_DHUHR_DISK_PAST_MERIDIAN) return 0;
    if ((int)s->method.midnight < 0 || (int)s->method.midnight > PT_MIDNIGHT_SUNSET_TO_FAJR) return 0;
    if (s->asr != PT_ASR_STANDARD && s->asr != PT_ASR_HANAFI) return 0;
    if (!((int)s->high_latitude >= 0 && (int)s->high_latitude <= PT_HIGHLAT_ANGLE_BASED)) return 0;
    if (!((int)s->rounding >= 0 && (int)s->rounding <= PT_ROUND_PRECAUTIONARY)) return 0;
    if (!pt__valid_rule(&s->method.fajr, (1 << PT_RULE_ANGLE) | (1 << PT_RULE_NIGHT_FRACTION))) return 0;
    if (!pt__valid_rule(&s->method.maghrib, (1 << PT_RULE_SUNSET) | (1 << PT_RULE_MINUTES) | (1 << PT_RULE_ANGLE))) return 0;
    if (!pt__valid_rule(&s->method.isha, (1 << PT_RULE_ANGLE) | (1 << PT_RULE_MINUTES) | (1 << PT_RULE_NIGHT_FRACTION))) return 0;
    if (!pt__valid_offsets(&s->method.precaution) || !pt__valid_offsets(&s->offsets)) return 0;
    return 1;
}

/* High-latitude portion of the night: 1/2, 1/7, or the prayer's angle / 60. */
static double pt__hl_portion(const pt_settings *s, double angle) {
    switch ((int)s->high_latitude) {
    case PT_HIGHLAT_MIDDLE_OF_NIGHT: return 0.5;
    case PT_HIGHLAT_ONE_SEVENTH:     return 1.0 / 7.0;
    case PT_HIGHLAT_ANGLE_BASED:     return angle / 60.0;
    default:                         return 0.0;
    }
}

PTDEF int pt_compute(int year, int month, int day, int utc_offset_min,
                     double latitude_deg, double longitude_deg,
                     const pt_settings *s, pt_day *out) {
    pt_day d;
    double t0, guess, T, Tp, Tn, dT, h_rs;
    double sunrise, sunset, sunset_prev, sunrise_next;
    double night_before, night_after;
    double fajr, dhuhr, asr, maghrib, isha, midnight, last_third;
    int fajr_adj = 0, maghrib_adj = 0, isha_adj = 0, hl, i;
    pt_sun sunT;

    if (!s || !out) return PT_ERR_SETTINGS;
    if (!pt__valid_location(latitude_deg, longitude_deg)) return PT_ERR_LOCATION;
    if (!pt__valid_date(year, month, day, utc_offset_min)) return PT_ERR_DATE;
    if (!pt__valid_settings(s)) return PT_ERR_SETTINGS;

    /* 1. Transits and Dhuhr. */
    t0 = pt_unix_from_civil(year, month, day, 0.0) - utc_offset_min * 60.0;
    guess = pt_unix_from_civil(year, month, day, 43200.0) - longitude_deg * 240.0;
    T = pt__transit(guess, longitude_deg);
    if (T < t0) T = pt__transit(guess + 86400.0, longitude_deg);
    if (T >= t0 + 86400.0) T = pt__transit(guess - 86400.0, longitude_deg);
    Tp = pt__transit(T - 86400.0, longitude_deg);
    Tn = pt__transit(T + 86400.0, longitude_deg);
    pt_sun_at(T, &sunT);
    dT = sunT.dec_deg;
    dhuhr = (s->method.dhuhr == PT_DHUHR_DISK_PAST_MERIDIAN) ? T + 64.0 / cos(pt__rad(dT)) : T;

    /* 2. Sunrise and sunset (upper limb). */
    h_rs = pt__rs_altitude(sunT.dist_au);
    sunrise = pt__crossing(T, h_rs, -1, latitude_deg, longitude_deg);
    sunset = pt__crossing(T, h_rs, +1, latitude_deg, longitude_deg);
    sunset_prev = pt__crossing(Tp, h_rs, +1, latitude_deg, longitude_deg);
    sunrise_next = pt__crossing(Tn, h_rs, -1, latitude_deg, longitude_deg);

    /* 3. Nights. */
    night_before = sunrise - sunset_prev;
    night_after = sunrise_next - sunset;

    /* 4. Fajr. */
    if (s->method.fajr.kind == PT_RULE_ANGLE)
        fajr = pt__crossing(T, -s->method.fajr.value, -1, latitude_deg, longitude_deg);
    else
        fajr = sunrise - s->method.fajr.value * night_before;

    /* 5. Asr. */
    {
        double tf = (double)s->asr;
        /* Noon altitude of the Sun's centre as seen by the observer (parallax lowers it). */
        double hn = 90.0 - fabs(latitude_deg - dT);
        double h;
        hn -= (8.794143 / 3600.0) / sunT.dist_au * cos(pt__rad(hn));
        h = pt__deg(atan(1.0 / (tf + tan(pt__rad(90.0 - hn)))));
        asr = h > 0.0 ? pt__crossing(T, h, +1, latitude_deg, longitude_deg) : NAN;
    }

    /* 6. Maghrib. */
    switch ((int)s->method.maghrib.kind) {
    case PT_RULE_SUNSET:  maghrib = sunset; break;
    case PT_RULE_MINUTES: maghrib = sunset + 60.0 * s->method.maghrib.value; break;
    default:              maghrib = pt__crossing(T, -s->method.maghrib.value, +1, latitude_deg, longitude_deg); break;
    }

    /* 7. Isha. */
    switch ((int)s->method.isha.kind) {
    case PT_RULE_ANGLE:   isha = pt__crossing(T, -s->method.isha.value, +1, latitude_deg, longitude_deg); break;
    case PT_RULE_MINUTES: isha = maghrib + 60.0 * (s->ramadan ? s->method.isha.ramadan_value : s->method.isha.value); break;
    default:              isha = sunset + s->method.isha.value * night_after; break;
    }

    /* 8. High-latitude rule (angle rules only). */
    hl = (s->high_latitude != PT_HIGHLAT_NONE);
    if (hl && s->method.fajr.kind == PT_RULE_ANGLE && !isnan(night_before)) {
        double limit = sunrise - pt__hl_portion(s, s->method.fajr.value) * night_before;
        if (isnan(fajr) || fajr < limit) { fajr = limit; fajr_adj = 1; }
    }
    if (hl && s->method.maghrib.kind == PT_RULE_ANGLE && !isnan(night_after)) {
        double limit = sunset + pt__hl_portion(s, s->method.maghrib.value) * night_after;
        if (isnan(maghrib) || maghrib > limit) { maghrib = limit; maghrib_adj = 1; }
    }
    if (hl && s->method.isha.kind == PT_RULE_ANGLE && !isnan(night_after)) {
        double limit = sunset + pt__hl_portion(s, s->method.isha.value) * night_after;
        if (isnan(isha) || isha > limit) { isha = limit; isha_adj = 1; }
    }
    if (maghrib_adj && s->method.isha.kind == PT_RULE_MINUTES) {
        isha = maghrib + 60.0 * (s->ramadan ? s->method.isha.ramadan_value : s->method.isha.value);
        isha_adj = 1;
    }

    /* 9. Midnight and last third. */
    {
        double end;
        if (s->method.midnight == PT_MIDNIGHT_SUNSET_TO_FAJR) {
            if (s->method.fajr.kind == PT_RULE_ANGLE) {
                end = pt__crossing(Tn, -s->method.fajr.value, -1, latitude_deg, longitude_deg);
                if (hl && !isnan(night_after)) {
                    double limit = sunrise_next - pt__hl_portion(s, s->method.fajr.value) * night_after;
                    if (isnan(end) || end < limit) end = limit;
                }
            } else {
                end = sunrise_next - s->method.fajr.value * night_after;
            }
        } else {
            end = sunrise_next;
        }
        midnight = sunset + (end - sunset) / 2.0;
        last_third = sunset + 2.0 * (end - sunset) / 3.0;
    }

    /* 10. Offsets: method precaution plus user minutes, sunset/midnight/last third get none. */
    fajr    += 60.0 * (s->method.precaution.fajr    + s->offsets.fajr);
    sunrise += 60.0 * (s->method.precaution.sunrise + s->offsets.sunrise);
    dhuhr   += 60.0 * (s->method.precaution.dhuhr   + s->offsets.dhuhr);
    asr     += 60.0 * (s->method.precaution.asr     + s->offsets.asr);
    maghrib += 60.0 * (s->method.precaution.maghrib + s->offsets.maghrib);
    isha    += 60.0 * (s->method.precaution.isha    + s->offsets.isha);

    /* 11-12. Statuses and rounding. */
    d.t[PT_FAJR].utc = fajr;        d.t[PT_FAJR].status = fajr_adj ? PT_ADJUSTED : PT_COMPUTED;
    d.t[PT_SUNRISE].utc = sunrise;  d.t[PT_SUNRISE].status = PT_COMPUTED;
    d.t[PT_DHUHR].utc = dhuhr;      d.t[PT_DHUHR].status = PT_COMPUTED;
    d.t[PT_ASR].utc = asr;          d.t[PT_ASR].status = PT_COMPUTED;
    d.t[PT_SUNSET].utc = sunset;    d.t[PT_SUNSET].status = PT_COMPUTED;
    d.t[PT_MAGHRIB].utc = maghrib;  d.t[PT_MAGHRIB].status = maghrib_adj ? PT_ADJUSTED : PT_COMPUTED;
    d.t[PT_ISHA].utc = isha;        d.t[PT_ISHA].status = isha_adj ? PT_ADJUSTED : PT_COMPUTED;
    d.t[PT_MIDNIGHT].utc = midnight;    d.t[PT_MIDNIGHT].status = PT_COMPUTED;
    d.t[PT_LAST_THIRD].utc = last_third; d.t[PT_LAST_THIRD].status = PT_COMPUTED;
    for (i = 0; i < PT_EVENT_COUNT; i++) {
        double t = d.t[i].utc;
        if (isnan(t)) { d.t[i].utc = NAN; d.t[i].status = PT_NOT_REACHED; continue; }
        if (s->rounding == PT_ROUND_NEAREST)
            t = floor((t + 30.0) / 60.0) * 60.0;
        else if (s->rounding == PT_ROUND_PRECAUTIONARY)
            t = (i == PT_SUNRISE || i == PT_MIDNIGHT) ? floor(t / 60.0) * 60.0 : ceil(t / 60.0) * 60.0;
        d.t[i].utc = t;
    }
    *out = d;
    return PT_OK;
}

PTDEF int pt_next_prayer(double now_utc, int utc_offset_min, double latitude_deg, double longitude_deg,
                         const pt_settings *s, pt_event *prayer, double *utc) {
    static const pt_event order[5] = {PT_FAJR, PT_DHUHR, PT_ASR, PT_MAGHRIB, PT_ISHA};
    int y, m, d, i, k;
    double sod, best = NAN;
    pt_event best_ev = PT_FAJR;

    if (!s || !prayer || !utc) return PT_ERR_SETTINGS;
    if (!pt__valid_location(latitude_deg, longitude_deg)) return PT_ERR_LOCATION;
    if (!isfinite(now_utc)) return PT_ERR_DATE;
    if (!pt__valid_settings(s)) return PT_ERR_SETTINGS;

    pt_civil_from_unix(now_utc + utc_offset_min * 60.0, &y, &m, &d, &sod);
    for (k = 0; k < 3; k++) {
        pt_day day;
        int rc = pt_compute(y, m, d, utc_offset_min, latitude_deg, longitude_deg, s, &day);
        if (rc != PT_OK) return rc;
        for (i = 0; i < 5; i++) {
            double t = day.t[order[i]].utc;
            if (!isnan(t) && t > now_utc && (isnan(best) || t < best)) { best = t; best_ev = order[i]; }
        }
        pt_civil_from_unix(pt_unix_from_civil(y, m, d, 0.0) + 86400.0, &y, &m, &d, &sod);
    }
    if (isnan(best)) return PT_ERR_NO_NEXT;
    *prayer = best_ev;
    *utc = best;
    return PT_OK;
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
