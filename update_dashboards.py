import os

target_file = r"c:\Users\sylva\Documents\! projet climat\Station_Complete_wifi\templates\dashboard.html"

with open(target_file, "r", encoding="utf-8") as f:
    content = f.read()

css_to_add = """
        /* --- HELP BUTTON & THRESHOLD STYLES --- */
        .card-header-actions { display: flex; align-items: center; gap: 0.5rem; }
        .card-help-btn { 
            background: rgba(255, 255, 255, 0.1); 
            border: 1px solid var(--card-border); 
            color: var(--accent-blue); 
            width: 22px; height: 22px; 
            border-radius: 50%; 
            display: flex; align-items: center; justify-content: center; 
            font-size: 0.8rem; font-weight: bold; cursor: pointer; 
            transition: all 0.2s ease; z-index: 10;
        }
        .card-help-btn:hover { 
            background: var(--accent-blue); 
            color: #000; 
            transform: scale(1.15); 
            box-shadow: 0 0 10px var(--accent-blue);
        }

        .status-badge {
            font-size: 0.65rem; padding: 2px 7px; border-radius: 10px; font-weight: 700; text-transform: uppercase; letter-spacing: 0.5px;
        }
        .status-good { background: rgba(16, 185, 129, 0.2); color: #34d399; border: 1px solid rgba(16, 185, 129, 0.4); }
        .status-acceptable { background: rgba(245, 158, 11, 0.2); color: #fbbf24; border: 1px solid rgba(245, 158, 11, 0.4); }
        .status-warning { background: rgba(234, 88, 12, 0.25); color: #fb923c; border: 1px solid rgba(234, 88, 12, 0.5); }
        .status-alert { background: rgba(239, 68, 68, 0.25); color: #f87171; border: 1px solid rgba(239, 68, 68, 0.5); }
        .status-danger { background: rgba(153, 27, 27, 0.35); color: #fca5a5; border: 1px solid rgba(153, 27, 27, 0.6); }

        /* Dynamic Card Threshold States */
        .card.card-thresh-good::before { background: #10b981 !important; opacity: 1 !important; }
        .card.card-thresh-good { box-shadow: 0 4px 20px rgba(16, 185, 129, 0.2); border-color: rgba(16, 185, 129, 0.35); }
        
        .card.card-thresh-acceptable::before { background: #f59e0b !important; opacity: 1 !important; }
        .card.card-thresh-acceptable { box-shadow: 0 4px 20px rgba(245, 158, 11, 0.2); border-color: rgba(245, 158, 11, 0.35); }

        .card.card-thresh-warning::before { background: #ea580c !important; opacity: 1 !important; }
        .card.card-thresh-warning { box-shadow: 0 4px 20px rgba(234, 88, 12, 0.25); border-color: rgba(234, 88, 12, 0.45); }

        .card.card-thresh-alert::before { background: #ef4444 !important; opacity: 1 !important; }
        .card.card-thresh-alert { box-shadow: 0 4px 25px rgba(239, 68, 68, 0.3); border-color: rgba(239, 68, 68, 0.5); }

        .card.card-thresh-danger::before { background: #991b1b !important; opacity: 1 !important; }
        .card.card-thresh-danger { box-shadow: 0 4px 25px rgba(153, 27, 27, 0.35); border-color: rgba(153, 27, 27, 0.6); }

        /* Threshold Reference Modal Styling */
        .thresh-table { width: 100%; border-collapse: collapse; margin-top: 1rem; font-size: 0.9rem; }
        .thresh-table td, .thresh-table th { padding: 0.7rem 0.8rem; border-bottom: 1px solid rgba(255,255,255,0.08); text-align: left; }
        .thresh-pill { display: inline-block; width: 12px; height: 12px; border-radius: 3px; margin-right: 8px; vertical-align: middle; }
"""

info_modal_html = """
    <!-- Info Threshold Modal -->
    <div class="modal-overlay" id="info-modal" onclick="if(event.target === this) closeInfoModal()">
        <div class="modal-content" style="max-width: 650px;">
            <span class="close-btn" onclick="closeInfoModal()">&times;</span>
            <h2 class="modal-title" id="info-modal-title">Seuils de Référence</h2>
            <div id="info-modal-body" style="color: var(--text-muted); line-height: 1.5; font-size: 0.95rem;">
                <!-- Dynamically populated -->
            </div>
        </div>
    </div>
"""

