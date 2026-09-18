#!/usr/bin/env python3
"""Clean NOAA GSOM monthly temperature files for Sarajevo and Bjelasnica.

Usage:
    python clean_data.py sarajevo_raw.csv bjelasnica_raw.csv

Outputs:
    data/sarajevo_temperature.csv
    data/bjelasnica_temperature.csv
    data/sarajevo_bjelasnica_temperature.csv
"""

from __future__ import annotations

import csv
import math
import re
import sys
from datetime import date
from pathlib import Path

START_DATE = date(2000, 1, 1)
END_DATE = date(2026, 8, 1)
OUTPUT_DIR = Path(__file__).resolve().parent

# NOAA commonly uses these values for missing observations.
MISSING_VALUES = {"", ".", "NA", "N/A", "NULL", "-9999", "-999.9", "-9999.0"}


def normalize_name(value: str) -> str:
    return re.sub(r"[^a-z0-9]", "", value.lower())


def find_column(fieldnames: list[str], candidates: list[str]) -> str | None:
    normalized = {normalize_name(name): name for name in fieldnames if name is not None}
    for candidate in candidates:
        if normalize_name(candidate) in normalized:
            return normalized[normalize_name(candidate)]
    return None


def parse_date(value: str) -> date | None:
    value = value.strip()
    formats = ["%Y-%m-%d", "%Y%m%d", "%Y-%m", "%Y%m"]
    for fmt in formats:
        try:
            parsed = date.fromisoformat(value) if fmt == "%Y-%m-%d" else date(*__import__("datetime").datetime.strptime(value, fmt).timetuple()[:3])
            return parsed.replace(day=1)
        except ValueError:
            pass

    # Also handle timestamps such as 2000-01-01T00:00:00.
    match = re.match(r"^(\d{4})[-/]?(\d{2})", value)
    if match:
        return date(int(match.group(1)), int(match.group(2)), 1)
    return None


def parse_temperature(value: str) -> float | None:
    text = value.strip().replace(",", ".")
    if text.upper() in MISSING_VALUES:
        return None
    try:
        number = float(text)
    except ValueError:
        return None
    if not math.isfinite(number) or number <= -900:
        return None
    return number


def read_station_file(path: Path) -> dict[tuple[int, int], float]:
    with path.open("r", encoding="utf-8-sig", newline="") as file:
        sample = file.read(4096)
        file.seek(0)
        try:
            dialect = csv.Sniffer().sniff(sample, delimiters=",;\t")
        except csv.Error:
            dialect = csv.excel
        reader = csv.DictReader(file, dialect=dialect)
        if not reader.fieldnames:
            raise ValueError(f"{path}: no CSV header found")

        date_col = find_column(reader.fieldnames, [
            "DATE", "DATE_TIME", "DATETIME", "YEAR_MONTH", "MONTH", "TIME"
        ])
        temp_col = find_column(reader.fieldnames, [
            "TEMP", "TAVG", "TMEAN", "MEAN_TEMPERATURE", "MONTHLY_MEAN_TEMPERATURE",
            "MEAN TEMPERATURE"
        ])

        if date_col is None or temp_col is None:
            raise ValueError(
                f"{path}: could not find date and temperature columns.\n"
                f"Columns found: {reader.fieldnames}"
            )

        result: dict[tuple[int, int], float] = {}
        for line_number, row in enumerate(reader, start=2):
            parsed_date = parse_date(row.get(date_col, ""))
            temperature = parse_temperature(row.get(temp_col, ""))
            if parsed_date is None or temperature is None:
                continue
            if START_DATE <= parsed_date <= END_DATE:
                result[(parsed_date.year, parsed_date.month)] = temperature

        if not result:
            raise ValueError(f"{path}: no valid observations found")
        return result


def write_clean_file(path: Path, observations: dict[tuple[int, int], float]) -> None:
    first_year, first_month = min(observations)
    start_index = first_year * 12 + first_month

    with path.open("w", encoding="utf-8", newline="") as file:
        writer = csv.writer(file)
        writer.writerow(["month", "year", "month_of_year", "temperature_celsius"])
        for (year, month), temperature in sorted(observations.items()):
            month_index = year * 12 + month - start_index
            writer.writerow([month_index, year, month, f"{temperature:.4f}"])


def write_combined_file(path: Path, station_data: dict[str, dict[tuple[int, int], float]]) -> None:
    all_months = sorted(set().union(*(values.keys() for values in station_data.values())))
    start_year, start_month = min(all_months)
    start_index = start_year * 12 + start_month

    with path.open("w", encoding="utf-8", newline="") as file:
        writer = csv.writer(file)
        writer.writerow([
            "month", "year", "month_of_year", "sarajevo_temperature_celsius",
            "bjelasnica_temperature_celsius"
        ])
        for year, month in all_months:
            month_index = year * 12 + month - start_index
            writer.writerow([
                month_index,
                year,
                month,
                format_value(station_data.get("sarajevo", {}).get((year, month))),
                format_value(station_data.get("bjelasnica", {}).get((year, month))),
            ])


def format_value(value: float | None) -> str:
    return "" if value is None else f"{value:.4f}"


def main() -> int:
    if len(sys.argv) != 3:
        print("Usage: python clean_weather_data.py sarajevo_raw.csv bjelasnica_raw.csv")
        return 2

    sarajevo_path = Path(sys.argv[1])
    bjelasnica_path = Path(sys.argv[2])
    OUTPUT_DIR.mkdir(exist_ok=True)

    station_data = {
        "sarajevo": read_station_file(sarajevo_path),
        "bjelasnica": read_station_file(bjelasnica_path),
    }

    write_clean_file(OUTPUT_DIR / "sarajevo_temperature.csv", station_data["sarajevo"])
    write_clean_file(OUTPUT_DIR / "bjelasnica_temperature.csv", station_data["bjelasnica"])
    write_combined_file(OUTPUT_DIR / "sarajevo_bjelasnica_temperature.csv", station_data)

    for station, values in station_data.items():
        print(f"{station}: {len(values)} valid monthly observations")
    print(f"Output directory: {OUTPUT_DIR.resolve()}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
