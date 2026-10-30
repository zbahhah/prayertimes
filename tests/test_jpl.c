/* test_jpl: library event times against JPL Horizons Sun tables.
   Reference times come from linear interpolation between the minute rows
   of tests/data/horizons/<Site>_<Date>.txt. */
#define PRAYERTIMES_IMPLEMENTATION
#include "prayertimes.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define PI 3.14159265358979323846
#define MAXROWS 2048

static double rad(double d) { return d * PI / 180.0; }
static double deg(double r) { return r * 180.0 / PI; }

enum { K_DHUHR, K_SUNRISE, K_SUNSET, K_FAJR, K_ISHA, K_MAGHRIB, K_ASR1, K_ASR2, KIND_COUNT };
static const char *kind_names[KIND_COUNT] = { "dhuhr", "sunrise", "sunset", "fajr", "isha", "maghrib", "asr1", "asr2" };

typedef struct { int n; double sum, worst; char where[96]; } stat;
static stat stats[KIND_COUNT];

static char fails[16][600];
static int nfail;

static void add_fail(const char *msg) {
    if (nfail < 16)
        snprintf(fails[nfail], sizeof fails[0], "%s", msg);
    nfail++;
}

static void record(int kind, double diff, const char *file, double angle) {
    stat *st = &stats[kind];
    st->n++;
    st->sum += diff;
    if (fabs(diff) > st->worst) {
        st->worst = fabs(diff);
        if (angle > 0.0)
            snprintf(st->where, sizeof st->where, "%s a=%g", file, angle);
        else
            snprintf(st->where, sizeof st->where, "%s", file);
    }
    if (!(fabs(diff) <= 1.0)) {
        char msg[128];
        snprintf(msg, sizeof msg, "%s %s: diff %+.2f s exceeds 1.0 s", kind_names[kind], file, diff);
        add_fail(msg);
    }
}

/* Compare one library time with one JPL time; difference in seconds. */
static void compare(int kind, const pt_day *d, pt_event ev, double jpl, const char *file, double angle) {
    char msg[128];
    if (d->t[ev].status != PT_COMPUTED || !(d->t[ev].utc == d->t[ev].utc)) {
        snprintf(msg, sizeof msg, "%s %s: library time not computed", kind_names[kind], file);
        add_fail(msg);
        return;
    }
    if (!(jpl == jpl)) {
        snprintf(msg, sizeof msg, "%s %s: JPL crossing not found", kind_names[kind], file);
        add_fail(msg);
        return;
    }
    record(kind, d->t[ev].utc - jpl, file, angle);
}

static double interp_time(double t0, double t1, double v0, double v1, double v) {
    return t0 + (v - v0) / (v1 - v0) * (t1 - t0);
}

/* Morning crossing of h: last pair before the transit (between rows ti and
   ti+1) with elev[i] < h <= elev[i+1]. Evening crossing: first pair after
   the transit with elev[i] >= h > elev[i+1]. NAN when not found. */
static double crossing(const double *t, const double *elev, int n, int ti, double h, int evening) {
    int i;
    if (!evening) {
        for (i = ti; i >= 0; i--)
            if (elev[i] < h && h <= elev[i + 1])
                return interp_time(t[i], t[i + 1], elev[i], elev[i + 1], h);
    } else {
        for (i = ti + 1; i + 1 < n; i++)
            if (elev[i] >= h && h > elev[i + 1])
                return interp_time(t[i], t[i + 1], elev[i], elev[i + 1], h);
    }
    return NAN;
}

static void make_method(pt_method *m, pt_rule fajr, pt_rule maghrib, pt_rule isha) {
    memset(m, 0, sizeof *m);
    m->id = PT_CUSTOM;
    m->key = "custom";
    m->name = "custom";
    m->source = "test";
    m->dhuhr = PT_DHUHR_TRANSIT;
    m->midnight = PT_MIDNIGHT_SUNSET_TO_SUNRISE;
    m->fajr = fajr;
    m->maghrib = maghrib;
    m->isha = isha;
}

