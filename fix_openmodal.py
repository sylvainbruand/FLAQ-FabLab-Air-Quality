import os
import re

cards_html = """
        <div class="dashboard-grid">
            <div class="card" id="card-temp-bme" onclick="openModal('temp', 'Temp_BME(C)', '°C', event)">
                <div class="card-header">
                    <div class="card-title" data-i18n="temp">Température</div>
                    <div class="card-header-actions">
                        <span class="status-badge" id="badge-temp-bme">--</span>
                        <div class="card-help-btn" onclick="openInfoModal(event, 'temp')" title="Seuils de référence">?</div>
                    </div>
                </div>
                <div class="card-value"><span id="val-temp-bme">--</span><span class="card-unit">°C</span></div>
                <div class="card-source"><span data-i18n="sensor">Capteur:</span> BME680</div>
            </div>
            
            <div class="card" id="card-temp-dht" onclick="openModal('temp_alt', 'Temp_DHT20(C)', '°C', event)">
                <div class="card-header">
                    <div class="card-title" data-i18n="temp_alt">Température (Alt)</div>
                    <div class="card-header-actions">
                        <span class="status-badge" id="badge-temp-dht">--</span>
                        <div class="card-help-btn" onclick="openInfoModal(event, 'temp')" title="Seuils de référence">?</div>
                    </div>
                </div>
                <div class="card-value"><span id="val-temp-dht">--</span><span class="card-unit">°C</span></div>
                <div class="card-source"><span data-i18n="sensor">Capteur:</span> DHT20</div>
            </div>

            <div class="card" id="card-temp-scd" onclick="openModal('temp_scd', 'Temp_SCD30(C)', '°C', event)">
                <div class="card-header">
                    <div class="card-title" data-i18n="temp_scd">Température (SCD)</div>
                    <div class="card-header-actions">
                        <span class="status-badge" id="badge-temp-scd">--</span>
                        <div class="card-help-btn" onclick="openInfoModal(event, 'temp')" title="Seuils de référence">?</div>
                    </div>
                </div>
                <div class="card-value"><span id="val-temp-scd">--</span><span class="card-unit">°C</span></div>
                <div class="card-source"><span data-i18n="sensor">Capteur:</span> SCD30</div>
            </div>

            <div class="card" id="card-hum-bme" onclick="openModal('hum', 'Hum_BME(%)', '%', event)">
                <div class="card-header">
                    <div class="card-title" data-i18n="hum">Humidité</div>
                    <div class="card-header-actions">
                        <span class="status-badge" id="badge-hum-bme">--</span>
                        <div class="card-help-btn" onclick="openInfoModal(event, 'hum')" title="Seuils de référence">?</div>
                    </div>
                </div>
                <div class="card-value"><span id="val-hum-bme">--</span><span class="card-unit">%</span></div>
                <div class="card-source"><span data-i18n="sensor">Capteur:</span> BME680</div>
            </div>

            <div class="card" id="card-hum-dht" onclick="openModal('hum_alt', 'Hum_DHT20(%)', '%', event)">
                <div class="card-header">
                    <div class="card-title" data-i18n="hum_alt">Humidité (Alt)</div>
                    <div class="card-header-actions">
                        <span class="status-badge" id="badge-hum-dht">--</span>
                        <div class="card-help-btn" onclick="openInfoModal(event, 'hum')" title="Seuils de référence">?</div>
                    </div>
                </div>
                <div class="card-value"><span id="val-hum-dht">--</span><span class="card-unit">%</span></div>
                <div class="card-source"><span data-i18n="sensor">Capteur:</span> DHT20</div>
            </div>

            <div class="card" id="card-hum-scd" onclick="openModal('hum_scd', 'Hum_SCD30(%)', '%', event)">
                <div class="card-header">
                    <div class="card-title" data-i18n="hum_scd">Humidité (SCD)</div>
                    <div class="card-header-actions">
                        <span class="status-badge" id="badge-hum-scd">--</span>
                        <div class="card-help-btn" onclick="openInfoModal(event, 'hum')" title="Seuils de référence">?</div>
                    </div>
                </div>
                <div class="card-value"><span id="val-hum-scd">--</span><span class="card-unit">%</span></div>
                <div class="card-source"><span data-i18n="sensor">Capteur:</span> SCD30</div>
            </div>

            <div class="card" id="card-pres" onclick="openModal('pres', 'Pression(hPa)', 'hPa', event)">
                <div class="card-header">
                    <div class="card-title" data-i18n="pres">Pression Atmo.</div>
                    <div class="card-header-actions">
                        <span class="status-badge" id="badge-pres">--</span>
                        <div class="card-help-btn" onclick="openInfoModal(event, 'pres')" title="Seuils de référence">?</div>
                    </div>
                </div>
                <div class="card-value"><span id="val-pres">--</span><span class="card-unit">hPa</span></div>
                <div class="card-source"><span data-i18n="sensor">Capteur:</span> BME680</div>
            </div>

            <div class="card" id="card-co2-scd" onclick="openModal('co2', 'CO2_SCD30(ppm)', 'ppm', event)">
                <div class="card-header">
                    <div class="card-title" data-i18n="co2">Dioxyde de Carbone</div>
                    <div class="card-header-actions">
                        <span class="status-badge" id="badge-co2-scd">--</span>
                        <div class="card-help-btn" onclick="openInfoModal(event, 'co2')" title="Seuils de référence">?</div>
                    </div>
                </div>
                <div class="card-value"><span id="val-co2-scd">--</span><span class="card-unit">ppm</span></div>
                <div class="card-source"><span data-i18n="sensor">Capteur:</span> SCD30</div>
            </div>

            <div class="card" id="card-co2-mhz" onclick="openModal('co2', 'CO2_MHZ16(ppm)', 'ppm', event)">
                <div class="card-header">
                    <div class="card-title" data-i18n="co2">Dioxyde de Carbone</div>
                    <div class="card-header-actions">
                        <span class="status-badge" id="badge-co2-mhz">--</span>
                        <div class="card-help-btn" onclick="openInfoModal(event, 'co2')" title="Seuils de référence">?</div>
                    </div>
                </div>
                <div class="card-value"><span id="val-co2-mhz">--</span><span class="card-unit">ppm</span></div>
                <div class="card-source"><span data-i18n="sensor">Capteur:</span> MH-Z16</div>
            </div>

            <div class="card" id="card-pm10-fine" onclick="openModal('pm10_fine', 'PM1.0', 'µg/m³', event)">
                <div class="card-header">
                    <div class="card-title" data-i18n="pm10_fine">Particules PM1.0</div>
                    <div class="card-header-actions">
                        <span class="status-badge" id="badge-pm10-fine">--</span>
                        <div class="card-help-btn" onclick="openInfoModal(event, 'pm25')" title="Seuils de référence">?</div>
                    </div>
                </div>
                <div class="card-value"><span id="val-pm10-fine">--</span><span class="card-unit">µg/m³</span></div>
                <div class="card-source"><span data-i18n="sensor">Capteur:</span> HM3301</div>
            </div>

            <div class="card" id="card-pm25" onclick="openModal('pm25', 'PM2.5', 'µg/m³', event)">
                <div class="card-header">
                    <div class="card-title" data-i18n="pm25">Particules PM2.5</div>
                    <div class="card-header-actions">
                        <span class="status-badge" id="badge-pm25">--</span>
                        <div class="card-help-btn" onclick="openInfoModal(event, 'pm25')" title="Seuils de référence">?</div>
                    </div>
                </div>
                <div class="card-value"><span id="val-pm25">--</span><span class="card-unit">µg/m³</span></div>
                <div class="card-source"><span data-i18n="sensor">Capteur:</span> HM3301</div>
            </div>
            
            <div class="card" id="card-pm10" onclick="openModal('pm10', 'PM10', 'µg/m³', event)">
                <div class="card-header">
                    <div class="card-title" data-i18n="pm10">Particules PM10</div>
                    <div class="card-header-actions">
                        <span class="status-badge" id="badge-pm10">--</span>
                        <div class="card-help-btn" onclick="openInfoModal(event, 'pm10')" title="Seuils de référence">?</div>
                    </div>
                </div>
                <div class="card-value"><span id="val-pm10">--</span><span class="card-unit">µg/m³</span></div>
                <div class="card-source"><span data-i18n="sensor">Capteur:</span> HM3301</div>
            </div>

            <div class="card" id="card-voc-sgp" onclick="openModal('voc_idx', 'VOC_SGP40', 'Idx', event)">
                <div class="card-header">
                    <div class="card-title" data-i18n="voc_idx">Index VOC</div>
                    <div class="card-header-actions">
                        <span class="status-badge" id="badge-voc-sgp">--</span>
                        <div class="card-help-btn" onclick="openInfoModal(event, 'sgp40')" title="Seuils de référence">?</div>
                    </div>
                </div>
                <div class="card-value"><span id="val-voc-sgp">--</span><span class="card-unit">Idx</span></div>
                <div class="card-source"><span data-i18n="sensor">Capteur:</span> SGP40</div>
            </div>

            <div class="card" id="card-no2" onclick="openModal('no2', 'NO2', '', event)">
                <div class="card-header">
                    <div class="card-title" data-i18n="no2">Dioxyde d'Azote (NO2)</div>
                    <div class="card-header-actions">
                        <span class="status-badge" id="badge-no2">--</span>
                        <div class="card-help-btn" onclick="openInfoModal(event, 'no2')" title="Seuils de référence">?</div>
                    </div>
                </div>
                <div class="card-value"><span id="val-no2">--</span><span class="card-unit"></span></div>
                <div class="card-source"><span data-i18n="sensor">Capteur:</span> Multichannel V2</div>
            </div>
            
            <div class="card" id="card-etoh" onclick="openModal('etoh', 'EtOH', 'ppm', event)">
                <div class="card-header">
                    <div class="card-title" data-i18n="etoh">Éthanol (EtOH)</div>
                    <div class="card-header-actions">
                        <span class="status-badge" id="badge-etoh">--</span>
                        <div class="card-help-btn" onclick="openInfoModal(event, 'etoh')" title="Seuils de référence">?</div>
                    </div>
                </div>
                <div class="card-value"><span id="val-etoh">--</span><span class="card-unit">ppm</span></div>
                <div class="card-source"><span data-i18n="sensor">Capteur:</span> Multichannel V2</div>
            </div>

            <div class="card" id="card-voc-multi" onclick="openModal('voc_multi', 'VOC_Multi', 'ppm', event)">
                <div class="card-header">
                    <div class="card-title" data-i18n="voc_multi">VOC (Multi Gaz)</div>
                    <div class="card-header-actions">
                        <span class="status-badge" id="badge-voc-multi">--</span>
                        <div class="card-help-btn" onclick="openInfoModal(event, 'voc_multi')" title="Seuils de référence">?</div>
                    </div>
                </div>
                <div class="card-value"><span id="val-voc-multi">--</span><span class="card-unit">ppm</span></div>
                <div class="card-source"><span data-i18n="sensor">Capteur:</span> Multichannel V2</div>
            </div>
            
            <div class="card" id="card-co" onclick="openModal('co', 'CO', 'ppm', event)">
                <div class="card-header">
                    <div class="card-title" data-i18n="co">Monoxyde de Carbone</div>
                    <div class="card-header-actions">
                        <span class="status-badge" id="badge-co">--</span>
                        <div class="card-help-btn" onclick="openInfoModal(event, 'co')" title="Seuils de référence">?</div>
                    </div>
                </div>
                <div class="card-value"><span id="val-co">--</span><span class="card-unit">ppm</span></div>
                <div class="card-source"><span data-i18n="sensor">Capteur:</span> Multichannel Gas</div>
            </div>

            <div class="card" id="card-tvoc-sgp30" onclick="openModal('tvoc', 'TVOC_SGP30(ppb)', 'ppb', event)">
                <div class="card-header">
                    <div class="card-title" data-i18n="tvoc">TVOC (Composés Volatils)</div>
                    <div class="card-header-actions">
                        <span class="status-badge" id="badge-tvoc-sgp30">--</span>
                        <div class="card-help-btn" onclick="openInfoModal(event, 'tvoc')" title="Seuils de référence">?</div>
                    </div>
                </div>
                <div class="card-value"><span id="val-tvoc-sgp30">--</span><span class="card-unit">ppb</span></div>
                <div class="card-source"><span data-i18n="sensor">Capteur:</span> SGP30</div>
            </div>

            <div class="card" id="card-eco2-sgp30" onclick="openModal('eco2', 'eCO2_SGP30(ppm)', 'ppm', event)">
                <div class="card-header">
                    <div class="card-title" data-i18n="eco2">Equivalent CO2 (eCO2)</div>
                    <div class="card-header-actions">
                        <span class="status-badge" id="badge-eco2-sgp30">--</span>
                        <div class="card-help-btn" onclick="openInfoModal(event, 'co2')" title="Seuils de référence">?</div>
                    </div>
                </div>
                <div class="card-value"><span id="val-eco2-sgp30">--</span><span class="card-unit">ppm</span></div>
                <div class="card-source"><span data-i18n="sensor">Capteur:</span> SGP30</div>
            </div>

            <div class="card" id="card-hcho" onclick="openModal('hcho', 'HCHO(ppm)', 'ppm', event)">
                <div class="card-header">
                    <div class="card-title" data-i18n="hcho">Formaldéhyde (HCHO)</div>
                    <div class="card-header-actions">
                        <span class="status-badge" id="badge-hcho">--</span>
                        <div class="card-help-btn" onclick="openInfoModal(event, 'hcho')" title="Seuils de référence">?</div>
                    </div>
                </div>
                <div class="card-value"><span id="val-hcho">--</span><span class="card-unit">ppm</span></div>
                <div class="card-source"><span data-i18n="sensor">Capteur:</span> WSP2110</div>
            </div>
        </div>
"""

