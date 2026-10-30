/* prayertimes_cli: print the nine prayer times for one place, date and method.
   Build: cc -std=c99 -I. examples/prayertimes_cli.c -o prayertimes_cli -lm */
#define PRAYERTIMES_IMPLEMENTATION
#include "prayertimes.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(FILE *f) {
    fputs("prayertimes_cli <lat> <lon> <YYYY-MM-DD> <utc-offset-minutes> <method> "
          "[hanafi] [ramadan] [highlat=none|middle|seventh|angle] [round=none|nearest|precaution]\n"
          "prayertimes_cli --methods\n", f);
}

static void print_keys(FILE *f) {
    int id;
    fputs("method keys:", f);
    for (id = 0; id < PT_METHOD_COUNT; id++) {
        pt_method m;
        if (pt_method_preset((pt_method_id)id, &m) == PT_OK)
            fprintf(f, " %s", m.key);
    }
    fputs("\n", f);
}

static int parse_double(const char *s, double *out) {
    char *end;
    double v;
    if (!*s)
        return 0;
    v = strtod(s, &end);
    if (*end != '\0')
        return 0; /* trailing garbage */
    *out = v;
    return 1;
}

static int parse_long(const char *s, long *out) {
    char *end;
    long v;
    if (!*s)
        return 0;
    v = strtol(s, &end, 10);
    if (*end != '\0')
        return 0; /* trailing garbage */
    *out = v;
    return 1;
}

int main(int argc, char **argv) {
    double lat, lon;
    long off;
    int y, mo, d, consumed, i;
    pt_method m;
    pt_settings s;
    pt_day day;

    if (argc == 2 && strcmp(argv[1], "--methods") == 0) {
        for (i = 0; i < PT_METHOD_COUNT; i++) {
            pt_method p;
            if (pt_method_preset((pt_method_id)i, &p) != PT_OK)
                continue;
            printf("%-12s %-44s %-10s %s\n", p.key, p.name,
                   p.official ? "Official" : "Secondary", p.source);
        }
        return 0;
    }
    if (argc < 6) {
        usage(stderr);
        return 2;
    }
    if (!parse_double(argv[1], &lat) || !parse_double(argv[2], &lon) || !parse_long(argv[4], &off)) {
        usage(stderr);
        return 2;
    }
    if (sscanf(argv[3], "%d-%d-%d%n", &y, &mo, &d, &consumed) != 3 ||
        argv[3][consumed] != '\0' || mo < 1 || mo > 12 || d < 1 || d > 31) {
        usage(stderr);
        return 2;
    }
    if (pt_method_by_key(argv[5], &m) != PT_OK) {
        fprintf(stderr, "unknown method \"%s\"\n", argv[5]);
        usage(stderr);
        print_keys(stderr);
        return 2;
    }
    pt_settings_init(&s, &m);
    for (i = 6; i < argc; i++) {
        const char *a = argv[i];
        if (strcmp(a, "hanafi") == 0) {
            s.asr = PT_ASR_HANAFI;
        } else if (strcmp(a, "ramadan") == 0) {
            s.ramadan = 1;
        } else if (strncmp(a, "highlat=", 8) == 0) {
            const char *v = a + 8;
            if (strcmp(v, "none") == 0)
                s.high_latitude = PT_HIGHLAT_NONE;
            else if (strcmp(v, "middle") == 0)
                s.high_latitude = PT_HIGHLAT_MIDDLE_OF_NIGHT;
            else if (strcmp(v, "seventh") == 0)
                s.high_latitude = PT_HIGHLAT_ONE_SEVENTH;
            else if (strcmp(v, "angle") == 0)
                s.high_latitude = PT_HIGHLAT_ANGLE_BASED;
            else {
                usage(stderr);
                return 2;
            }
        } else if (strncmp(a, "round=", 6) == 0) {
            const char *v = a + 6;
            if (strcmp(v, "none") == 0)
                s.rounding = PT_ROUND_NONE;
            else if (strcmp(v, "nearest") == 0)
                s.rounding = PT_ROUND_NEAREST;
            else if (strcmp(v, "precaution") == 0)
                s.rounding = PT_ROUND_PRECAUTIONARY;
            else {
                usage(stderr);
                return 2;
            }
        } else {
            usage(stderr);
            return 2;
        }
    }
    if (pt_compute(y, mo, d, (int)off, lat, lon, &s, &day) != PT_OK) {
        fprintf(stderr, "prayertimes_cli: cannot compute (bad location or date)\n");
        return 1;
    }
    for (i = 0; i < PT_EVENT_COUNT; i++) {
        char buf[9];
        const char *status = (day.t[i].status == PT_COMPUTED) ? "computed"
                           : (day.t[i].status == PT_ADJUSTED)  ? "adjusted"
                                                               : "not reached";
        printf("%-10s %s  %s\n", pt_event_name((pt_event)i), pt_format_hms(day.t[i].utc, (int)off, buf), status);
    }
    return 0;
}
