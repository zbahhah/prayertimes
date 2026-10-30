/* test_muis: library times against the MUIS Singapore published timetable
   (tests/data/muis/muis-2024-2026.csv, local time UTC+8). MUIS does not
   publish its parameters or rounding, so this checks agreement, not identity. */
#define PRAYERTIMES_IMPLEMENTATION
#include "prayertimes.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define EXPECTED_ROWS 1096

static const char *col_names[6] = { "fajr", "sunrise", "dhuhr", "asr", "maghrib", "isha" };
static const pt_event col_ev[6] = { PT_FAJR, PT_SUNRISE, PT_DHUHR, PT_ASR, PT_MAGHRIB, PT_ISHA };

typedef struct { int n; double sum, mn, mx; } colstat;

int main(int argc, char **argv) {
    const char *data = argc > 1 ? argv[1] : "tests/data";
    char path[512], line[512];
    FILE *f;
    colstat cs[6];
    pt_method m;
    pt_settings s;
    int rows = 0, bad = 0, header_seen = 0, i;

    snprintf(path, sizeof path, "%s/muis/muis-2024-2026.csv", data);
    f = fopen(path, "r");
    if (!f) {
        printf("cannot open %s\n", path);
        printf("test_muis: FAIL\n");
        return 1;
    }
    if (pt_method_preset(PT_MUIS, &m) != PT_OK) {
        printf("pt_method_preset(PT_MUIS) failed\n");
        printf("test_muis: FAIL\n");
        fclose(f);
        return 1;
    }
    pt_settings_init(&s, &m); /* defaults: no high-lat rule, rounding NONE, no offsets */

    for (i = 0; i < 6; i++) {
        cs[i].n = 0;
        cs[i].sum = 0.0;
        cs[i].mn = 1e30;
        cs[i].mx = -1e30;
    }

    while (fgets(line, sizeof line, f)) {
        int y, mo, d;
        int hh[6], mm[6];
        pt_day day;
        if (line[0] == '#')
            continue;
        if (!header_seen) { /* the header line right after the comments */
            if (line[0] == '\n' || line[0] == '\r')
                continue;
            header_seen = 1;
            continue;
        }
        if (line[0] == '\n' || line[0] == '\r')
            continue;
        if (sscanf(line, "%d-%d-%d,%d:%d,%d:%d,%d:%d,%d:%d,%d:%d,%d:%d",
                   &y, &mo, &d,
                   &hh[0], &mm[0], &hh[1], &mm[1], &hh[2], &mm[2],
                   &hh[3], &mm[3], &hh[4], &mm[4], &hh[5], &mm[5]) != 15) {
            bad++;
            continue;
        }
        if (pt_compute(y, mo, d, 480, 1.3521, 103.8198, &s, &day) != PT_OK) {
            bad++;
            continue;
        }
        rows++;
        for (i = 0; i < 6; i++) {
            double pub = hh[i] * 60.0 + mm[i];
            double comp, diff;
            if (day.t[col_ev[i]].status == PT_NOT_REACHED || !(day.t[col_ev[i]].utc == day.t[col_ev[i]].utc)) {
                bad++;
                continue;
            }
            comp = fmod(day.t[col_ev[i]].utc + 480.0 * 60.0, 86400.0) / 60.0;
            diff = pub - comp;
            cs[i].n++;
            cs[i].sum += diff;
            if (diff < cs[i].mn) cs[i].mn = diff;
            if (diff > cs[i].mx) cs[i].mx = diff;
            if (!(fabs(diff) <= 3.0))
                bad++;
        }
    }
    fclose(f);

    for (i = 0; i < 6; i++)
        printf("%-8s n=%4d mean=%+.2f min=%+.2f max=%+.2f\n",
               col_names[i], cs[i].n, cs[i].sum / (cs[i].n ? cs[i].n : 1), cs[i].mn, cs[i].mx);
    printf("rows %d\n", rows);

    if (rows != EXPECTED_ROWS) {
        printf("expected %d rows, got %d\n", EXPECTED_ROWS, rows);
        printf("test_muis: FAIL\n");
        return 1;
    }
    if (bad) {
        printf("%d row/column differences over 3.0 min or unreadable rows\n", bad);
        printf("test_muis: FAIL\n");
        return 1;
    }
    printf("test_muis: PASS\n");
    return 0;
}
