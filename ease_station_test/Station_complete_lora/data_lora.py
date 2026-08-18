# -*- coding: utf-8 -*-
"""Réception série LoRa et serveur Web de la station EASE."""

import csv
import math
import os
import shutil
import threading
import time
import webbrowser
from collections import deque
from datetime import datetime
from pathlib import Path

import serial
from flask import Flask, jsonify, render_template, request
from waitress import serve


BASE_DIR = Path(__file__).resolve().parent
DATA_FILE_PREFIX = "all_sensors_lora"
LEGACY_DATA_FILE = BASE_DIR / f"{DATA_FILE_PREFIX}.csv"
BACKUP_DIR = BASE_DIR / "backups"
BACKUP_INFO_FILE = BACKUP_DIR / "last_backup.txt"

HEADERS = [
    "Date", "Heure", "Temp_BME(C)", "Hum_BME(%)", "Pression(hPa)", "VOC_BME(kOhms)",
    "CO2_SCD30(ppm)", "Temp_SCD30(C)", "Hum_SCD30(%)", "PM1.0", "PM2.5", "PM10",
    "CO2_MHZ16(ppm)", "NO2", "EtOH", "VOC_Multi", "CO", "VOC_SGP40",
    "Temp_DHT20(C)", "Hum_DHT20(%)", "TVOC_SGP30(ppb)", "eCO2_SGP30(ppm)", "HCHO(ppm)",
]

SERIAL_PORT = os.getenv("EASE_SERIAL_PORT", "COM5")
SERIAL_BAUD = int(os.getenv("EASE_SERIAL_BAUD", "115200"))
HOST = os.getenv("EASE_HOST", "0.0.0.0")
PORT = int(os.getenv("EASE_PORT", "5000"))
DEFAULT_HISTORY_LIMIT = 20_000
MAX_HISTORY_LIMIT = 50_000

app = Flask(__name__)
csv_lock = threading.RLock()
latest_data = {}
latest_received_at = None


def weekly_data_file(moment=None):
    """Retourne le fichier de la semaine ISO courante (lundi à dimanche)."""
    iso_year, iso_week, _ = (moment or datetime.now()).isocalendar()
    return BASE_DIR / f"{DATA_FILE_PREFIX}_{iso_year}-W{iso_week:02d}.csv"


def data_files():
    """Liste l'ancien journal puis les journaux hebdomadaires dans l'ordre."""
    files = []
    if LEGACY_DATA_FILE.exists():
        files.append(LEGACY_DATA_FILE)
    files.extend(sorted(BASE_DIR.glob(f"{DATA_FILE_PREFIX}_????-W??.csv")))
    return files


def ensure_csv_file(data_file=None):
    data_file = data_file or weekly_data_file()
    if not data_file.exists() or data_file.stat().st_size == 0:
        with data_file.open("w", newline="", encoding="utf-8") as handle:
            csv.writer(handle).writerow(HEADERS)
        print(f"[OK] Nouveau fichier CSV hebdomadaire : {data_file.name}", flush=True)
    return data_file


def parse_measurement(line):
    try:
        values = next(csv.reader([line], strict=True))
    except (csv.Error, StopIteration) as exc:
        raise ValueError("trame CSV illisible") from exc

    if len(values) != len(HEADERS):
        raise ValueError(f"{len(values)} colonnes reçues, {len(HEADERS)} attendues")

    try:
        datetime.strptime(f"{values[0]} {values[1]}", "%d/%m/%Y %H:%M:%S")
    except ValueError as exc:
        raise ValueError("date ou heure invalide") from exc

    for header, raw_value in zip(HEADERS[2:], values[2:]):
        try:
            value = float(raw_value)
        except ValueError as exc:
            raise ValueError(f"valeur non numérique pour {header}") from exc
        if not math.isfinite(value):
            raise ValueError(f"valeur non finie pour {header}")

    return dict(zip(HEADERS, values))


def append_measurement(parsed):
    with csv_lock:
        data_file = ensure_csv_file()
        with data_file.open("a", newline="", encoding="utf-8") as handle:
            csv.writer(handle).writerow(parsed[header] for header in HEADERS)


def check_and_backup():
    BACKUP_DIR.mkdir(exist_ok=True)
    now = datetime.now()
    data_file = weekly_data_file(now)
    do_backup = True

    if BACKUP_INFO_FILE.exists():
        try:
            last_backup = datetime.strptime(
                BACKUP_INFO_FILE.read_text(encoding="utf-8").strip(),
                "%Y-%m-%d %H:%M:%S",
            )
            do_backup = (now - last_backup).days >= 7
        except (OSError, ValueError):
            do_backup = True

    if do_backup and data_file.exists():
        destination = BACKUP_DIR / f"backup_{now:%Y-%m-%d}_{data_file.name}"
        shutil.copy2(data_file, destination)
        BACKUP_INFO_FILE.write_text(now.strftime("%Y-%m-%d %H:%M:%S"), encoding="utf-8")
        print(f"Sauvegarde automatique effectuée : {destination.name}", flush=True)


