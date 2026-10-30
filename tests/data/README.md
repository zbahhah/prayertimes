# Test data

## `horizons/`: JPL Horizons Sun tables (independent reference)

One file per site and date, `<Site>_<YYYY-MM-DD>.txt`, for six sites (Makkah, Madinah, AlQuds, Riyadh, Damascus, Baghdad) and four dates (2026-03-20, 2026-06-21, 2026-10-30, 2026-12-21). The second header line of each file gives the observer's geodetic latitude and longitude; the height is 0 m.

Each file runs from 18:00 UT on the previous day to 00:00 UT on the next day, one row per minute. Query: `COMMAND='10'`, `CENTER='coord@399'`, `COORD_TYPE='GEODETIC'`, `APPARENT='AIRLESS'` (no refraction), `QUANTITIES='2,4,42'`, `EXTRA_PREC='YES'`. Each data row has five fields:

```
2026-Mar-19 18:00 -35.042267598 -0.342996083 8.527572919
```

1. date;
2. time (UT);
3. elevation (degrees);
4. apparent declination (degrees);
5. local apparent hour angle (hours).

The tests use linear interpolation between rows to find two things:
- the times when the Sun's centre crosses a geometric altitude;
- the transit, where the hour angle crosses 0.

Source: the JPL Horizons on-line ephemeris system (NASA/JPL), https://ssd.jpl.nasa.gov/horizons/, fetched 2026-10-30 with `tests/tools/fetch_horizons.py`. The site coordinates come from GeoNames (https://www.geonames.org, CC BY 4.0).

## `muis/muis-2024-2026.csv`: MUIS Singapore published timetable

Every day of 2024–2026, in local time (Asia/Singapore, UTC+8). Columns: `date,fajr,sunrise,dhuhr,asr,maghrib,isha` (MUIS names them Subuh, Syuruk, Zohor, Asar, Maghrib, Isyak). MUIS does not publish its reference point, angles, precaution minutes or rounding. The tests therefore check against a tolerance, not an exact match.

Contains information from the "Muslim Prayer Timetable" datasets (data.gov.sg collection 2312) accessed on 30 October 2026 from https://data.gov.sg/collections/2312/view, which is made available under the terms of the Singapore Open Data Licence version 1.0, https://data.gov.sg/open-data-licence. This use does not imply endorsement by MUIS or the Government of Singapore.
