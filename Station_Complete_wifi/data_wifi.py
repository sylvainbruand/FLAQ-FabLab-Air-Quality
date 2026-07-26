# -*- coding: utf-8 -*-
"""Serveur de collecte Wi-Fi de la station EASE."""

import csv
import hmac
import math
import os
import shutil
import threading
from collections import deque
from datetime import datetime
from pathlib import Path

from flask import Flask, jsonify, render_template, request
from waitress import serve


BASE_DIR = Path(__file__).resolve().parent
DATA_FILE_PREFIX = "all_sensors_log"
LEGACY_DATA_FILE = BASE_DIR / f"{DATA_FILE_PREFIX}.csv"
BACKUP_DIR = BASE_DIR / "backups"
BACKUP_INFO_FILE = BACKUP_DIR / "last_backup.txt"

HEADERS = [
    "Date", "Heure", "Temp_BME(C)", "Hum_BME(%)", "Pression(hPa)", "VOC_BME(kOhms)",
    "CO2_SCD30(ppm)", "Temp_SCD30(C)", "Hum_SCD30(%)", "PM1.0", "PM2.5", "PM10",
    "CO2_MHZ16(ppm)", "NO2", "EtOH", "VOC_Multi", "CO", "VOC_SGP40",
    "Temp_DHT20(C)", "Hum_DHT20(%)", "TVOC_SGP30(ppb)", "eCO2_SGP30(ppm)", "HCHO(ppm)",
]

MAX_BODY_BYTES = 4096
DEFAULT_HISTORY_LIMIT = 20_000
MAX_HISTORY_LIMIT = 50_000
INGEST_TOKEN = os.getenv("EASE_INGEST_TOKEN", "").strip()
ALLOW_UNAUTHENTICATED = os.getenv("EASE_ALLOW_UNAUTHENTICATED", "0") == "1"
HOST = os.getenv("EASE_HOST", "0.0.0.0")
PORT = int(os.getenv("EASE_PORT", "5000"))

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
    """Crée le journal et son en-tête sans dépendre du dossier de lancement."""
    data_file = data_file or weekly_data_file()
    if not data_file.exists() or data_file.stat().st_size == 0:
        with data_file.open("w", newline="", encoding="utf-8") as handle:
            csv.writer(handle).writerow(HEADERS)
        print(f"[OK] Nouveau fichier CSV hebdomadaire : {data_file.name}", flush=True)
    return data_file


def parse_measurement(line):
    """Valide une trame CSV et retourne le dictionnaire associé."""
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


def append_measurement(values):
    with csv_lock:
        data_file = ensure_csv_file()
        with data_file.open("a", newline="", encoding="utf-8") as handle:
            csv.writer(handle).writerow(values)


def check_and_backup():
    """Effectue au maximum une sauvegarde tous les sept jours."""
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


def authorized_ingest():
    if INGEST_TOKEN:
        supplied = request.headers.get("X-EASE-Token", "")
        return hmac.compare_digest(supplied, INGEST_TOKEN)
    return ALLOW_UNAUTHENTICATED


ensure_csv_file()
load_latest_measurement()


@app.post("/data")
def receive_data():
    global latest_data, latest_received_at

    if not authorized_ingest():
        if not INGEST_TOKEN:
            return jsonify(error="collecte non configurée : EASE_INGEST_TOKEN absent"), 503
        return jsonify(error="jeton de collecte invalide"), 401
    if request.content_length is not None and request.content_length > MAX_BODY_BYTES:
        return jsonify(error="trame trop volumineuse"), 413

    line = request.get_data(cache=False, as_text=True).strip()
    if not line or len(line.encode("utf-8")) > MAX_BODY_BYTES:
        return jsonify(error="trame vide ou trop volumineuse"), 400

    try:
        parsed = parse_measurement(line)
    except ValueError as exc:
        print(f"[REJET] {exc} : {line[:160]}", flush=True)
        return jsonify(error=str(exc)), 400

    append_measurement([parsed[header] for header in HEADERS])
    latest_data = parsed
    latest_received_at = datetime.now()
    check_and_backup()
    print(f"Nouvelle donnée reçue : {line}", flush=True)
    return jsonify(status="ok"), 200


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
        has_data=bool(latest_data),
        age_seconds=age,
        ingest_protected=bool(INGEST_TOKEN),
    )


if __name__ == "__main__":
    if INGEST_TOKEN:
        print("Authentification de collecte activée.", flush=True)
    elif ALLOW_UNAUTHENTICATED:
        print("[AVERTISSEMENT] Collecte non authentifiée explicitement autorisée.", flush=True)
    else:
        raise SystemExit(
            "Configuration refusée : définissez EASE_INGEST_TOKEN, "
            "ou EASE_ALLOW_UNAUTHENTICATED=1 pour un test local temporaire."
        )
    print(f"Serveur EASE Wi-Fi sur http://{HOST}:{PORT}", flush=True)
    serve(app, host=HOST, port=PORT, threads=8)
