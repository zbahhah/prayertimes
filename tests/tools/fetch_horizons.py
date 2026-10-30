"""Fetch the JPL Horizons Sun tables in tests/data/horizons. Usage: python fetch_horizons.py <out_dir>"""
import json
import sys
import time
import urllib.parse
import urllib.request
from pathlib import Path

OUT = Path(sys.argv[1])
OUT.mkdir(parents=True, exist_ok=True)

# Test sites (coordinates from GeoNames, CC BY 4.0). Height 0 m: the library
# computes at sea level, as published tables do.
SITES = {
    "Makkah": (21.4266, 39.8256),
    "Madinah": (24.4686, 39.6142),
    "AlQuds": (31.7690, 35.2163),
    "Riyadh": (24.6877, 46.7219),
    "Damascus": (33.5102, 36.2913),
    "Baghdad": (33.3406, 44.4009),
}
DATES = ["2026-03-20", "2026-06-21", "2026-10-30", "2026-12-21"]


def prev_day(d):
    import datetime as dt
    return (dt.date.fromisoformat(d) - dt.timedelta(days=1)).isoformat()


def next_day(d):
    import datetime as dt
    return (dt.date.fromisoformat(d) + dt.timedelta(days=1)).isoformat()


for name, (lat, lon) in SITES.items():
    for d in DATES:
        params = {
            "format": "json",
            "COMMAND": "'10'",
            "OBJ_DATA": "'NO'",
            "MAKE_EPHEM": "'YES'",
            "EPHEM_TYPE": "'OBSERVER'",
            "CENTER": "'coord@399'",
            "COORD_TYPE": "'GEODETIC'",
            "SITE_COORD": f"'{lon},{lat},0'",
            "START_TIME": f"'{prev_day(d)} 18:00'",
            "STOP_TIME": f"'{next_day(d)} 00:00'",
            "STEP_SIZE": "'1 m'",
            "QUANTITIES": "'2,4,42'",
            "APPARENT": "'AIRLESS'",
            "TIME_DIGITS": "'MINUTES'",
            "ANG_FORMAT": "'DEG'",
            "EXTRA_PREC": "'YES'",
            "CSV_FORMAT": "'YES'",
            "TIME_TYPE": "'UT'",
        }
        url = "https://ssd.jpl.nasa.gov/api/horizons.api?" + urllib.parse.urlencode(params)
        with urllib.request.urlopen(url, timeout=120) as r:
            data = json.load(r)
        text = data["result"]
        lines = text.splitlines()
        hdr_idx = next(i for i, l in enumerate(lines) if l.strip().startswith("Date__(UT)"))
        s = lines.index("$$SOE")
        e = lines.index("$$EOE")
        header = [h.strip() for h in lines[hdr_idx].split(",")]
        rows = []
        for l in lines[s + 1 : e]:
            f = [x.strip() for x in l.split(",")]
            rec = dict(zip(header, f))
            ts = rec["Date__(UT)__HR:MN"]
            dec = rec["DEC___(a-app)"]
            elev = rec["Elevation_(a-app)"]
            ha = rec["L_Ap_Hour_Ang"]
            rows.append(f"{ts} {elev} {dec} {ha}")
        dst = OUT / f"{name}_{d}.txt"
        with dst.open("w", newline="\n") as fh:
            fh.write(f"# Source: JPL Horizons API (ssd.jpl.nasa.gov, NASA/JPL), fetched 2026-10-30.\n")
            fh.write(f"# Sun (10), observer geodetic lat {lat} lon {lon} height 0 m (site: {name}).\n")
            fh.write("# APPARENT='AIRLESS' (no refraction), QUANTITIES 2,4,42, step 1 min, times UT.\n")
            fh.write("# Columns: date time_UT elevation_deg declination_deg local_apparent_hour_angle_h\n")
            fh.write("\n".join(rows) + "\n")
        print(name, d, len(rows), flush=True)
        time.sleep(1)
