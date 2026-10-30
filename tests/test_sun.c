#define PRAYERTIMES_IMPLEMENTATION
#include "prayertimes.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static char fails[16][600];
static int nfail = 0;

static void add_fail(const char *msg) {
    if (nfail < 16)
        snprintf(fails[nfail], sizeof fails[0], "%s", msg);
    nfail++;
}

int main(int argc, char **argv) {
    static const char *sites[] = { "Makkah", "Madinah", "AlQuds", "Riyadh", "Damascus", "Baghdad" };
    static const char *dates[] = { "2026-03-20", "2026-06-21", "2026-10-30", "2026-12-21" };
    static const char *month_names = "JanFebMarAprMayJunJulAugSepOctNovDec";
    const char *data = argc > 1 ? argv[1] : "tests/data";
    double worst_all = 0.0;
    char worst_file[256] = "", worst_time[16] = "";
    int over_3arcsec = 0;
    int si, di, i;

    for (si = 0; si < 6; si++) {
        for (di = 0; di < 4; di++) {
            char path[512];
            char line[512];
            FILE *f;
            int lineno = 0, rows = 0;
            double lat = 0.0, lon = 0.0, worst = 0.0;

            snprintf(path, sizeof path, "%s/horizons/%s_%s.txt", data, sites[si], dates[di]);
            f = fopen(path, "r");
            if (!f) {
                char msg[600];
                snprintf(msg, sizeof msg, "cannot open %s", path);
                add_fail(msg);
                printf("%s %s: rows 0, worst 0.00 arcsec\n", sites[si], dates[di]);
                continue;
            }
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
                {
                    int yy, dd, hh, mi;
                    char mons[4];
                    double elev, dec, ha;
                    if (sscanf(line, "%d-%3s-%d %d:%d %lf %lf %lf",
                               &yy, mons, &dd, &hh, &mi, &elev, &dec, &ha) == 8) {
                        const char *mp = strstr(month_names, mons);
                        if (mp) {
                            int mnum = (int)(mp - month_names) / 3 + 1;
                            double unix_s = pt_unix_from_civil(yy, mnum, dd, hh * 3600.0 + mi * 60.0);
                            double err = fabs(pt_sun_altitude(unix_s, lat, lon) - elev) * 3600.0;
                            rows++;
                            if (err > worst) worst = err;
                            if (err > 3.0) over_3arcsec++;
                            if (err > worst_all) {
                                worst_all = err;
                                snprintf(worst_file, sizeof worst_file, "%s_%s", sites[si], dates[di]);
                                snprintf(worst_time, sizeof worst_time, "%02d:%02d", hh, mi);
                            }
                        }
                    }
                }
                lineno++;
            }
            fclose(f);
            printf("%s %s: rows %d, worst %.2f arcsec\n", sites[si], dates[di], rows, worst);
        }
    }

    /* Civil date helpers. */
    if (pt_unix_from_civil(1970, 1, 1, 0.0) != 0.0)
        add_fail("pt_unix_from_civil(1970,1,1,0) != 0");
    if (pt_unix_from_civil(2000, 1, 1, 43200.0) != 946728000.0)
        add_fail("pt_unix_from_civil(2000,1,1,43200) != 946728000");
    {
        int y, m, d;
        double sod;
        pt_civil_from_unix(pt_unix_from_civil(2026, 10, 30, 12 * 3600.0 + 34 * 60.0 + 56.0), &y, &m, &d, &sod);
        if (y != 2026 || m != 10 || d != 30 || sod != 12 * 3600.0 + 34 * 60.0 + 56.0)
            add_fail("round-trip 2026-10-30 12:34:56 failed");
        pt_civil_from_unix(pt_unix_from_civil(1999, 12, 31, 23 * 3600.0 + 59 * 60.0 + 59.0), &y, &m, &d, &sod);
        if (y != 1999 || m != 12 || d != 31 || sod != 23 * 3600.0 + 59 * 60.0 + 59.0)
            add_fail("round-trip 1999-12-31 23:59:59 failed");
    }

    /* Delta T sanity for 2026. */
    {
        double dt = pt_delta_t(pt_unix_from_civil(2026, 1, 1, 0.0));
        if (!(dt > 70.0 && dt < 80.0))
            add_fail("pt_delta_t(2026-01-01) not between 70 and 80");
    }

    if (over_3arcsec > 0) {
        char msg[96];
        snprintf(msg, sizeof msg, "%d rows over 3.0 arcsec (worst %.2f)", over_3arcsec, worst_all);
        add_fail(msg);
    }

    printf("test_sun: worst altitude %.2f arcsec (%s %s)\n", worst_all, worst_file, worst_time);
    if (nfail == 0) {
        printf("test_sun: PASS\n");
        return 0;
    }
    printf("test_sun: FAIL\n");
    for (i = 0; i < nfail && i < 16; i++)
        printf("  %s\n", fails[i]);
    if (nfail > 16)
        printf("  ... and %d more\n", nfail - 16);
    return 1;
}