cards_html = """
        <div class="dashboard-grid">
            <div class="card" id="card-temp-bme" onclick="openModal('temp', 'Temp_BME(C)', '°C')">
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
            
            <div class="card" id="card-temp-dht" onclick="openModal('temp_alt', 'Temp_DHT20(C)', '°C')">
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

            <div class="card" id="card-temp-scd" onclick="openModal('temp_scd', 'Temp_SCD30(C)', '°C')">
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

            <div class="card" id="card-hum-bme" onclick="openModal('hum', 'Hum_BME(%)', '%')">
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

            <div class="card" id="card-hum-dht" onclick="openModal('hum_alt', 'Hum_DHT20(%)', '%')">
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

            <div class="card" id="card-hum-scd" onclick="openModal('hum_scd', 'Hum_SCD30(%)', '%')">
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

            <div class="card" id="card-pres" onclick="openModal('pres', 'Pression(hPa)', 'hPa')">
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

            <div class="card" id="card-co2-scd" onclick="openModal('co2', 'CO2_SCD30(ppm)', 'ppm')">
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

            <div class="card" id="card-co2-mhz" onclick="openModal('co2', 'CO2_MHZ16(ppm)', 'ppm')">
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

            <div class="card" id="card-pm10-fine" onclick="openModal('pm10_fine', 'PM1.0', 'µg/m³')">
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

            <div class="card" id="card-pm25" onclick="openModal('pm25', 'PM2.5', 'µg/m³')">
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
            
            <div class="card" id="card-pm10" onclick="openModal('pm10', 'PM10', 'µg/m³')">
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

            <div class="card" id="card-voc-sgp" onclick="openModal('voc_idx', 'VOC_SGP40', 'Idx')">
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

            <div class="card" id="card-no2" onclick="openModal('no2', 'NO2', '')">
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
            
            <div class="card" id="card-etoh" onclick="openModal('etoh', 'EtOH', 'ppm')">
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

            <div class="card" id="card-voc-multi" onclick="openModal('voc_multi', 'VOC_Multi', 'ppm')">
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
            
            <div class="card" id="card-co" onclick="openModal('co', 'CO', 'ppm')">
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

            <div class="card" id="card-tvoc-sgp30" onclick="openModal('tvoc', 'TVOC_SGP30(ppb)', 'ppb')">
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

            <div class="card" id="card-eco2-sgp30" onclick="openModal('eco2', 'eCO2_SGP30(ppm)', 'ppm')">
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

            <div class="card" id="card-hcho" onclick="openModal('hcho', 'HCHO(ppm)', 'ppm')">
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

js_threshold_logic = """
        // --- THRESHOLD CONFIGURATIONS & EVALUATION ---
        const thresholdConfigs = {
            'co2': {
                title: 'CO₂ — Dioxyde de carbone',
                subtitle: 'Traceur de confinement intérieur (SCD30 / MH-Z16)',
                objective: 'Révèle la qualité de la ventilation. Un taux élevé de CO₂ signale l\'accumulation d\'autres polluants (virus, allergènes, COV) faute de renouvellement d\'air suffisant.',
                levels: [
                    { color: '#10b981', label: 'Air extérieur (référence)', detail: '420 ppm', statusClass: 'good' },
                    { color: '#10b981', label: 'Bonne qualité intérieure', detail: '< 800 ppm', statusClass: 'good' },
                    { color: '#f59e0b', label: 'Acceptable', detail: '800 – 1000 ppm', statusClass: 'acceptable' },
                    { color: '#ea580c', label: 'Seuil de gestion HCSP / ASHRAE', detail: '1000 ppm', statusClass: 'warning' },
                    { color: '#ea580c', label: 'Dégradé — aérer impérativement', detail: '1000 – 1500 ppm', statusClass: 'warning' },
                    { color: '#ef4444', label: 'Seuil d\'alerte HCSP (ERP) / Symptômes', detail: '> 1500 – 1700 ppm', statusClass: 'alert' },
                    { color: '#991b1b', label: 'Limite expo professionnelle', detail: '≥ 5000 ppm (INRS 8h)', statusClass: 'danger' }
                ],
                eval: (val) => {
                    if (val < 800) return 'good';
                    if (val <= 1000) return 'acceptable';
                    if (val <= 1500) return 'warning';
                    if (val < 5000) return 'alert';
                    return 'danger';
                }
            },
            'pm25': {
                title: 'PM2.5 — Particules très fines',
                subtitle: 'Polluant prioritaire — mortalité prématurée (HM3301)',
                objective: 'Les PM2.5 atteignent les alvéoles pulmonaires puis le sang. Origines : combustion, trafic, chauffage.',
                levels: [
                    { color: '#10b981', label: 'OMS PM2.5 — Annuel', detail: '5 µg/m³', statusClass: 'good' },
                    { color: '#10b981', label: 'OMS PM2.5 — Journalier (24h)', detail: '15 µg/m³', statusClass: 'good' },
                    { color: '#f59e0b', label: 'UE PM2.5 — Annuel', detail: '25 µg/m³ (→10 en 2030)', statusClass: 'acceptable' },
                    { color: '#ea580c', label: 'Seuil info FR PM2.5', detail: '50 µg/m³ (24h)', statusClass: 'warning' },
                    { color: '#ef4444', label: 'Seuil alerte FR PM2.5', detail: '75 µg/m³ (24h)', statusClass: 'alert' }
                ],
                eval: (val) => {
                    if (val <= 15) return 'good';
                    if (val <= 25) return 'acceptable';
                    if (val <= 50) return 'warning';
                    return 'alert';
                }
            },
            'pm10': {
                title: 'PM10 — Particules fines',
                subtitle: 'Polluant respiratoire — s\'arrêtent aux bronches (HM3301)',
                objective: 'Particules fines en suspension. Origines : poussières, chantiers, trafic.',
                levels: [
                    { color: '#10b981', label: 'OMS PM10 — Journalier', detail: '45 µg/m³', statusClass: 'good' },
                    { color: '#f59e0b', label: 'UE PM10 — Journalier', detail: '50 µg/m³', statusClass: 'acceptable' },
                    { color: '#ef4444', label: 'Seuil alerte FR PM10', detail: '> 50 µg/m³ (24h)', statusClass: 'alert' }
                ],
                eval: (val) => {
                    if (val <= 45) return 'good';
                    if (val <= 50) return 'acceptable';
                    return 'alert';
                }
            },
            'no2': {
                title: 'NO₂ — Dioxyde d\'azote',
                subtitle: 'Pollution trafic et combustion (diesel, gaz) (Multichannel V2)',
                objective: 'Marqueur du trafic routier (diesel) et des chaudières gaz. Indicateur clé pour l\'exposition scolaire.',
                levels: [
                    { color: '#10b981', label: 'OMS 2021 — Annuel', detail: '10 µg/m³', statusClass: 'good' },
                    { color: '#10b981', label: 'OMS 2021 — Journalier (24h)', detail: '25 µg/m³', statusClass: 'good' },
                    { color: '#f59e0b', label: 'UE/France — Annuel', detail: '40 µg/m³', statusClass: 'acceptable' },
                    { color: '#ea580c', label: 'UE/France — Horaire (≤18h/an)', detail: '200 µg/m³', statusClass: 'warning' },
                    { color: '#ea580c', label: 'Seuil information France', detail: '200 µg/m³ (1h)', statusClass: 'warning' },
                    { color: '#ef4444', label: 'Seuil alerte France', detail: '400 µg/m³ (1h)', statusClass: 'alert' }
                ],
                eval: (val) => {
                    if (val <= 25) return 'good';
                    if (val <= 40) return 'acceptable';
                    if (val <= 200) return 'warning';
                    return 'alert';
                }
            },
            'hcho': {
                title: 'COV — Formaldéhyde (HCHO)',
                subtitle: 'Polluants intérieurs — formaldéhyde cible en milieu scolaire (WSP2110)',
                objective: 'Émis par mobilier, peintures, parfums, colles. Cancérogène avéré CIRC Groupe 1.',
                levels: [
                    { color: '#10b981', label: 'OMS — Formaldéhyde long terme', detail: '100 µg/m³', statusClass: 'good' },
                    { color: '#10b981', label: 'ANSES VTR — Long terme', detail: '10 µg/m³ (~0.008 ppm)', statusClass: 'good' },
                    { color: '#f59e0b', label: 'France ERP (depuis 2023)', detail: '30 µg/m³ (~0.024 ppm)', statusClass: 'acceptable' },
                    { color: '#ea580c', label: 'France ERP — Cible long terme', detail: '10 µg/m³ (~0.008 ppm)', statusClass: 'warning' }
                ],
                eval: (val) => {
                    if (val <= 0.008) return 'good';
                    if (val <= 0.024) return 'acceptable';
                    return 'warning';
                }
            },
            'sgp40': {
                title: 'Index VOC (COV) — SGP40',
                subtitle: 'Score dynamique d\'évolution des polluants intérieurs (SGP40)',
                objective: 'Analyse continue des composés organiques volatils par rapport à la moyenne des 24h.',
                levels: [
                    { color: '#10b981', label: 'Air Bon / Normal', detail: '< 120 Idx (100 = Référence)', statusClass: 'good' },
                    { color: '#f59e0b', label: 'Pollution modérée', detail: '120 – 220 Idx', statusClass: 'warning' },
                    { color: '#ef4444', label: 'Pollution élevée', detail: '> 220 Idx', statusClass: 'alert' }
                ],
                eval: (val) => {
                    if (val <= 120) return 'good';
                    if (val <= 220) return 'warning';
                    return 'alert';
                }
            },
            'tvoc': {
                title: 'TVOC (Composés Volatils) — SGP30',
                subtitle: 'Concentration totale estimée des COV (SGP30)',
                objective: 'Mesure globale des molécules organiques volatiles dans l\'air intérieur.',
                levels: [
                    { color: '#10b981', label: 'Air Pur', detail: '< 300 ppb', statusClass: 'good' },
                    { color: '#f59e0b', label: 'Pollution modérée', detail: '300 – 1000 ppb', statusClass: 'warning' },
                    { color: '#ef4444', label: 'Pollution élevée', detail: '> 1000 ppb', statusClass: 'alert' }
                ],
                eval: (val) => {
                    if (val <= 300) return 'good';
                    if (val <= 1000) return 'warning';
                    return 'alert';
                }
            },
            'temp': {
                title: 'Température Ambiante',
                subtitle: 'Confort thermique de l\'air',
                objective: 'Maintien de la température optimale d\'occupation.',
                levels: [
                    { color: '#f59e0b', label: 'Frais', detail: '< 18 °C', statusClass: 'acceptable' },
                    { color: '#10b981', label: 'Confort Idéal', detail: '18 – 24 °C', statusClass: 'good' },
                    { color: '#f59e0b', label: 'Chaud', detail: '24 – 28 °C', statusClass: 'acceptable' },
                    { color: '#ef4444', label: 'Très Chaud', detail: '> 28 °C', statusClass: 'alert' }
                ],
                eval: (val) => {
                    if (val >= 18 && val <= 24) return 'good';
                    if (val >= 15 && val <= 28) return 'acceptable';
                    return 'alert';
                }
            },
            'hum': {
                title: 'Humidité Relative',
                subtitle: 'Taux d\'hygrométrie de l\'air',
                objective: 'Prévention de la sécheresse des muqueuses et du développement fongique.',
                levels: [
                    { color: '#f59e0b', label: 'Air Sec', detail: '< 40 %', statusClass: 'acceptable' },
                    { color: '#10b981', label: 'Hygrométrie Idéale', detail: '40 – 60 %', statusClass: 'good' },
                    { color: '#f59e0b', label: 'Air Humide', detail: '> 60 %', statusClass: 'acceptable' }
                ],
                eval: (val) => {
                    if (val >= 40 && val <= 60) return 'good';
                    return 'acceptable';
                }
            },
            'pres': {
                title: 'Pression Atmosphérique',
                subtitle: 'Pression barométrique de l\'air (hPa)',
                objective: 'Météorologie et suivi des masses d\'air.',
                levels: [
                    { color: '#10b981', label: 'Pression Standard', detail: '1013.25 hPa (Pression moyenne)', statusClass: 'good' }
                ],
                eval: () => 'good'
            },
            'etoh': {
                title: 'Éthanol (EtOH)',
                subtitle: 'Canal Alcool — Multichannel Gas V2',
                objective: 'Détection des vapeurs d\'alcool, gel hydroalcoolique et désinfectants.',
                levels: [
                    { color: '#10b981', label: 'Air Pur', detail: 'Niveau de fond normal', statusClass: 'good' }
                ],
                eval: () => 'good'
            },
            'voc_multi': {
                title: 'VOC (Multi Gaz)',
                subtitle: 'Canal COV brut — Multichannel Gas V2',
                objective: 'Mesure brute de résistance des composés organiques.',
                levels: [
                    { color: '#10b981', label: 'Air Pur', detail: 'Niveau de fond normal', statusClass: 'good' }
                ],
                eval: () => 'good'
            },
            'co': {
                title: 'Monoxyde de Carbone (CO)',
                subtitle: 'Canal CO — Multichannel Gas V2',
                objective: 'Détection du gaz asphyxiant issu de combustions incomplètes.',
                levels: [
                    { color: '#10b981', label: 'Air Pur', detail: '< 5 ppm', statusClass: 'good' },
                    { color: '#ef4444', label: 'Seuil d\'alerte', detail: '> 20 ppm', statusClass: 'alert' }
                ],
                eval: (val) => val > 20 ? 'alert' : 'good'
            }
        };

        function openInfoModal(event, key) {
            event.stopPropagation();
            const cfg = thresholdConfigs[key];
            if (!cfg) return;

            document.getElementById('info-modal-title').innerText = cfg.title;
            let html = `<p style="color:var(--accent-blue); font-weight:600; margin-bottom:0.5rem;">${cfg.subtitle}</p>`;
            html += `<p style="margin-bottom:1.5rem;">${cfg.objective}</p>`;
            html += `<h4 style="color:#fff; margin-bottom:0.5rem;">⚠️ Seuils de Référence</h4>`;
            html += `<table class="thresh-table"><tbody>`;
            for (let lvl of cfg.levels) {
                html += `<tr>
                    <td><span class="thresh-pill" style="background:${lvl.color};"></span><b>${lvl.label}</b></td>
                    <td style="text-align:right; font-weight:600; color:var(--text-main);">${lvl.detail}</td>
                </tr>`;
            }
            html += `</tbody></table>`;

            document.getElementById('info-modal-body').innerHTML = html;
            document.getElementById('info-modal').classList.add('active');
        }

        function closeInfoModal() {
            document.getElementById('info-modal').classList.remove('active');
        }

        function updateCardThresholds(data) {
            const mappings = [
                { cardId: 'card-temp-bme', badgeId: 'badge-temp-bme', val: parseFloat(data['Temp_BME(C)']), cfgKey: 'temp' },
                { cardId: 'card-temp-dht', badgeId: 'badge-temp-dht', val: parseFloat(data['Temp_DHT20(C)']), cfgKey: 'temp' },
                { cardId: 'card-temp-scd', badgeId: 'badge-temp-scd', val: parseFloat(data['Temp_SCD30(C)']), cfgKey: 'temp' },
                { cardId: 'card-hum-bme', badgeId: 'badge-hum-bme', val: parseFloat(data['Hum_BME(%)']), cfgKey: 'hum' },
                { cardId: 'card-hum-dht', badgeId: 'badge-hum-dht', val: parseFloat(data['Hum_DHT20(%)']), cfgKey: 'hum' },
                { cardId: 'card-hum-scd', badgeId: 'badge-hum-scd', val: parseFloat(data['Hum_SCD30(%)']), cfgKey: 'hum' },
                { cardId: 'card-pres', badgeId: 'badge-pres', val: parseFloat(data['Pression(hPa)']), cfgKey: 'pres' },
                { cardId: 'card-co2-scd', badgeId: 'badge-co2-scd', val: parseFloat(data['CO2_SCD30(ppm)']), cfgKey: 'co2' },
                { cardId: 'card-co2-mhz', badgeId: 'badge-co2-mhz', val: parseFloat(data['CO2_MHZ16(ppm)']), cfgKey: 'co2' },
                { cardId: 'card-pm10-fine', badgeId: 'badge-pm10-fine', val: parseFloat(data['PM1.0']), cfgKey: 'pm25' },
                { cardId: 'card-pm25', badgeId: 'badge-pm25', val: parseFloat(data['PM2.5']), cfgKey: 'pm25' },
                { cardId: 'card-pm10', badgeId: 'badge-pm10', val: parseFloat(data['PM10']), cfgKey: 'pm10' },
                { cardId: 'card-voc-sgp', badgeId: 'badge-voc-sgp', val: parseFloat(data['VOC_SGP40']), cfgKey: 'sgp40' },
                { cardId: 'card-no2', badgeId: 'badge-no2', val: parseFloat(data['NO2']), cfgKey: 'no2' },
                { cardId: 'card-etoh', badgeId: 'badge-etoh', val: parseFloat(data['EtOH']), cfgKey: 'etoh' },
                { cardId: 'card-voc-multi', badgeId: 'badge-voc-multi', val: parseFloat(data['VOC_Multi']), cfgKey: 'voc_multi' },
                { cardId: 'card-co', badgeId: 'badge-co', val: parseFloat(data['CO']), cfgKey: 'co' },
                { cardId: 'card-tvoc-sgp30', badgeId: 'badge-tvoc-sgp30', val: parseFloat(data['TVOC_SGP30(ppb)']), cfgKey: 'tvoc' },
                { cardId: 'card-eco2-sgp30', badgeId: 'badge-eco2-sgp30', val: parseFloat(data['eCO2_SGP30(ppm)']), cfgKey: 'co2' },
                { cardId: 'card-hcho', badgeId: 'badge-hcho', val: parseFloat(data['HCHO(ppm)']), cfgKey: 'hcho' }
            ];

            mappings.forEach(m => {
                const card = document.getElementById(m.cardId);
                const badge = document.getElementById(m.badgeId);
                const cfg = thresholdConfigs[m.cfgKey];
                if (!card || !cfg || isNaN(m.val)) return;

                const statusClass = cfg.eval(m.val);
                card.classList.remove('card-thresh-good', 'card-thresh-acceptable', 'card-thresh-warning', 'card-thresh-alert', 'card-thresh-danger');
                card.classList.add('card-thresh-' + statusClass);

                if (badge) {
                    badge.className = 'status-badge status-' + statusClass;
                    const labelsMap = { 'good': 'Bon', 'acceptable': 'Acceptable', 'warning': 'Dégradé', 'alert': 'Alerte', 'danger': 'Danger' };
                    badge.innerText = labelsMap[statusClass] || statusClass;
                }
            });
        }
