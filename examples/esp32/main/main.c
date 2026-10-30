/* prayertimes ESP-IDF example: compute the nine times for one date and print
   how long a single pt_compute call takes.
   On a real device take `now` from time(NULL) after SNTP sync and the UTC
   offset from your own settings; the library needs no network, file system or heap. */
#define PRAYERTIMES_IMPLEMENTATION
#include "prayertimes.h"
#include <stdio.h>
#include "esp_timer.h"

void app_main(void) {
    pt_method m;
    pt_settings s;
    pt_day d;
    int64_t t0, us;
    int i;

    pt_method_preset(PT_UMM_AL_QURA, &m);
    pt_settings_init(&s, &m);

    /* Makkah, 2026-10-30, UTC+3 */
    t0 = esp_timer_get_time();
    pt_compute(2026, 10, 30, 180, 21.4266, 39.8256, &s, &d);
    us = esp_timer_get_time() - t0;

    for (i = 0; i < PT_EVENT_COUNT; i++) {
        char buf[9];
        printf("%-10s %s\n", pt_event_name((pt_event)i), pt_format_hms(d.t[i].utc, 180, buf));
    }
    printf("pt_compute took %lld us\n", (long long)us);
}
