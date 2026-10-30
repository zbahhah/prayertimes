/* test_rules.c - property tests for the PT-0002 rules engine (no expected clock times). */
#define PRAYERTIMES_IMPLEMENTATION
#include "prayertimes.h"

#include <stdio.h>
#include <string.h>

static int failures = 0;

#define CHECK(cond, msg) \
    do { if (!(cond)) { printf("FAIL: %s\n", msg); failures++; } } while (0)

/* Makkah, 2026-10-30, Umm al-Qura, defaults. */
#define MK_LAT 21.4266
#define MK_LON 39.8256
#define MK_OFF 180
static const int Y = 2026, M = 10, D = 30;

static pt_settings makkah_settings(pt_method_id id) {
    pt_method m;
    pt_settings s;
    pt_method_preset(id, &m);
    pt_settings_init(&s, &m);
    return s;
}

int main(void) {
    pt_settings s = makkah_settings(PT_UMM_AL_QURA);
    pt_day day;
    int rc, i, j;

    /* 1. Order and statuses. */
    rc = pt_compute(Y, M, D, MK_OFF, MK_LAT, MK_LON, &s, &day);
    CHECK(rc == PT_OK, "1: compute Makkah failed");
    CHECK(day.t[PT_FAJR].utc < day.t[PT_SUNRISE].utc, "1: fajr < sunrise");
    CHECK(day.t[PT_SUNRISE].utc < day.t[PT_DHUHR].utc, "1: sunrise < dhuhr");
    CHECK(day.t[PT_DHUHR].utc < day.t[PT_ASR].utc, "1: dhuhr < asr");
    CHECK(day.t[PT_ASR].utc < day.t[PT_SUNSET].utc, "1: asr < sunset");
    CHECK(day.t[PT_SUNSET].utc <= day.t[PT_MAGHRIB].utc, "1: sunset <= maghrib");
    CHECK(day.t[PT_MAGHRIB].utc < day.t[PT_ISHA].utc, "1: maghrib < isha");
    CHECK(day.t[PT_ISHA].utc < day.t[PT_MIDNIGHT].utc, "1: isha < midnight");
    CHECK(day.t[PT_MIDNIGHT].utc < day.t[PT_LAST_THIRD].utc, "1: midnight < last_third");
    for (i = 0; i < PT_EVENT_COUNT; i++)
        CHECK(day.t[i].status == PT_COMPUTED, "1: all nine PT_COMPUTED");

    /* 2. Isha 90 min after Maghrib; 120 min in Ramadan. */
    CHECK(day.t[PT_ISHA].utc - day.t[PT_MAGHRIB].utc == 5400.0, "2: isha - maghrib == 5400");
    {
        pt_settings r = makkah_settings(PT_UMM_AL_QURA);
        pt_day rd;
        r.ramadan = 1;
        rc = pt_compute(Y, M, D, MK_OFF, MK_LAT, MK_LON, &r, &rd);
        CHECK(rc == PT_OK && rd.t[PT_ISHA].utc - rd.t[PT_MAGHRIB].utc == 7200.0, "2: ramadan isha - maghrib == 7200");
    }

    /* 3. Hanafi Asr later than Standard Asr. */
    {
        pt_settings h = makkah_settings(PT_UMM_AL_QURA);
        pt_day hd;
        h.asr = PT_ASR_HANAFI;
        rc = pt_compute(Y, M, D, MK_OFF, MK_LAT, MK_LON, &h, &hd);
        CHECK(rc == PT_OK && hd.t[PT_ASR].utc > day.t[PT_ASR].utc, "3: hanafi asr > standard asr");
    }

    /* 4. Rounding. */
    {
        pt_settings rn = makkah_settings(PT_UMM_AL_QURA);
        pt_settings rp = makkah_settings(PT_UMM_AL_QURA);
        pt_day dn, dp;
        int mult = 1;
        rn.rounding = PT_ROUND_NEAREST;
        rp.rounding = PT_ROUND_PRECAUTIONARY;
        rc = pt_compute(Y, M, D, MK_OFF, MK_LAT, MK_LON, &rn, &dn);
        CHECK(rc == PT_OK, "4: nearest compute failed");
        for (i = 0; i < PT_EVENT_COUNT; i++)
            if (fmod(dn.t[i].utc, 60.0) != 0.0) mult = 0;
        CHECK(mult, "4: NEAREST times all multiples of 60");
        rc = pt_compute(Y, M, D, MK_OFF, MK_LAT, MK_LON, &rp, &dp);
        CHECK(rc == PT_OK, "4: precautionary compute failed");
        CHECK(dp.t[PT_SUNRISE].utc <= day.t[PT_SUNRISE].utc, "4: PRECAUTIONARY sunrise <= unrounded");
        CHECK(dp.t[PT_DHUHR].utc >= day.t[PT_DHUHR].utc, "4: PRECAUTIONARY dhuhr >= unrounded");
        CHECK(fmod(dp.t[PT_SUNRISE].utc, 60.0) == 0.0 && fmod(dp.t[PT_DHUHR].utc, 60.0) == 0.0,
              "4: PRECAUTIONARY sunrise/dhuhr multiples of 60");
    }

    /* 5. User offsets. */
    {
        pt_settings o = makkah_settings(PT_UMM_AL_QURA);
        pt_settings bad = makkah_settings(PT_UMM_AL_QURA);
        pt_day od, bd;
        o.offsets.asr = 5.0;
        rc = pt_compute(Y, M, D, MK_OFF, MK_LAT, MK_LON, &o, &od);
        CHECK(rc == PT_OK && od.t[PT_ASR].utc - day.t[PT_ASR].utc == 300.0, "5: asr offset +5 moves asr by 300 s");
        bad.offsets.asr = 61.0;
        CHECK(pt_compute(Y, M, D, MK_OFF, MK_LAT, MK_LON, &bad, &bd) == PT_ERR_SETTINGS, "5: asr offset 61 -> PT_ERR_SETTINGS");
    }

    /* 6. Presets and keys. */
    {
        pt_method ms[PT_METHOD_COUNT];
        int ok = 1, distinct = 1;
        pt_method m;
        for (i = 0; i < PT_METHOD_COUNT; i++)
            if (pt_method_preset((pt_method_id)i, &ms[i]) != PT_OK) ok = 0;
        CHECK(ok, "6: preset succeeds for all 22 ids");
        for (i = 0; i < PT_METHOD_COUNT && distinct; i++)
            for (j = i + 1; j < PT_METHOD_COUNT; j++)
                if (strcmp(ms[i].key, ms[j].key) == 0) distinct = 0;
        CHECK(distinct, "6: all 22 keys distinct");
        CHECK(pt_method_preset(PT_CUSTOM, &m) == PT_ERR_SETTINGS, "6: PT_CUSTOM -> PT_ERR_SETTINGS");
        CHECK(pt_method_by_key("mwl", &m) == PT_OK && m.id == PT_MWL, "6: by_key(\"mwl\") -> PT_MWL");
        CHECK(pt_method_by_key("nope", &m) == PT_ERR_SETTINGS, "6: by_key(\"nope\") -> PT_ERR_SETTINGS");
    }

    /* 7. Location and date validation. */
    {
        pt_day d7;
        CHECK(pt_compute(Y, M, D, MK_OFF, 91.0, MK_LON, &s, &d7) == PT_ERR_LOCATION, "7: lat 91 -> PT_ERR_LOCATION");
        CHECK(pt_compute(Y, M, D, MK_OFF, 90.0, MK_LON, &s, &d7) == PT_ERR_LOCATION, "7: lat 90 -> PT_ERR_LOCATION");
        CHECK(pt_compute(Y, 13, D, MK_OFF, MK_LAT, MK_LON, &s, &d7) == PT_ERR_DATE, "7: month 13 -> PT_ERR_DATE");
        CHECK(pt_compute(Y, 4, 31, MK_OFF, MK_LAT, MK_LON, &s, &d7) == PT_ERR_DATE, "7: Apr 31 -> PT_ERR_DATE");
        CHECK(pt_compute(2025, 2, 29, MK_OFF, MK_LAT, MK_LON, &s, &d7) == PT_ERR_DATE, "7: 2025-02-29 -> PT_ERR_DATE");
        CHECK(pt_compute(2024, 2, 29, MK_OFF, MK_LAT, MK_LON, &s, &d7) == PT_OK, "7: 2024-02-29 -> PT_OK");
    }

    /* 8. MWL dhuhr precaution is exactly +120 s. */
    {
        pt_settings mwl = makkah_settings(PT_MWL);
        pt_settings raw = makkah_settings(PT_MWL);
        pt_day d1, d2;
        memset(&raw.method.precaution, 0, sizeof raw.method.precaution);
        rc = pt_compute(Y, M, D, MK_OFF, MK_LAT, MK_LON, &mwl, &d1);
        CHECK(rc == PT_OK && pt_compute(Y, M, D, MK_OFF, MK_LAT, MK_LON, &raw, &d2) == PT_OK
              && d1.t[PT_DHUHR].utc - d2.t[PT_DHUHR].utc == 120.0, "8: MWL dhuhr precaution == 120 s");
    }

    /* 9. Bahrain DISK_PAST_MERIDIAN: dhuhr 64..75 s after Karachi's. */
    {
        pt_settings bh = makkah_settings(PT_BAHRAIN);
        pt_settings ka = makkah_settings(PT_KARACHI);
        pt_day d1, d2;
        rc = pt_compute(Y, M, D, MK_OFF, MK_LAT, MK_LON, &bh, &d1);
        CHECK(rc == PT_OK && pt_compute(Y, M, D, MK_OFF, MK_LAT, MK_LON, &ka, &d2) == PT_OK
              && d1.t[PT_DHUHR].utc - d2.t[PT_DHUHR].utc > 64.0
              && d1.t[PT_DHUHR].utc - d2.t[PT_DHUHR].utc < 75.0, "9: Bahrain dhuhr - Karachi dhuhr in (64, 75) s");
    }

    /* 10. High latitude: Oslo, 2026-06-21, MWL. */
    {
        pt_settings none = makkah_settings(PT_MWL);
        pt_settings ang = makkah_settings(PT_MWL);
        pt_day o1, o2;
        rc = pt_compute(2026, 6, 21, 120, 59.9139, 10.7522, &none, &o1);
        CHECK(rc == PT_OK, "10: Oslo compute failed");
        CHECK(o1.t[PT_FAJR].status == PT_NOT_REACHED && isnan(o1.t[PT_FAJR].utc), "10: NONE fajr NOT_REACHED");
        CHECK(o1.t[PT_ISHA].status == PT_NOT_REACHED && isnan(o1.t[PT_ISHA].utc), "10: NONE isha NOT_REACHED");
        ang.high_latitude = PT_HIGHLAT_ANGLE_BASED;
        rc = pt_compute(2026, 6, 21, 120, 59.9139, 10.7522, &ang, &o2);
        CHECK(rc == PT_OK, "10: Oslo angle-based compute failed");
        CHECK(o2.t[PT_FAJR].status == PT_ADJUSTED && isfinite(o2.t[PT_FAJR].utc), "10: ANGLE_BASED fajr ADJUSTED, finite");
        CHECK(o2.t[PT_ISHA].status == PT_ADJUSTED && isfinite(o2.t[PT_ISHA].utc), "10: ANGLE_BASED isha ADJUSTED, finite");
        CHECK(o2.t[PT_FAJR].utc < o2.t[PT_SUNRISE].utc, "10: adjusted fajr < sunrise");
        CHECK(o2.t[PT_ISHA].utc > o2.t[PT_MAGHRIB].utc, "10: adjusted isha > maghrib");
    }

    /* 11. Polar night: Tromso, 2026-12-21, MWL, rule NONE. */
    {
        pt_settings mwl = makkah_settings(PT_MWL);
        pt_day t1;
        rc = pt_compute(2026, 12, 21, 60, 69.6492, 18.9553, &mwl, &t1);
        CHECK(rc == PT_OK, "11: Tromso compute failed");
        CHECK(t1.t[PT_SUNRISE].status == PT_NOT_REACHED, "11: polar sunrise NOT_REACHED");
        CHECK(t1.t[PT_SUNSET].status == PT_NOT_REACHED, "11: polar sunset NOT_REACHED");
        CHECK(t1.t[PT_ASR].status == PT_NOT_REACHED, "11: polar asr NOT_REACHED");
        CHECK(t1.t[PT_DHUHR].status == PT_COMPUTED, "11: polar dhuhr PT_COMPUTED");
    }

    /* 12. Southern hemisphere: Cape Town, 2026-12-21, MWL. */
    {
        pt_settings mwl = makkah_settings(PT_MWL);
        pt_day c1;
        int ordered = 1, computed = 1;
        static const pt_event seq[9] = {PT_FAJR, PT_SUNRISE, PT_DHUHR, PT_ASR, PT_SUNSET,
                                        PT_MAGHRIB, PT_ISHA, PT_MIDNIGHT, PT_LAST_THIRD};
        rc = pt_compute(2026, 12, 21, 120, -33.9249, 18.4241, &mwl, &c1);
        CHECK(rc == PT_OK, "12: Cape Town compute failed");
        for (i = 0; i + 1 < 9; i++) {
            double a = c1.t[seq[i]].utc, b = c1.t[seq[i + 1]].utc;
            if (!(a < b || (seq[i] == PT_SUNSET && a == b))) ordered = 0;
        }
        for (i = 0; i < PT_EVENT_COUNT; i++)
            if (c1.t[i].status != PT_COMPUTED) computed = 0;
        CHECK(ordered, "12: Cape Town order fajr < ... < last_third");
        CHECK(computed, "12: Cape Town all nine PT_COMPUTED");
    }

    /* 13. pt_next_prayer at 2026-10-30 09:00 UTC for Makkah. */
    {
        static const pt_event five[5] = {PT_FAJR, PT_DHUHR, PT_ASR, PT_MAGHRIB, PT_ISHA};
        double now = pt_unix_from_civil(2026, 10, 30, 9 * 3600.0);
        pt_event ev;
        double utc;
        double best = 0.0;
        int have = 0, k;
        rc = pt_next_prayer(now, MK_OFF, MK_LAT, MK_LON, &s, &ev, &utc);
        CHECK(rc == PT_OK, "13: pt_next_prayer failed");
        for (k = 0; k < 5; k++) {
            double t = day.t[five[k]].utc;
            if (!isnan(t) && t > now && (!have || t < best)) { best = t; have = 1; }
        }
        CHECK(have && utc == best, "13: next prayer time == smallest of the day's five > now");
        CHECK(day.t[ev].utc == utc, "13: prayer names that time");
    }

    /* 14. pt_format_hms. */
    {
        char buf[9];
        CHECK(pt_format_hms(NAN, 0, buf) == buf && strcmp(buf, "--:--:--") == 0, "14: NAN -> --:--:--");
        pt_format_hms(pt_unix_from_civil(2026, 1, 1, 3661.0), 180, buf);
        CHECK(strcmp(buf, "04:01:01") == 0, "14: 01:01:01 UTC +180 -> 04:01:01");
    }

    if (failures == 0) {
        printf("test_rules: PASS\n");
        return 0;
    }
    printf("test_rules: FAIL (%d failures)\n", failures);
    return 1;
}