def load_latest_measurement():
    global latest_data, latest_received_at
    ensure_csv_file()
    last_valid = None
    latest_file = None
    with csv_lock:
        for data_file in data_files():
            with data_file.open("r", newline="", encoding="utf-8-sig") as handle:
                for row in csv.reader(handle):
                    if row == HEADERS:
                        continue
                    try:
                        last_valid = parse_measurement(",".join(row))
                        latest_file = data_file
                    except ValueError:
                        continue
    latest_data = last_valid or {}
    if latest_data and latest_file:
        latest_received_at = datetime.fromtimestamp(latest_file.stat().st_mtime)


def read_history(limit):
    history = deque(maxlen=limit)
    with csv_lock:
        for data_file in data_files():
            with data_file.open("r", newline="", encoding="utf-8-sig") as handle:
                for row in csv.reader(handle):
                    if row == HEADERS:
                        continue
                    try:
                        history.append(parse_measurement(",".join(row)))
                    except ValueError:
                        continue
    return list(history)


def serial_listener():
    global latest_data, latest_received_at

    while True:
        try:
            with serial.Serial(SERIAL_PORT, SERIAL_BAUD, timeout=1) as receiver:
                print(f"[OK] Connecté au récepteur LoRa sur {SERIAL_PORT}", flush=True)
                while True:
                    raw_line = receiver.readline()
                    if not raw_line:
                        continue
                    try:
                        line = raw_line.decode("utf-8", errors="strict").strip()
                        if not line:
                            continue
                        parsed = parse_measurement(line)
                    except (UnicodeDecodeError, ValueError) as exc:
                        print(f"[REJET] Trame LoRa invalide : {exc}", flush=True)
                        continue

                    append_measurement(parsed)
                    latest_data = parsed
                    latest_received_at = datetime.now()
                    check_and_backup()
                    print(f"Nouvelle donnée LoRa : {line}", flush=True)
        except (serial.SerialException, OSError) as exc:
            print(
                f"[ATTENTION] Port série {SERIAL_PORT} indisponible : {exc}. "
                "Nouvelle tentative dans 5 s...",
                flush=True,
            )
            time.sleep(5)


ensure_csv_file()
load_latest_measurement()


@app.get("/")
def dashboard():
    return render_template("dashboard.html")


@app.get("/api/latest")
def api_latest():
    payload = dict(latest_data)
    if latest_received_at:
        payload["_received_at"] = latest_received_at.isoformat(timespec="seconds")
        payload["_age_seconds"] = max(0, int((datetime.now() - latest_received_at).total_seconds()))
    return jsonify(payload)


@app.get("/api/history")
def api_history():
    try:
        requested_limit = int(request.args.get("limit", DEFAULT_HISTORY_LIMIT))
    except ValueError:
        return jsonify(error="paramètre limit invalide"), 400
    limit = min(max(requested_limit, 1), MAX_HISTORY_LIMIT)
    return jsonify(read_history(limit))


@app.get("/api/health")
def api_health():
    age = None
    if latest_received_at:
        age = max(0, int((datetime.now() - latest_received_at).total_seconds()))
    return jsonify(
        status="ok",
        serial_port=SERIAL_PORT,
        has_data=bool(latest_data),
        age_seconds=age,
    )


if __name__ == "__main__":
    print(f"Démarrage du serveur EASE LoRa ({SERIAL_PORT}, {SERIAL_BAUD} bauds)...", flush=True)
    thread_serial = threading.Thread(target=serial_listener, daemon=True, name="lora-serial")
    thread_serial.start()
    dashboard_url = f"http://127.0.0.1:{PORT}"
    print(f"[OK] Serveur Web prêt : {dashboard_url}", flush=True)
    print("[INFO] Laissez cette fenêtre ouverte pendant l'utilisation.", flush=True)

    if os.getenv("EASE_OPEN_BROWSER", "0") == "1":
        threading.Timer(1.0, webbrowser.open, args=(dashboard_url,)).start()

    try:
        serve(app, host=HOST, port=PORT, threads=8)
    except OSError as exc:
        print(f"[ERREUR] Impossible d'ouvrir le port HTTP {PORT} : {exc}", flush=True)
        print("Fermez tout autre serveur EASE déjà lancé, puis réessayez.", flush=True)
        raise SystemExit(1) from exc
