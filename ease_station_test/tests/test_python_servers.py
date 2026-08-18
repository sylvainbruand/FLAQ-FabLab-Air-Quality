import importlib.util
import re
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch


ROOT = Path(__file__).resolve().parents[1]


def load_module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


WIFI = load_module("ease_data_wifi", ROOT / "Station_Complete_wifi" / "data_wifi.py")
LORA = load_module("ease_data_lora", ROOT / "Station_complete_lora" / "data_lora.py")

VALID_LINE = (
    "25/07/2026,18:00:00,22.10,45.20,1013.25,95.00,650,22.20,45.00,"
    "3,5,8,640,120.50,200.20,180.10,90.30,100,22.00,46.00,15,430,0.01"
)


class ParserTests(unittest.TestCase):
    def test_wifi_local_tokens_match(self):
        arduino_secret = (ROOT / "Station_Complete_wifi" / "arduino_secrets.h").read_text(
            encoding="utf-8"
        )
        server_secret = (ROOT / "Station_Complete_wifi" / "server_secrets.bat").read_text(
            encoding="utf-8"
        )
        arduino_token = re.search(r'EASE_INGEST_TOKEN\s+"([^"]+)"', arduino_secret).group(1)
        server_token = re.search(r'EASE_INGEST_TOKEN=([^"\r\n]+)', server_secret).group(1)
        self.assertGreaterEqual(len(arduino_token), 32)
        self.assertEqual(arduino_token, server_token)

    def test_both_parsers_accept_the_23_column_schema(self):
        for module in (WIFI, LORA):
            with self.subTest(module=module.__name__):
                parsed = module.parse_measurement(VALID_LINE)
                self.assertEqual(len(parsed), 23)
                self.assertEqual(parsed["PM2.5"], "5")

    def test_both_parsers_reject_bad_frames(self):
        for module in (WIFI, LORA):
            with self.subTest(module=module.__name__):
                with self.assertRaises(ValueError):
                    module.parse_measurement("25/07/2026,18:00:00,1,2")
                with self.assertRaises(ValueError):
                    module.parse_measurement(VALID_LINE.replace("22.10", "nan"))
                with self.assertRaises(ValueError):
                    module.parse_measurement(VALID_LINE.replace("25/07/2026", "99/99/2026"))


class WifiApiTests(unittest.TestCase):
    def setUp(self):
        self.temp_dir = tempfile.TemporaryDirectory()
        self.data_file = Path(self.temp_dir.name) / "measurements.csv"
        self.backup_dir = Path(self.temp_dir.name) / "backups"
        self.patchers = [
            patch.object(WIFI, "DATA_FILE", self.data_file),
            patch.object(WIFI, "BACKUP_DIR", self.backup_dir),
            patch.object(WIFI, "BACKUP_INFO_FILE", self.backup_dir / "last_backup.txt"),
            patch.object(WIFI, "INGEST_TOKEN", ""),
            patch.object(WIFI, "ALLOW_UNAUTHENTICATED", True),
        ]
        for patcher in self.patchers:
            patcher.start()
        WIFI.ensure_csv_file()
        WIFI.latest_data = {}
        WIFI.latest_received_at = None
        self.client = WIFI.app.test_client()

    def tearDown(self):
        for patcher in reversed(self.patchers):
            patcher.stop()
        self.temp_dir.cleanup()

    def test_valid_frame_is_persisted_and_exposed(self):
        response = self.client.post("/data", data=VALID_LINE, content_type="text/plain")
        self.assertEqual(response.status_code, 200)
        latest = self.client.get("/api/latest").get_json()
        self.assertEqual(latest["CO2_SCD30(ppm)"], "650")
        self.assertLessEqual(latest["_age_seconds"], 1)
        history = self.client.get("/api/history?limit=10").get_json()
        self.assertEqual(len(history), 1)

    def test_invalid_frame_is_rejected_without_persistence(self):
        response = self.client.post("/data", data="invalid", content_type="text/plain")
        self.assertEqual(response.status_code, 400)
        self.assertEqual(self.client.get("/api/history").get_json(), [])

    def test_token_can_protect_ingestion(self):
        with patch.object(WIFI, "INGEST_TOKEN", "secret"):
            self.assertEqual(self.client.post("/data", data=VALID_LINE).status_code, 401)
            response = self.client.post(
                "/data",
                data=VALID_LINE,
                headers={"X-EASE-Token": "secret"},
            )
            self.assertEqual(response.status_code, 200)

    def test_missing_security_configuration_is_closed_by_default(self):
        with (
            patch.object(WIFI, "INGEST_TOKEN", ""),
            patch.object(WIFI, "ALLOW_UNAUTHENTICATED", False),
        ):
            self.assertEqual(self.client.post("/data", data=VALID_LINE).status_code, 503)

    def test_history_limit_is_validated_and_bounded(self):
        self.assertEqual(self.client.get("/api/history?limit=nope").status_code, 400)
        self.assertEqual(self.client.get("/api/history?limit=0").status_code, 200)


if __name__ == "__main__":
    unittest.main()
