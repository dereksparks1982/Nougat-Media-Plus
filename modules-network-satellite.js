(() => {
  'use strict';
  if (!window.NougatWebModules) return;

  const previous = window.NougatWebModules.activate.bind(window.NougatWebModules);
  const root = () => document.getElementById('moduleView');
  const esc = (value) => String(value ?? '').replace(/[&<>"']/g, (c) => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
  const button = (label, id, active = false) => `<button type="button" class="sheet-button${active ? ' active-tool' : ''}" id="${id}">${esc(label)}</button>`;
  const bind = (id, fn) => { const el = document.getElementById(id); if (el) el.addEventListener('click', fn); };

  let networkPanel = 'Overview';
  let satellitePanel = 'Track';
  let lastSnapshot = null;

  const networkPanels = ['Overview','Connections','Devices','Security','Satellite','Diagnostics','Logs'];
  const satellitePanels = ['Track','Passes','Receive','Decode','Transmit','Imagery','Antenna','Hardware','Logs'];

  function shell(html) {
    const r = root();
    if (!r) return null;
    r.innerHTML = `<div class="module-workspace">${html}</div>`;
    return r.querySelector('.module-workspace');
  }

  function connectionInfo() {
    const c = navigator.connection || navigator.mozConnection || navigator.webkitConnection || null;
    if (!c) return [];
    const lines = [];
    if (c.type) lines.push(`Connection type: ${c.type}`);
    if (c.effectiveType) lines.push(`Effective type: ${c.effectiveType}`);
    if (Number.isFinite(c.downlink)) lines.push(`Estimated downlink: ${c.downlink} Mbps`);
    if (Number.isFinite(c.rtt)) lines.push(`Estimated RTT: ${c.rtt} ms`);
    if (typeof c.saveData === 'boolean') lines.push(`Data saver: ${c.saveData ? 'enabled' : 'disabled'}`);
    return lines;
  }

  async function collectBrowserSnapshot(host) {
    const base = host && typeof host.baseUrl === 'function' ? host.baseUrl() : '';
    const snapshot = {
      timestamp: new Date().toLocaleString(),
      bridgeOnline: false,
      bridgeLatency: null,
      bridgeService: 'Unavailable',
      bridgeVersion: 'Unknown',
      base,
      catalogCount: null,
      healthError: '',
      catalogError: ''
    };

    try {
      const start = performance.now();
      const response = await fetch(`${base}/nougat/v1/health`, { cache: 'no-store' });
      const data = await response.json();
      snapshot.bridgeLatency = Math.max(0, Math.round(performance.now() - start));
      snapshot.bridgeOnline = response.ok && data && data.ok !== false;
      snapshot.bridgeService = data.service || data.product || 'Nougat Web Player';
      snapshot.bridgeVersion = data.version || 'Unknown';
      if (!response.ok) snapshot.healthError = data.error || `HTTP ${response.status}`;
    } catch (error) {
      snapshot.healthError = error && error.message ? error.message : String(error);
    }

    try {
      const response = await fetch(`${base}/nougat/v1/catalog`, { cache: 'no-store' });
      const data = await response.json();
      if (response.ok && data) snapshot.catalogCount = Number(data.count ?? (Array.isArray(data.items) ? data.items.length : NaN));
      else snapshot.catalogError = data && data.error ? data.error : `HTTP ${response.status}`;
    } catch (error) {
      snapshot.catalogError = error && error.message ? error.message : String(error);
    }

    return snapshot;
  }

  function lineBlock(lines) {
    return `<div class="module-output">${lines.map(esc).join('\n')}</div>`;
  }

  function networkLines(panel, snapshot) {
    const connection = connectionInfo();
    const bridge = snapshot || {};

    if (panel === 'Overview') {
      return [
        'Health: collecting live browser/network state',
        `Nougat bridge: ${bridge.bridgeOnline ? 'online' : 'offline'}`,
        `Bridge endpoint: ${bridge.base || 'same origin'}`,
        `Bridge service: ${bridge.bridgeService || 'Unavailable'}`,
        `Bridge version: ${bridge.bridgeVersion || 'Unknown'}`,
        `Bridge latency: ${Number.isFinite(bridge.bridgeLatency) ? bridge.bridgeLatency + ' ms' : 'unavailable'}`,
        `Browser online: ${navigator.onLine ? 'yes' : 'no'}`,
        ...connection,
        'Local IPv4 / IPv6: hidden by normal browser privacy boundaries',
        'Gateway / DNS / MAC / MTU / Wi-Fi SSID: host-only data is not exposed by the browser'
      ];
    }

    if (panel === 'Connections') {
      return [
        'Active browser connections:',
        `  Nougat bridge: ${bridge.bridgeOnline ? 'connected' : 'not connected'}`,
        `  Bridge endpoint: ${bridge.base || 'same origin'}`,
        `  Catalog request: ${Number.isFinite(bridge.catalogCount) ? bridge.catalogCount + ' indexed item(s)' : 'unavailable'}`,
        ...connection.map((line) => `  ${line}`),
        'Raw TCP/UDP socket inventory remains a host Network Center function and is not fabricated in the browser.'
      ];
    }

    if (panel === 'Devices') {
      return [
        'Browser device:',
        `  Platform: ${navigator.platform || 'not reported'}`,
        `  CPU threads: ${navigator.hardwareConcurrency || 'not reported'}`,
        `  Device memory: ${navigator.deviceMemory ? navigator.deviceMemory + ' GB' : 'not reported'}`,
        `  Screen: ${screen.width} x ${screen.height}`,
        'LAN neighbor enumeration is not exposed to ordinary web pages, so no neighbor list is invented here.'
      ];
    }

    if (panel === 'Security') {
      return [
        'Defensive assessment only. Findings are indicators to review, not proof of compromise.',
        `Secure context: ${window.isSecureContext ? 'yes' : 'no'}`,
        `Service worker API: ${'serviceWorker' in navigator ? 'available' : 'unavailable'}`,
        `Cookies enabled: ${navigator.cookieEnabled ? 'yes' : 'no'}`,
        `Nougat bridge reachable: ${bridge.bridgeOnline ? 'yes' : 'no'}`,
        'Listening sockets and firewall status require the host Network Center and are not guessed by the web player.',
        'No system settings were changed by this browser scan.'
      ];
    }

    if (panel === 'Diagnostics') {
      return [
        `Internet/browser reachability: ${navigator.onLine ? 'browser reports online' : 'browser reports offline'}`,
        `Nougat health probe: ${bridge.bridgeOnline ? 'passed' : 'failed'}`,
        `Health latency: ${Number.isFinite(bridge.bridgeLatency) ? bridge.bridgeLatency + ' ms' : 'unavailable'}`,
        `Catalog probe: ${Number.isFinite(bridge.catalogCount) ? 'passed, ' + bridge.catalogCount + ' item(s)' : 'unavailable'}`,
        bridge.healthError ? `Health error: ${bridge.healthError}` : 'Health error: none',
        bridge.catalogError ? `Catalog error: ${bridge.catalogError}` : 'Catalog error: none'
      ];
    }

    return [
      `Network Center web snapshot: ${bridge.timestamp || new Date().toLocaleString()}`,
      `Bridge: ${bridge.bridgeOnline ? 'online' : 'offline'}`,
      `Browser online: ${navigator.onLine ? 'yes' : 'no'}`,
      'Snapshot collected from browser connection state and the Nougat web bridge.',
      'No system settings were changed by this scan.'
    ];
  }

  function satelliteLines(panel) {
    switch (panel) {
      case 'Track':
        return [
          'Orbital engine: SGP4/SDP4 is installed in the standalone Nougat Satellite Center',
          'Catalog path on host: ~/.config/nougat/satellite/catalog.tle',
          'Web status: host TLE catalog and propagated satellite coordinates are not exposed through the current web bridge.',
          'Tracking remains truthful: no satellite position is invented without orbital data.'
        ];
      case 'Passes':
        return [
          'Pass prediction foundation: SGP4 orbital propagation ready in the standalone host',
          'Observer location and pass scheduler are not exposed to this browser yet.',
          'Native planned outputs: AOS, maximum elevation, LOS, duration, azimuth/elevation and Doppler.'
        ];
      case 'Receive':
        return [
          'Receive workspace: hardware-neutral foundation active',
          'Native backends: RTL-SDR and SoapySDR detection',
          'Browser status: receiver hardware is not reported READY unless the host bridge actually exposes it.'
        ];
      case 'Decode':
        return [
          'Decode workspace foundation active',
          'Telemetry, image and packet decoders attach here as supported backends are added.',
          'Encrypted/unsupported data remains labeled encrypted, unsupported or unknown.'
        ];
      case 'Transmit':
        return [
          'TX STATE: DISABLED',
          'No transmitter backend is armed by this foundation build.',
          'Native future controls: uplink frequency, modulation, bandwidth, power, Doppler compensation and logging.'
        ];
      case 'Imagery':
        return [
          'Earth Observation / Satellite Imagery foundation active',
          'Native GDAL command-tool detection belongs to the standalone host.',
          'Source classes: ONLINE SATELLITE DATA and DIRECT SATELLITE RECEPTION',
          'Native planned processing: natural color, infrared, thermal, false color, raw bands and before/after comparison where source data supports them.'
        ];
      case 'Antenna':
        return [
          'Antenna/rotator control foundation active',
          'Native backend: Hamlib rotator tools',
          'Automatic azimuth/elevation tracking remains disabled until a supported rotator and observer location are configured.'
        ];
      case 'Hardware':
        return [
          'SGP4 orbital engine: available in standalone host',
          'GDAL command tools: host-detected',
          'RTL-SDR: host-detected',
          'SoapySDR: host-detected',
          'Hamlib radio: host-detected',
          'Hamlib rotator: host-detected',
          'The browser does not claim hardware availability without a host report.'
        ];
      default:
        return [
          'Satellite Center foundation loaded.',
          'No RF transmission was performed.',
          'No satellite position is shown without a valid TLE and successful SGP4 propagation.'
        ];
    }
  }

  function renderSatellitePane() {
    const body = document.getElementById('networkBody');
    if (!body) return;
    body.innerHTML = `
      <div class="module-toolbar search-tabs">${satellitePanels.map((panel, index) => button(panel, `satelliteTab${index}`, satellitePanel === panel)).join('')}</div>
      <h2 class="module-heading">SATELLITE • ${esc(satellitePanel.toUpperCase())}</h2>
      ${lineBlock(satelliteLines(satellitePanel))}`;
    satellitePanels.forEach((panel, index) => bind(`satelliteTab${index}`, () => {
      satellitePanel = panel;
      renderSatellitePane();
    }));
  }

  function renderNetworkBody() {
    if (networkPanel === 'Satellite') {
      renderSatellitePane();
      return;
    }
    const body = document.getElementById('networkBody');
    if (!body) return;
    body.innerHTML = `<h2 class="module-heading">NETWORK CENTER • ${esc(networkPanel.toUpperCase())}</h2>${lineBlock(networkLines(networkPanel, lastSnapshot))}`;
  }

  async function refreshNetwork(host) {
    const status = document.getElementById('networkStatus');
    if (status) status.textContent = 'Collecting live web/network state...';
    lastSnapshot = await collectBrowserSnapshot(host);
    if (status) status.textContent = `SNAPSHOT ${lastSnapshot.bridgeOnline ? 'ONLINE' : 'OFFLINE'} • ${lastSnapshot.timestamp}`;
    renderNetworkBody();
  }

  async function renderNetwork(host) {
    shell(`
      <div class="module-toolbar search-tabs">${networkPanels.map((panel, index) => button(panel, `networkTab${index}`, networkPanel === panel)).join('')}</div>
      <div class="module-toolbar">${button('Refresh','networkRefresh')}</div>
      <div class="status-line" id="networkStatus">Collecting live web/network state...</div>
      <div id="networkBody"></div>`);

    networkPanels.forEach((panel, index) => bind(`networkTab${index}`, () => {
      networkPanel = panel;
      renderNetworkBody();
      document.querySelectorAll('[id^="networkTab"]').forEach((el) => el.classList.remove('active-tool'));
      const active = document.getElementById(`networkTab${index}`);
      if (active) active.classList.add('active-tool');
    }));
    bind('networkRefresh', () => refreshNetwork(host));

    if (!lastSnapshot) await refreshNetwork(host);
    else {
      const status = document.getElementById('networkStatus');
      if (status) status.textContent = `SNAPSHOT ${lastSnapshot.bridgeOnline ? 'ONLINE' : 'OFFLINE'} • ${lastSnapshot.timestamp}`;
      renderNetworkBody();
    }
  }

  window.NougatWebModules.activate = function(name, host) {
    if (name === 'network') return renderNetwork(host);
    return previous(name, host);
  };
})();