int main(int argc, char **argv) {
    static const char *sites[] = { "Makkah", "Madinah", "AlQuds", "Riyadh", "Damascus", "Baghdad" };
    static const char *dates[] = { "2026-03-20", "2026-06-21", "2026-10-30", "2026-12-21" };
    static const char *month_names = "JanFebMarAprMayJunJulAugSepOctNovDec";
    static const double fajr_angles[] = { 12, 15, 18, 18.5, 19.5, 20 };
    static const double isha_angles[] = { 12, 14, 15, 17, 17.5, 18 };
    static const double maghrib_angles[] = { 4, 4.5 };
    static double t[MAXROWS], elev[MAXROWS], dec[MAXROWS], ha[MAXROWS];
    static int on_date[MAXROWS];
    const char *data = argc > 1 ? argv[1] : "tests/data";
    int si, di, i;

    for (si = 0; si < 6; si++) {
        for (di = 0; di < 4; di++) {
            char path[512], file[64], line[512], msg[600];
            FILE *f;
            int lineno = 0, n = 0, ti = -1;
            double lat = 0.0, lon = 0.0, t_transit = NAN, dec_transit = NAN, h_rs = NAN, jpl;
            pt_method m;
            pt_settings s;
            pt_day d;
            int y, mo, dd;
            pt_rule r_sunset = { PT_RULE_SUNSET, 0, 0 };
            pt_rule r18 = { PT_RULE_ANGLE, 18, 0 };

            snprintf(file, sizeof file, "%s_%s", sites[si], dates[di]);
            snprintf(path, sizeof path, "%s/horizons/%s.txt", data, file);
            f = fopen(path, "r");
            if (!f) {
                snprintf(msg, sizeof msg, "cannot open %s", path);
                add_fail(msg);
                continue;
            }
            sscanf(dates[di], "%d-%d-%d", &y, &mo, &dd);
            {
                double day0 = pt_unix_from_civil(y, mo, dd, 0.0);
                while (fgets(line, sizeof line, f)) {
                    if (line[0] == '#') {
                        if (lineno == 1) { /* second line: "lat <number> lon <number>" */
                            const char *pl = strstr(line, "lat ");
                            const char *po = strstr(line, "lon ");
                            if (pl) lat = strtod(pl + 4, NULL);
                            if (po) lon = strtod(po + 4, NULL);
                        }
                        lineno++;
                        continue;
                    }
                    if (n < MAXROWS) {
                        int yy, dday, hh, mi;
                        char mons[4];
                        if (sscanf(line, "%d-%3s-%d %d:%d %lf %lf %lf",
                                   &yy, mons, &dday, &hh, &mi, &elev[n], &dec[n], &ha[n]) == 8) {
                            const char *mp = strstr(month_names, mons);
                            if (mp) {
                                int mnum = (int)(mp - month_names) / 3 + 1;
                                t[n] = pt_unix_from_civil(yy, mnum, dday, hh * 3600.0 + mi * 60.0);
                                on_date[n] = (t[n] >= day0 && t[n] < day0 + 86400.0);
                                n++;
                            }
                        }
                    }
                    lineno++;
                }
            }
            fclose(f);

            if (n < 100) {
                snprintf(msg, sizeof msg, "%s: too few rows (%d)", file, n);
                add_fail(msg);
                continue;
            }
            /* Transit: consecutive rows on the file's date (UT) where the
               hour angle goes negative -> positive with both |ha| < 1. */
            for (i = 0; i + 1 < n; i++) {
                if (on_date[i] && on_date[i + 1] && ha[i] < 0.0 && ha[i + 1] > 0.0 &&
                    fabs(ha[i]) < 1.0 && fabs(ha[i + 1]) < 1.0) {
                    double frac = (0.0 - ha[i]) / (ha[i + 1] - ha[i]);
                    ti = i;
                    t_transit = t[i] + frac * (t[i + 1] - t[i]);
                    dec_transit = dec[i] + frac * (dec[i + 1] - dec[i]);
                    break;
                }
            }
            if (ti < 0) {
                snprintf(msg, sizeof msg, "%s: transit not found", file);
                add_fail(msg);
                continue;
            }
            {
                pt_sun sun;
                pt_sun_at(t_transit, &sun);
                h_rs = -(34.0 / 60.0 + deg(asin(695700.0 / (149597870.7 * sun.dist_au))));
            }

            /* dhuhr vs transit. */
            make_method(&m, r18, r_sunset, r18);
            pt_settings_init(&s, &m);
            if (pt_compute(y, mo, dd, 180, lat, lon, &s, &d) == PT_OK)
                compare(K_DHUHR, &d, PT_DHUHR, t_transit, file, 0.0);
            else {
                snprintf(msg, sizeof msg, "%s: pt_compute failed (dhuhr case)", file);
                add_fail(msg);
            }

            /* sunrise / sunset vs crossings of h_rs (Maghrib rule SUNSET). */
            make_method(&m, r18, r_sunset, r18);
            pt_settings_init(&s, &m);
            if (pt_compute(y, mo, dd, 180, lat, lon, &s, &d) == PT_OK) {
                compare(K_SUNRISE, &d, PT_SUNRISE, crossing(t, elev, n, ti, h_rs, 0), file, 0.0);
                compare(K_SUNSET, &d, PT_SUNSET, crossing(t, elev, n, ti, h_rs, 1), file, 0.0);
            } else {
                snprintf(msg, sizeof msg, "%s: pt_compute failed (sunrise/sunset case)", file);
                add_fail(msg);
            }

            /* fajr ANGLE a vs morning crossing of -a. */
            for (i = 0; i < 6; i++) {
                pt_rule r = { PT_RULE_ANGLE, fajr_angles[i], 0 };
                make_method(&m, r, r_sunset, r18);
                pt_settings_init(&s, &m);
                jpl = crossing(t, elev, n, ti, -fajr_angles[i], 0);
                if (pt_compute(y, mo, dd, 180, lat, lon, &s, &d) == PT_OK)
                    compare(K_FAJR, &d, PT_FAJR, jpl, file, fajr_angles[i]);
                else {
                    snprintf(msg, sizeof msg, "%s: pt_compute failed (fajr a=%g)", file, fajr_angles[i]);
                    add_fail(msg);
                }
            }

            /* isha ANGLE a vs evening crossing of -a. */
            for (i = 0; i < 6; i++) {
                pt_rule r = { PT_RULE_ANGLE, isha_angles[i], 0 };
                make_method(&m, r18, r_sunset, r);
                pt_settings_init(&s, &m);
                jpl = crossing(t, elev, n, ti, -isha_angles[i], 1);
                if (pt_compute(y, mo, dd, 180, lat, lon, &s, &d) == PT_OK)
                    compare(K_ISHA, &d, PT_ISHA, jpl, file, isha_angles[i]);
                else {
                    snprintf(msg, sizeof msg, "%s: pt_compute failed (isha a=%g)", file, isha_angles[i]);
                    add_fail(msg);
                }
            }

            /* maghrib ANGLE a vs evening crossing of -a. */
            for (i = 0; i < 2; i++) {
                pt_rule r = { PT_RULE_ANGLE, maghrib_angles[i], 0 };
                make_method(&m, r18, r, r18);
                pt_settings_init(&s, &m);
                jpl = crossing(t, elev, n, ti, -maghrib_angles[i], 1);
                if (pt_compute(y, mo, dd, 180, lat, lon, &s, &d) == PT_OK)
                    compare(K_MAGHRIB, &d, PT_MAGHRIB, jpl, file, maghrib_angles[i]);
                else {
                    snprintf(msg, sizeof msg, "%s: pt_compute failed (maghrib a=%g)", file, maghrib_angles[i]);
                    add_fail(msg);
                }
            }

            /* asr: JPL altitude h = deg(atan(1/(tf + tan(|lat - delta|))))
               with delta interpolated at the transit; evening crossing of h. */
            for (i = 0; i < 2; i++) {
                double tf = (i == 0) ? 1.0 : 2.0;
                double h_asr = deg(atan(1.0 / (tf + tan(rad(fabs(lat - dec_transit))))));
                make_method(&m, r18, r_sunset, r18);
                pt_settings_init(&s, &m);
                s.asr = (i == 0) ? PT_ASR_STANDARD : PT_ASR_HANAFI;
                jpl = crossing(t, elev, n, ti, h_asr, 1);
                if (pt_compute(y, mo, dd, 180, lat, lon, &s, &d) == PT_OK)
                    compare(i == 0 ? K_ASR1 : K_ASR2, &d, PT_ASR, jpl, file, 0.0);
                else {
                    snprintf(msg, sizeof msg, "%s: pt_compute failed (asr tf=%g)", file, tf);
                    add_fail(msg);
                }
            }
        }
    }

    printf("%-8s %5s %9s %9s  %s\n", "kind", "n", "mean_s", "worst_s", "worst_at");
    for (i = 0; i < KIND_COUNT; i++)
        printf("%-8s %5d %9.2f %9.2f  %s\n", kind_names[i], stats[i].n,
               stats[i].n ? stats[i].sum / stats[i].n : 0.0, stats[i].worst, stats[i].where);

    if (nfail == 0) {
        printf("test_jpl: PASS\n");
        return 0;
    }
    printf("test_jpl: FAIL\n");
    for (i = 0; i < nfail && i < 16; i++)
        printf("  %s\n", fails[i]);
    if (nfail > 16)
        printf("  ... and %d more\n", nfail - 16);
    return 1;
}