def fix_file(filepath):
    print(f"Fixing openModal and cards in {filepath}...")
    with open(filepath, "r", encoding="utf-8") as f:
        content = f.read()

    # 1. Update openModal signature to check event target
    content = content.replace(
        "async function openModal(i18nKey, dataKey, unit) {",
        "async function openModal(i18nKey, dataKey, unit, event) {\n            if (event && (event.target.classList.contains('card-help-btn') || event.target.closest('.card-help-btn'))) return;"
    )

    # 2. Update dashboard grid cards
    start_grid = content.find('<div class="dashboard-grid">')
    end_grid = content.find('</div> <!-- END TAB 1 -->')
    if start_grid != -1 and end_grid != -1:
        content = content[:start_grid] + cards_html.strip() + "\n    " + content[end_grid:]

    with open(filepath, "w", encoding="utf-8") as f:
        f.write(content)
    print(f"[OK] {filepath} fixed.")

files_to_fix = [
    r"c:\Users\sylva\Documents\! projet climat\Station_Complete_wifi\templates\dashboard.html",
    r"c:\Users\sylva\Documents\! projet climat\station_complete_lora\templates\dashboard.html",
    r"c:\Users\sylva\Documents\! projet climat\station_complete_lora_no_python\sd_card_files\dashboard.html",
    r"c:\Users\sylva\Documents\! projet climat\station_complete_lora_no_python\sd_card_files\dash.htm"
]

for f in files_to_fix:
    if os.path.exists(f):
        fix_file(f)
