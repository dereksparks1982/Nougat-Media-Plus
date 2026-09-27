(() => {
  'use strict';

  const config = window.NOUGAT_WEB_PLAYER || {};
  const player = document.getElementById('player');
  const emptyPlayer = document.getElementById('emptyPlayer');
  const playerTitleHud = document.getElementById('playerTitleHud');
  const playerStatus = document.getElementById('playerStatus');
  const healthBadge = document.getElementById('healthBadge');
  const serverLight = document.getElementById('serverLight');
  const continueRow = document.getElementById('continueRow');
  const homeGrid = document.getElementById('homeGrid');
  const catalogStatus = document.getElementById('catalogStatus');
  const libraryGrid = document.getElementById('libraryGrid');
  const libraryList = document.getElementById('libraryList');
  const search = document.getElementById('search');
  const refreshButton = document.getElementById('refreshButton');
  const moviesFilter = document.getElementById('moviesFilter');
  const tvFilter = document.getElementById('tvFilter');
  const listViewButton = document.getElementById('listViewButton');
  const gridViewButton = document.getElementById('gridViewButton');
  const moduleView = document.getElementById('moduleView');
  const moduleTitle = document.getElementById('moduleTitle');
  const moduleMessage = document.getElementById('moduleMessage');
  const playPause = document.getElementById('playPause');
  const seekControl = document.getElementById('seekControl');
  const currentTimeLabel = document.getElementById('currentTime');
  const durationTimeLabel = document.getElementById('durationTime');
  const prevItem = document.getElementById('prevItem');
  const nextItem = document.getElementById('nextItem');
  const back30 = document.getElementById('back30');
  const forward30 = document.getElementById('forward30');
  const compatButton = document.getElementById('compatButton');
  const fullscreenButton = document.getElementById('fullscreenButton');
  const volumeBank = document.getElementById('volumeBank');

  const RESUME_KEY = 'nougat-web-resume-v2';
  const RESUME_THRESHOLD_MS = 10000;
  const COMPLETE_WINDOW_MS = 30000;
  const moduleLabels = {
    discover: 'DISCOVER', livetv: 'LIVE TV', worldtv: 'WORLD TV', radio: 'RADIO',
    search: 'SEARCH', stream: 'STREAM', studio: 'STUDIO', games: 'GAMES',
    network: 'NETWORK', system: 'SYSTEM'
  };

  let items = [];
  let selected = null;
  let usingCompatibility = false;
  let autoFallbackArmed = false;
  let libraryFilter = 'movies';
  let libraryMode = 'grid';
  let currentView = 'home';
  let lastResumeWrite = 0;
  let resumeState = readResumeState();

  function readResumeState() {
    try {
      const parsed = JSON.parse(localStorage.getItem(RESUME_KEY) || '{}');
      return parsed && typeof parsed === 'object' ? parsed : {};
    } catch (_) {
      return {};
    }
  }

  function writeResumeState() {
    try { localStorage.setItem(RESUME_KEY, JSON.stringify(resumeState)); } catch (_) {}
  }

  function normalizedBaseUrl() {
    const configured = typeof config.baseUrl === 'string' ? config.baseUrl.trim() : '';
    return configured.replace(/\/+$/, '');
  }

  function apiUrl(path, id = '') {
    const base = config.enabled === false ? '' : normalizedBaseUrl();
    const query = id ? `?id=${encodeURIComponent(id)}` : '';
    return `${base}${path}${query}`;
  }

  function inferDirectPreferred(raw) {
    if (typeof raw.directPreferred === 'boolean') return raw.directPreferred;
    const contentType = String(raw.content_type || raw.contentType || '').toLowerCase();
    if (/video\/(mp4|webm|ogg)/.test(contentType)) return true;
    const filename = String(raw.filename || raw.path || '').toLowerCase();
    return /\.(mp4|m4v|webm|ogv|ogg)$/.test(filename);
  }

  function normalizeItem(raw) {
    const kind = String(raw.kind || raw.type || '').toLowerCase();
    const isEpisode = kind === 'tv' || kind === 'episode' || kind === 'television';
    return {
      id: String(raw.id || ''),
      name: String(raw.name || raw.title || raw.filename || 'Untitled media'),
      filename: String(raw.filename || raw.path || ''),
      type: isEpisode ? 'Episode' : 'Movie',
      year: Number(raw.year || raw.production_year) || 0,
      directPreferred: inferDirectPreferred(raw),
      ready: raw.ready !== false,
      poster: String(raw.poster || raw.posterUrl || raw.image || raw.imageUrl || '')
    };
  }

  function itemMeta(item) {
    const parts = [item.type === 'Episode' ? 'TV Episode' : 'Movie'];
    if (item.year > 0) parts.push(String(item.year));
    return parts.join(' • ');
  }

  function streamUrl(item, compatibility) {
    return apiUrl(compatibility ? '/nougat/v1/transcode' : '/nougat/v1/media', item.id);
  }

  function formatTime(value) {
    const seconds = Number.isFinite(value) && value > 0 ? Math.floor(value) : 0;
    const h = Math.floor(seconds / 3600);
    const m = Math.floor((seconds % 3600) / 60);
    const s = seconds % 60;
    return h > 0 ? `${h}:${String(m).padStart(2, '0')}:${String(s).padStart(2, '0')}` : `${m}:${String(s).padStart(2, '0')}`;
  }

  function setServerState(state, text) {
    serverLight.classList.remove('online', 'busy', 'offline');
    serverLight.classList.add(state);
    healthBadge.textContent = text;
  }

  function setPlayerStatus(message) {
    playerStatus.textContent = String(message || '').toUpperCase();
  }

  function setView(view, pushHistory = true) {
    const realView = ['home', 'player', 'library'].includes(view) ? view : 'module';
    currentView = view;
    document.querySelectorAll('[data-view-panel]').forEach((panel) => {
      panel.classList.toggle('active', panel.dataset.viewPanel === realView);
    });
    document.querySelectorAll('.rail-button').forEach((button) => {
      button.classList.toggle('active', button.dataset.view === view);
    });
    if (realView === 'module') {
      moduleTitle.textContent = moduleLabels[view] || String(view).toUpperCase();
      moduleMessage.textContent = 'This web player uses the standalone Nougat Media Plus shell. Home, Player and Library are live for remote media playback; this module remains in the standalone application.';
      moduleView.classList.add('active');
    }
    if (pushHistory) {
      const nextHash = `#${view}`;
      if (location.hash !== nextHash) history.pushState({ view }, '', nextHash);
    }
  }

  function viewFromHash() {
    const raw = location.hash.replace(/^#/, '').trim().toLowerCase();
    const allowed = new Set(['home', 'player', 'library', ...Object.keys(moduleLabels)]);
    return allowed.has(raw) ? raw : 'home';
  }

  function resumeRecord(item) {
    return resumeState[item.id] || null;
  }

  function saveResume(force = false) {
    if (!selected || !Number.isFinite(player.currentTime)) return;
    const now = Date.now();
    if (!force && now - lastResumeWrite < 3000) return;
    lastResumeWrite = now;
    const duration = Number.isFinite(player.duration) ? Math.max(0, player.duration * 1000) : 0;
    const position = Math.max(0, player.currentTime * 1000);
    resumeState[selected.id] = {
      position_ms: Math.round(position),
      duration_ms: Math.round(duration),
      updated: now,
      title: selected.name,
      completed: duration > 0 && position >= Math.max(0, duration - COMPLETE_WINDOW_MS)
    };
    writeResumeState();
    renderHome();
  }

  function continueItems() {
    return items.map((item) => ({ item, resume: resumeRecord(item) }))
      .filter(({ resume }) => resume && !resume.completed && resume.position_ms >= RESUME_THRESHOLD_MS &&
        (!resume.duration_ms || resume.position_ms < Math.max(0, resume.duration_ms - COMPLETE_WINDOW_MS)))
      .sort((a, b) => (b.resume.updated || 0) - (a.resume.updated || 0));
  }

  function makeCard(item, resume = null) {
    const card = document.createElement('button');
    card.type = 'button';
    card.className = 'media-card';
    card.disabled = !item.ready;
    card.setAttribute('aria-label', `Play ${item.name}`);

    const art = document.createElement('span');
    art.className = 'media-art';
    if (item.poster) {
      const img = document.createElement('img');
      img.src = item.poster;
      img.alt = '';
      img.loading = 'lazy';
      img.addEventListener('error', () => {
        img.remove();
        const fallback = document.createElement('span');
        fallback.className = 'fallback-mark';
        fallback.textContent = item.type === 'Episode' ? 'TV' : 'MOVIE';
        art.appendChild(fallback);
      }, { once: true });
      art.appendChild(img);
    } else {
      const fallback = document.createElement('span');
      fallback.className = 'fallback-mark';
      fallback.textContent = item.type === 'Episode' ? 'TV' : 'MOVIE';
      art.appendChild(fallback);
    }

    const body = document.createElement('span');
    body.className = 'media-card-body';
    const title = document.createElement('span');
    title.className = 'media-title';
    title.textContent = item.name;
    const meta = document.createElement('span');
    meta.className = 'media-meta';
    meta.textContent = item.ready ? itemMeta(item) : 'Unavailable';
    body.append(title, meta);

    if (resume && resume.duration_ms > 0) {
      const progress = document.createElement('span');
      progress.className = 'progress-line';
      const fill = document.createElement('span');
      fill.style.width = `${Math.max(0, Math.min(100, resume.position_ms / resume.duration_ms * 100))}%`;
      progress.appendChild(fill);
      body.appendChild(progress);
    }

    card.append(art, body);
    card.addEventListener('click', () => playItem(item, !item.directPreferred, resume ? resume.position_ms / 1000 : 0));
    return card;
  }

  function renderHome() {
    continueRow.textContent = '';
    const continues = continueItems();
    if (!continues.length) {
      const empty = document.createElement('div');
      empty.className = 'empty-shelf';
      empty.textContent = items.length ? 'No unfinished items yet.' : 'Loading Continue Watching...';
      continueRow.appendChild(empty);
    } else {
      continues.forEach(({ item, resume }) => continueRow.appendChild(makeCard(item, resume)));
    }

    homeGrid.textContent = '';
    if (!items.length) {
      const empty = document.createElement('div');
      empty.className = 'empty-shelf';
      empty.textContent = 'No local media is currently indexed.';
      homeGrid.appendChild(empty);
      return;
    }
    items.forEach((item) => homeGrid.appendChild(makeCard(item, null)));
  }

  function filteredLibraryItems() {
    const term = search.value.trim().toLowerCase();
    return items.filter((item) => {
      if (libraryFilter === 'movies' && item.type !== 'Movie') return false;
      if (libraryFilter === 'tv' && item.type !== 'Episode') return false;
      if (!term) return true;
      return `${item.name} ${item.type} ${item.year} ${item.filename}`.toLowerCase().includes(term);
    });
  }

  function renderLibrary() {
    const filtered = filteredLibraryItems();
    libraryGrid.textContent = '';
    libraryList.textContent = '';

    if (!filtered.length) {
      const empty = document.createElement('div');
      empty.className = 'empty-shelf';
      empty.textContent = items.length ? 'No library items match this view.' : 'No playable media is currently indexed.';
      (libraryMode === 'grid' ? libraryGrid : libraryList).appendChild(empty);
      return;
    }

    filtered.forEach((item) => {
      libraryGrid.appendChild(makeCard(item, resumeRecord(item)));
      const row = document.createElement('button');
      row.type = 'button';
      row.className = 'library-row';
      const title = document.createElement('span');
      title.textContent = item.name;
      const type = document.createElement('span');
      type.textContent = item.type === 'Episode' ? 'Television' : 'Movie';
      const year = document.createElement('span');
      year.textContent = item.year || '';
      row.append(title, type, year);
      row.addEventListener('click', () => {
        const resume = resumeRecord(item);
        playItem(item, !item.directPreferred, resume ? resume.position_ms / 1000 : 0);
      });
      libraryList.appendChild(row);
    });
  }

  function setLibraryFilter(filter) {
    libraryFilter = filter;
    moviesFilter.classList.toggle('active-tool', filter === 'movies');
    tvFilter.classList.toggle('active-tool', filter === 'tv');
    renderLibrary();
  }

  function setLibraryMode(mode) {
    libraryMode = mode;
    libraryGrid.hidden = mode !== 'grid';
    libraryList.hidden = mode !== 'list';
    gridViewButton.classList.toggle('active-tool', mode === 'grid');
    listViewButton.classList.toggle('active-tool', mode === 'list');
    renderLibrary();
  }

  async function playItem(item, compatibility = !item.directPreferred, startAt = 0) {
    if (selected && selected.id !== item.id) saveResume(true);
    selected = item;
    usingCompatibility = compatibility;
    autoFallbackArmed = !compatibility;
    emptyPlayer.hidden = true;
    playerTitleHud.hidden = false;
    playerTitleHud.textContent = item.name;
    compatButton.disabled = false;
    compatButton.textContent = compatibility ? 'DIRECT' : 'COMPAT';
    setView('player');
    setPlayerStatus(compatibility ? 'STARTING BROWSER COMPATIBILITY STREAM' : 'STARTING DIRECT PLAY');

    player.src = streamUrl(item, compatibility);
    player.load();
    const applyStart = () => {
      if (startAt > 0 && Number.isFinite(player.duration)) {
        player.currentTime = Math.min(startAt, Math.max(0, player.duration - 1));
      }
    };
    player.addEventListener('loadedmetadata', applyStart, { once: true });
    try { await player.play(); } catch (_) { setPlayerStatus('READY • PRESS PLAY'); }
  }

  async function loadHealth() {
    setServerState('busy', 'Connecting');
    try {
      const response = await fetch(apiUrl('/nougat/v1/health'), { cache: 'no-store' });
      if (!response.ok) throw new Error('health');
      const health = await response.json();
      const total = Number(health.total) || 0;
      const available = Number(health.available);
      const text = Number.isFinite(available) && total > 0 ? `${available}/${total}` : 'Online';
      setServerState('online', text);
      return true;
    } catch (_) {
      setServerState('offline', 'Offline');
      return false;
    }
  }

  async function loadCatalog() {
    refreshButton.disabled = true;
    catalogStatus.textContent = 'Loading Nougat Library...';
    try {
      const response = await fetch(apiUrl('/nougat/v1/catalog'), { cache: 'no-store' });
      const payload = await response.json();
      if (!response.ok || !payload.ok) throw new Error(payload.error || 'Catalog unavailable');
      items = (Array.isArray(payload.items) ? payload.items : []).map(normalizeItem).filter((item) => item.id);
      const movies = items.filter((item) => item.type === 'Movie').length;
      const episodes = items.length - movies;
      catalogStatus.textContent = `${movies} MOVIES • ${episodes} TV EPISODES • ${items.length} TOTAL`;
      renderHome();
      renderLibrary();
    } catch (error) {
      items = [];
      renderHome();
      renderLibrary();
      catalogStatus.textContent = `LIBRARY UNAVAILABLE • ${error.message || 'SERVER NOT READY'}`;
    } finally {
      refreshButton.disabled = false;
    }
  }

  function updatePlayerControls() {
    const duration = Number.isFinite(player.duration) ? player.duration : 0;
    const current = Number.isFinite(player.currentTime) ? player.currentTime : 0;
    currentTimeLabel.textContent = formatTime(current);
    durationTimeLabel.textContent = formatTime(duration);
    seekControl.value = duration > 0 ? String(Math.round(current / duration * 1000)) : '0';
    playPause.textContent = player.paused ? '▶' : 'Ⅱ';
    updateVolumeBank();
  }

  function createVolumeBank() {
    volumeBank.textContent = '';
    for (let i = 1; i <= 12; i += 1) {
      const segment = document.createElement('button');
      segment.type = 'button';
      segment.className = 'volume-segment';
      segment.style.height = `${7 + i * 1.45}px`;
      segment.setAttribute('aria-label', `Volume ${Math.round(i / 12 * 100)} percent`);
      segment.addEventListener('click', () => {
        player.muted = false;
        player.volume = i / 12;
        updateVolumeBank();
      });
      volumeBank.appendChild(segment);
    }
  }

  function updateVolumeBank() {
    const value = player.muted ? 0 : player.volume;
    [...volumeBank.children].forEach((segment, index) => {
      segment.classList.toggle('on', (index + 1) / volumeBank.children.length <= value + .001);
    });
  }

  function adjacentItem(direction) {
    if (!selected || !items.length) return null;
    const index = items.findIndex((item) => item.id === selected.id);
    if (index < 0) return null;
    const nextIndex = index + direction;
    return nextIndex >= 0 && nextIndex < items.length ? items[nextIndex] : null;
  }

  function skipBy(seconds) {
    if (!Number.isFinite(player.duration)) return;
    player.currentTime = Math.max(0, Math.min(player.duration, player.currentTime + seconds));
  }

  document.querySelectorAll('.rail-button').forEach((button) => {
    button.addEventListener('click', () => setView(button.dataset.view));
  });

  window.addEventListener('popstate', () => setView(viewFromHash(), false));
  search.addEventListener('input', renderLibrary);
  moviesFilter.addEventListener('click', () => setLibraryFilter('movies'));
  tvFilter.addEventListener('click', () => setLibraryFilter('tv'));
  gridViewButton.addEventListener('click', () => setLibraryMode('grid'));
  listViewButton.addEventListener('click', () => setLibraryMode('list'));
  refreshButton.addEventListener('click', async () => { await loadHealth(); await loadCatalog(); });

  playPause.addEventListener('click', async () => {
    if (!selected) return;
    if (player.paused) { try { await player.play(); } catch (_) {} } else player.pause();
  });
  back30.addEventListener('click', () => skipBy(-30));
  forward30.addEventListener('click', () => skipBy(30));
  prevItem.addEventListener('click', () => { const item = adjacentItem(-1); if (item) playItem(item); });
  nextItem.addEventListener('click', () => { const item = adjacentItem(1); if (item) playItem(item); });
  compatButton.addEventListener('click', () => { if (selected) playItem(selected, !usingCompatibility, player.currentTime || 0); });
  fullscreenButton.addEventListener('click', async () => {
    const frame = document.querySelector('.video-frame');
    try {
      if (document.fullscreenElement) await document.exitFullscreen();
      else if (frame.requestFullscreen) await frame.requestFullscreen();
      else if (player.webkitEnterFullscreen) player.webkitEnterFullscreen();
    } catch (_) {}
  });
  seekControl.addEventListener('input', () => {
    if (!Number.isFinite(player.duration) || player.duration <= 0) return;
    player.currentTime = Number(seekControl.value) / 1000 * player.duration;
  });

  player.addEventListener('loadedmetadata', updatePlayerControls);
  player.addEventListener('durationchange', updatePlayerControls);
  player.addEventListener('timeupdate', () => { updatePlayerControls(); saveResume(false); });
  player.addEventListener('volumechange', updateVolumeBank);
  player.addEventListener('play', () => { playPause.textContent = 'Ⅱ'; setPlayerStatus(usingCompatibility ? 'PLAYING • COMPATIBILITY' : 'PLAYING • DIRECT'); });
  player.addEventListener('pause', () => { playPause.textContent = '▶'; saveResume(true); if (!player.ended) setPlayerStatus('PAUSED'); });
  player.addEventListener('waiting', () => setPlayerStatus('BUFFERING'));
  player.addEventListener('ended', () => { saveResume(true); setPlayerStatus('PLAYBACK COMPLETE'); updatePlayerControls(); });
  player.addEventListener('error', () => {
    if (selected && autoFallbackArmed) {
      const startAt = Number.isFinite(player.currentTime) ? player.currentTime : 0;
      autoFallbackArmed = false;
      setPlayerStatus('DIRECT PLAY UNSUPPORTED • SWITCHING TO COMPATIBILITY');
      window.setTimeout(() => playItem(selected, true, startAt), 150);
      return;
    }
    setPlayerStatus('PLAYBACK ERROR • TRY COMPAT');
  });

  document.addEventListener('keydown', (event) => {
    if (event.target && /INPUT|TEXTAREA/.test(event.target.tagName)) return;
    if (event.code === 'Space' && currentView === 'player' && selected) {
      event.preventDefault();
      playPause.click();
    } else if (event.key === 'ArrowLeft' && currentView === 'player') {
      skipBy(-10);
    } else if (event.key === 'ArrowRight' && currentView === 'player') {
      skipBy(10);
    } else if ((event.key === 'f' || event.key === 'F') && currentView === 'player') {
      fullscreenButton.click();
    } else if (event.key === 'Escape' && !document.fullscreenElement) {
      setView('home');
    }
  });

  window.addEventListener('beforeunload', () => saveResume(true));

  async function initialLoad() {
    createVolumeBank();
    setView(viewFromHash(), false);
    await loadHealth();
    await loadCatalog();
    if (!items.length) {
      window.setTimeout(async () => { await loadHealth(); await loadCatalog(); }, 1500);
    }
  }

  if ('serviceWorker' in navigator && location.protocol === 'https:') {
    navigator.serviceWorker.register('./sw.js').then((registration) => registration.update()).catch(() => {});
  }

  initialLoad();
})();
