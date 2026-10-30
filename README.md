# prayertimes.h

A single-file C99 library that computes the Islamic prayer times (Fajr, Sunrise, Dhuhr, Asr, Sunset, Maghrib, Isha, Midnight and the last third of the night) for any place and date.

## Features

- 22 method presets, each with its source and Official/Secondary status (table below).
- Custom rules for Fajr, Maghrib and Isha: sun angle below the horizon, fixed minutes after sunset or Maghrib, or a fraction of the night.
- Asr with the Standard (shadow factor 1) or Hanafi (shadow factor 2) rule.
- High-latitude rules (middle of the night, one seventh, angle-based) with honest statuses: a time produced by a fallback is reported as `PT_ADJUSTED` and a time the Sun never reaches as `PT_NOT_REACHED` - never a silent wrong time.
- Ramadan Isha rule (a separate `ramadan_value` for minute-based Isha).
- Each method carries its own precautionary minutes (tamkin/temkin); users can add their own offsets (-60..60 minutes per time).
- Rounding: none, nearest minute, or precautionary (sunrise and midnight rounded down, the others up).
- No allocation, no global state, C99 and C++.

## Usage

`prayertimes.h` is a single-file (stb-style) library. In *one* C or C++ file, put

```c
#define PRAYERTIMES_IMPLEMENTATION
```

before `#include "prayertimes.h"` to create the implementation; include the header normally everywhere else. Define `PRAYERTIMES_STATIC` as well to make all functions static.

```c
#define PRAYERTIMES_IMPLEMENTATION
#include "prayertimes.h"
#include <stdio.h>

int main(void) {
    pt_method m; pt_settings s; pt_day d; char buf[9];
    pt_method_preset(PT_UMM_AL_QURA, &m);
    pt_settings_init(&s, &m);
    /* Makkah, 2026-10-30, UTC+3 */
    pt_compute(2026, 10, 30, 180, 21.4266, 39.8256, &s, &d);
    for (int e = 0; e < PT_EVENT_COUNT; e++)
        printf("%-10s %s\n", pt_event_name(e), pt_format_hms(d.t[e].utc, 180, buf));
    return 0;
}
```

## Accuracy

TODO: filled in after review

## Methods

| Key | Name | Fajr | Isha | Other rules | Status |
|---|---|---|---|---|---|
| `mwl` | Muslim World League | 18° | 17° | Maghrib at sunset | Official |
| `isna` | Islamic Society of North America | 15° | 15° | Maghrib at sunset | Secondary |
| `egypt` | Egyptian General Authority of Survey | 19.5° | 17.5° | Maghrib at sunset | Official |
| `ummalqura` | Umm al-Qura, Makkah | 18.5° | 90 min after Maghrib (120 in Ramadan) | Maghrib at sunset | Secondary |
| `karachi` | University of Islamic Sciences, Karachi | 18° | 18° | Maghrib at sunset | Secondary |
| `jafari` | Shia Ithna Ashari, Leva Institute, Qum | 16° | 14° | Maghrib 4°, midnight sunset→Fajr | Secondary |
| `tehran` | Institute of Geophysics, University of Tehran | 17.7° | 14° | Maghrib 4.5°, midnight sunset→Fajr | Official |
| `bahrain` | Bahrain | 18° | 18° | Dhuhr after the disk passes the meridian | Official |
| `diyanet` | Diyanet, Turkey | 18° | 17° | temkin: sunrise −7, Dhuhr +5, Asr +4, Maghrib +7 min | Official |
| `jakim` | JAKIM, Malaysia | 18° | 18° | Maghrib at sunset | Official |
| `muis` | MUIS, Singapore | 20° | 18° | Maghrib at sunset | Secondary |
| `kemenag` | Kemenag, Indonesia | 20° | 18° | Maghrib at sunset | Official |
| `tunisia` | Tunisia | 18° | 18° | Maghrib at sunset | Secondary |
| `algeria` | Algeria | 18° | 17° | Maghrib at sunset | Secondary |
| `dubai` | Dubai | 18.2° | 18.2° | offsets: sunrise −3, Dhuhr/Asr/Maghrib +3 min | Secondary |
| `kuwait` | Kuwait | 18° | 17.5° | Maghrib at sunset | Secondary |
| `qatar` | Qatar | 18° | 90 min after Maghrib | Maghrib at sunset | Secondary |
| `france12` | France 12 deg (UOIF) | 12° | 12° | Maghrib at sunset | Secondary |
| `france15` | France 15 deg | 15° | 15° | Maghrib at sunset | Secondary |
| `france18` | France 18 deg | 18° | 18° | Maghrib at sunset | Secondary |
| `russia` | Spiritual Administration of Muslims of Russia | 16° | 15° | Maghrib at sunset | Secondary |
| `gulf` | Gulf region | 19.5° | 90 min after Maghrib | Maghrib at sunset | Secondary |

## Building the tests

On Linux/macOS (GCC or Clang):

```
make test
```

On Windows, with any C99 compiler, e.g.:

```
clang -std=c99 -I. tests/test_jpl.c -o test_jpl.exe
test_jpl.exe tests/data
```

## Data sources

The tests compare against independent references stored under `tests/data/`; `tests/data/README.md` describes their exact provenance and format: Sun tables from the JPL Horizons on-line ephemeris (NASA/JPL) and the MUIS Singapore published timetable, from the "Muslim Prayer Timetable" datasets on data.gov.sg, made available under the Singapore Open Data Licence v1.0.

## Licence

MIT; see `LICENSE`. The truncated VSOP87 Earth series comes from Astronomy Engine (MIT, Don Cross).