"""

def update_file(filepath):
    print(f"Updating {filepath}...")
    with open(filepath, "r", encoding="utf-8") as f:
        content = f.read()

    # Add CSS if not present
    if "/* --- HELP BUTTON & THRESHOLD STYLES --- */" not in content:
        content = content.replace("</style>", css_to_add.strip() + "\n    </style>")

    # Add Info Modal HTML if not present
    if "id=\"info-modal\"" not in content:
        content = content.replace("</body>", info_modal_html.strip() + "\n</body>")

    # Replace dashboard grid cards
    start_grid = content.find('<div class="dashboard-grid">')
    end_grid = content.find('</div> <!-- END TAB 1 -->')
    if start_grid != -1 and end_grid != -1:
        content = content[:start_grid] + cards_html.strip() + "\n    " + content[end_grid:]

    # Add JS threshold logic if not present
    if "const thresholdConfigs" not in content:
        content = content.replace("</script>\n</body>", js_threshold_logic.strip() + "\n    </script>\n</body>")

    # Call updateCardThresholds in fetchLatestData if not present
    if "updateCardThresholds(data);" not in content:
        content = content.replace("document.getElementById('val-hcho').innerText = data['HCHO(ppm)'];", "document.getElementById('val-hcho').innerText = data['HCHO(ppm)'];\n                    updateCardThresholds(data);")

    with open(filepath, "w", encoding="utf-8") as f:
        f.write(content)
    print(f"[OK] {filepath} updated.")

files_to_update = [
    r"c:\Users\sylva\Documents\! projet climat\Station_Complete_wifi\templates\dashboard.html",
    r"c:\Users\sylva\Documents\! projet climat\station_complete_lora\templates\dashboard.html",
    r"c:\Users\sylva\Documents\! projet climat\station_complete_lora_no_python\sd_card_files\dashboard.html",
    r"c:\Users\sylva\Documents\! projet climat\station_complete_lora_no_python\sd_card_files\dash.htm"
]

for f in files_to_update:
    if os.path.exists(f):
        update_file(f)
