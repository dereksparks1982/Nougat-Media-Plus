(() => {
  'use strict';

  // This viewer is an HTTPS GitHub Pages shell, not a media-file host.
  // Keep credentials out of requests; public bridge routes are intentionally
  // separate from the Hanafi private-library and administrative routes.
  const sharedHost = window.NougatWebHost;
  const RESUME_PREFIX = 'nougat-watch-position-v1:';
  const REQUEST_TIMEOUT = 9000;
  const MAX_ITEMS = 2500;
  const el = (id) => document.getElementById(id);
  const video = el('video');
  const grid = el('library');
  const search = el('searchInput');
  const notice = el('libraryNotice');
  const status = el('connectionStatus');
  const statusLight = el('connectionLight');
  const serverInput = el('serverUrl');
  const serverForm = el('serverForm');
  const refreshButton = el('retry');
  const compatButton = el('compatibility');
  const config = window.NOUGAT_WEB_PLAYER || {};
  let base = '';
  let items = [];
  let selected = null;
  let mode = 'all';
  let compatible = false;
  let fallbackUsed = false;
  let connecting = false;
  let generation = 0;
  let resume = {};
  let lastSave = 0;
  let restoreAt = 0;

  function normalizeServer(value) {
    return sharedHost ? sharedHost.validate(value) : '';
  }
  function defaultServer() {
    return sharedHost ? sharedHost.getDefault() : normalizeServer(config.baseUrl);
  }
  function savedServer() {
    return sharedHost ? sharedHost.getBase() : defaultServer();
  }
  function api(path, id = '') {
    const query = id ? '?id=' + encodeURIComponent(id) : '';
    return base + path + query;
  }
  function statusMessage(text, state) {
    status.textContent = text;
    statusLight.classList.remove('online', 'offline');
    if (state) statusLight.classList.add(state);
  }
  function noticeMessage(text) {
    notice.textContent = text;
    notice.hidden = !text;
  }
  async function getJSON(path) {
    const controller = new AbortController();
    const timer = setTimeout(() => controller.abort(), REQUEST_TIMEOUT);
    try {
      const response = await fetch(api(path), {
        mode: 'cors',
        credentials: new URL(api(path), location.href).origin === location.origin ? 'same-origin' : 'omit',
        redirect: 'error', cache: 'no-store', signal: controller.signal
      });
      if (!response.ok) throw new Error('HTTP ' + response.status);
      return await response.json();
    } finally { clearTimeout(timer); }
  }
  function mediaType(raw) {
    const type = String(raw.kind || raw.type || '').toLowerCase();
    return ['episode', 'tv', 'television'].includes(type) ? 'tv' : 'movies';
  }
  function isDirect(raw) {
    if (typeof raw.directPreferred === 'boolean') return raw.directPreferred;
    const mime = String(raw.contentType || raw.content_type || '').toLowerCase();
    if (/video\/(mp4|webm|ogg)/.test(mime)) return true;
    return /\.(mp4|m4v|webm|ogv)$/i.test(String(raw.filename || raw.path || ''));
  }
  function normalize(raw) {
    const id = String(raw.id || '').trim();
    if (!id || id.length > 512) return null;
    return {
      id,
      title: String(raw.title || raw.name || raw.filename || 'Untitled').slice(0, 200),
      year: Number(raw.year || raw.production_year) || 0,
      kind: mediaType(raw),
      ready: raw.ready !== false,
      direct: isDirect(raw)
    };
  }
  function resumeKey() { return RESUME_PREFIX + base; }
  function readResume() {
    if (window.NougatWebResume) return window.NougatWebResume.read(base);
    try {
      const record=JSON.parse(localStorage.getItem(resumeKey())||'{}');
      return record && typeof record === 'object' && !Array.isArray(record) ? record : {};
    } catch (_) { return {}; }
  }
  function saveResume(force = false) {
    if (!selected || !Number.isFinite(video.currentTime)) return;
    const now = Date.now();
    if (!force && now - lastSave < 4000) return;
    lastSave = now;
    const seconds = video.ended || (Number.isFinite(video.duration) &&
      video.duration - video.currentTime < 20) ? 0 : video.currentTime;
    if (window.NougatWebResume) {
      window.NougatWebResume.set(base, selected.id, {
        position_ms: Math.round(seconds * 1000),
        duration_ms: Number.isFinite(video.duration) ? Math.round(video.duration * 1000) : 0,
        updated: now,
        completed: seconds <= 10,
        title: selected.title
      });
      resume = readResume();
    } else {
      if (seconds > 10) resume[selected.id] = { seconds, updated: now };
      else delete resume[selected.id];
      try { localStorage.setItem(resumeKey(), JSON.stringify(resume)); } catch (_) {}
    }
  }
  function timeLabel(seconds) {
    const min = Math.floor(seconds / 60);
    return min >= 60 ? Math.floor(min / 60) + 'h ' + min % 60 + 'm' : min + 'm';
  }
  function filteredItems() {
    const query=search.value.toLocaleLowerCase().trim();
    const filtered=items.filter(item => {
      const position=Number(resume[item.id]?.position_ms||0);
      const incomplete=resume[item.id]?.completed !== true;
      const matches=mode==='all' || item.kind===mode ||
        (mode==='continue' && position>=10000 && incomplete);
      return matches && (!query || item.title.toLocaleLowerCase().includes(query) ||
        String(item.year).includes(query));
    });
    if (mode==='continue') filtered.sort((a,b) =>
      Number(resume[b.id]?.updated||0)-Number(resume[a.id]?.updated||0));
    return filtered;
  }
  function createPoster(item) {
    const container = document.createElement('span');
    container.className = 'poster';
    container.textContent = item.kind === 'tv' ? 'TV' : '▶';
    const img = document.createElement('img');
    img.alt = '';
    img.loading = 'lazy';
    let fallback = false;
    img.addEventListener('load', () => { container.replaceChildren(img); });
    img.addEventListener('error', () => {
      if (fallback) return;
      fallback = true;
      img.src = api('/nougat/v1/artwork', item.id);
    }, { once: true });
    img.src = api('/nougat/v1/poster', item.id);
    return container;
  }
  function render() {
    const matches = filteredItems();
    el('libraryCount').textContent = items.length + (items.length === 1 ? ' title' : ' titles');
    const fragment = document.createDocumentFragment();
    for (const item of matches) {
      const card = document.createElement('button');
      card.type = 'button';
      card.className = 'media-card';
      card.setAttribute('aria-label', 'Play ' + item.title);
      card.appendChild(createPoster(item));
      const text = document.createElement('span');
      text.className = 'card-info';
      const title = document.createElement('span');
      title.className = 'card-title';
      title.textContent = item.title;
      const details = document.createElement('span');
      details.className = 'card-meta';
      const position = Number(
        resume[item.id]?.position_ms != null
          ? resume[item.id].position_ms / 1000
          : resume[item.id]?.seconds || 0);
      details.textContent = [item.kind === 'tv' ? 'TV Episode' : 'Movie',
        item.year || '', position > 10 ? 'Resume ' + timeLabel(position) : '',
        item.ready ? '' : 'Unavailable'].filter(Boolean).join(' · ');
      text.append(title, details);
      card.appendChild(text);
      card.disabled = !item.ready;
      card.addEventListener('click', () => start(item));
      fragment.appendChild(card);
    }
    grid.replaceChildren(fragment);
    noticeMessage(matches.length ? '' : (items.length
      ? 'No matching titles. Try another search or filter.'
      : 'The media catalog is empty or the server is unavailable.'));
  }
  async function refresh() {
    if (window.NougatLegalAccepted !== true || !base || connecting) return;
    connecting = true;
    const attempt = ++generation;
    refreshButton.disabled = true;
    statusMessage('Connecting to your media server…');
    noticeMessage('Loading library…');
    try {
      const health = await getJSON('/nougat/v1/health');
      if (attempt !== generation) return;
      if (health.ok === false) throw new Error('Media server is not ready');
      const data = await getJSON('/nougat/v1/catalog');
      if (attempt !== generation) return;
      if (data.ok === false || !Array.isArray(data.items))
        throw new Error('The server returned an invalid media catalog');
      items = data.items.slice(0, MAX_ITEMS).map(normalize).filter(Boolean);
      statusMessage('Connected · ' + items.length + ' titles', 'online');
      noticeMessage(items.length ? '' : 'No media is currently available from this server.');
      render();
    } catch (error) {
      if (attempt !== generation) return;
      statusMessage(navigator.onLine === false ? 'Network offline' : 'Server unavailable', 'offline');
      noticeMessage('Could not reach the HTTPS media bridge. Make sure the host is running, the HTTPS certificate is valid, and this site is allowed by CORS. ' +
        (error?.name === 'AbortError' ? 'The connection timed out.' : ''));
      // Keep the last in-memory catalog on a temporary outage.
      if (items.length) render();
    } finally {
      if (attempt === generation) { connecting = false; refreshButton.disabled = false; }
    }
  }
  function setServer(value, persist = false) {
    const localMode = location.protocol === 'http:' &&
      !String(config.baseUrl || '').trim() &&
      (!value || value === location.origin);
    const parsed = localMode ? '' : normalizeServer(value);
    if (!parsed && !localMode) return false;
    if (selected) { saveResume(true); selected = null; }
    video.pause();
    video.removeAttribute('src');
    video.load();
    el('watchPanel').hidden = true;
    base = parsed;
    resume = readResume();
    items = [];
    generation += 1;
    connecting = false;
    refreshButton.disabled = false;
    serverInput.value = base;
    statusMessage('Ready to connect');
    render();
    if (persist && !localMode && sharedHost && !sharedHost.setServer(base)) {
      statusMessage('Browser settings could not be saved', 'offline');
    }
    refresh();
    return true;
  }
  function stream(item, useCompat) {
    return api(useCompat ? '/nougat/v1/transcode' : '/nougat/v1/media', item.id);
  }
  function setPlaybackMessage(text) { el('playbackStatus').textContent = text; }
  function startStream(useCompat, startAt) {
    if (!selected) return;
    compatible = useCompat;
    compatButton.textContent = compatible ? 'Try direct playback' : 'Try compatibility stream';
    restoreAt = Number.isFinite(startAt) && startAt > 0 ? startAt : 0;
    video.src = stream(selected, compatible);
    video.load();
    setPlaybackMessage(compatible ? 'Starting browser-compatible stream…' : 'Starting direct playback…');
    video.play().catch(() => setPlaybackMessage('Ready. Tap the Play control to begin.'));
  }
  function start(item) {
    if (selected) saveResume(true);
    selected = item;
    fallbackUsed = false;
    el('nowTitle').textContent = item.title + (item.year ? ' (' + item.year + ')' : '');
    el('watchPanel').hidden = false;
    el('watchPanel').scrollIntoView({ behavior: 'smooth', block: 'start' });
    const previous = Number(
      resume[item.id]?.position_ms != null
        ? resume[item.id].position_ms / 1000
        : resume[item.id]?.seconds || 0);
    startStream(!item.direct, previous);
  }

  search.addEventListener('input', render);
  document.querySelectorAll('[data-filter]').forEach(button =>
    button.addEventListener('click', () => {
      mode = button.dataset.filter;
      for (const other of document.querySelectorAll('[data-filter]')) {
        const active = other === button;
        other.classList.toggle('active', active);
        other.setAttribute('aria-pressed', String(active));
      }
      render();
    }));
  serverForm.addEventListener('submit', event => {
    event.preventDefault();
    if (!setServer(serverInput.value, true)) {
      statusMessage('Enter an HTTPS server origin without a path, username or password', 'offline');
      serverInput.focus();
    } else el('connectionSettings').open = false;
  });
  el('resetServer').addEventListener('click', () => {
    if (sharedHost && !sharedHost.reset()) {
      statusMessage('Cannot reset settings in this browser', 'offline');
      return;
    }
    const defaultValue = defaultServer() ||
      (location.protocol === 'http:' && !config.baseUrl ? location.origin : '');
    if (!setServer(defaultValue)) statusMessage('Default media bridge is not configured', 'offline');
    else el('connectionSettings').open = false;
  });
  refreshButton.addEventListener('click', refresh);
  compatButton.addEventListener('click', () => {
    if (!selected) return;
    const offset = Number.isFinite(video.currentTime) && video.currentTime > 0
      ? video.currentTime : restoreAt;
    fallbackUsed = true;
    startStream(!compatible, offset);
  });
  el('back10').addEventListener('click', () => {
    if (Number.isFinite(video.currentTime)) video.currentTime = Math.max(0, video.currentTime - 10);
  });
  el('forward10').addEventListener('click', () => {
    if (Number.isFinite(video.duration) && video.duration > 0)
      video.currentTime = Math.min(video.duration - 0.1, video.currentTime + 10);
  });
  el('closePlayer').addEventListener('click', () => {
    saveResume(true);
    video.pause();
    video.removeAttribute('src');
    video.load();
    selected = null;
    el('watchPanel').hidden = true;
    render();
  });
  video.addEventListener('loadedmetadata', () => {
    if (restoreAt > 0 && Number.isFinite(video.duration)) {
      try { video.currentTime = Math.min(restoreAt, Math.max(0, video.duration - 1)); } catch (_) {}
    }
    restoreAt = 0;
    setPlaybackMessage('Ready · ' + (compatible ? 'Compatibility stream' : 'Direct stream'));
  });
  video.addEventListener('timeupdate', () => saveResume());
  video.addEventListener('pause', () => saveResume(true));
  video.addEventListener('ended', () => { saveResume(true); setPlaybackMessage('Playback complete'); render(); });
  video.addEventListener('waiting', () => setPlaybackMessage('Buffering…'));
  video.addEventListener('error', () => {
    if (!selected) return;
    if (!compatible && !fallbackUsed) {
      fallbackUsed = true;
      const offset = Number.isFinite(video.currentTime) ? video.currentTime : restoreAt;
      setPlaybackMessage('Direct stream unavailable. Trying browser compatibility…');
      startStream(true, offset);
    } else {
      setPlaybackMessage('Playback failed. Check your media host and try the other stream mode.');
    }
  });
  document.addEventListener('visibilitychange', () => {
    if (!document.hidden && window.NougatLegalAccepted === true && base && !connecting) refresh();
  });
  window.addEventListener('online', () => {
    if (window.NougatLegalAccepted === true) refresh();
  });
  window.addEventListener('beforeunload', () => saveResume(true));
  window.addEventListener('nougat:resume-changed', () => {
    resume = readResume();
    render();
  });
  window.addEventListener('nougat:host-changed', () => {
    if (window.NougatLegalAccepted !== true || !sharedHost) return;
    const next = sharedHost.getBase();
    if (next && next !== base) setServer(next);
  });

  function init() {
    const configured = savedServer() || defaultServer() ||
      (location.protocol === 'http:' && !config.baseUrl ? location.origin : '');
    if (!configured) {
      statusMessage('No HTTPS bridge configured', 'offline');
      el('connectionSettings').open = true;
      return;
    }
    setServer(configured);
  }
  // Avoid all library and health requests until the legal notice is accepted.
  if (window.NougatLegalAccepted === true) init();
  else window.addEventListener('nougat:legal-accepted', init, { once: true });

  if ('serviceWorker' in navigator && location.protocol === 'https:') {
    navigator.serviceWorker.register('../sw.js', { scope: '../' })
      .then(registration => registration.update()).catch(() => {});
  }
})();
