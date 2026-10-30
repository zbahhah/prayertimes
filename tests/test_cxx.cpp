#define PRAYERTIMES_IMPLEMENTATION
#include "prayertimes.h"
#include <cstdio>
int main() {
    pt_method m; pt_settings s; pt_day d;
    if (pt_method_preset(PT_MWL, &m) != PT_OK) return 1;
    pt_settings_init(&s, &m);
    if (pt_compute(2026, 10, 30, 180, 21.4266, 39.8256, &s, &d) != PT_OK) return 1;
    char buf[9];
    std::printf("test_cxx: dhuhr %s\ntest_cxx: PASS\n", pt_format_hms(d.t[PT_DHUHR].utc, 180, buf));
    return 0;
}
