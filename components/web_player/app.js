(() => {
  'use strict';

  const player = document.getElementById('player');
  const emptyPlayer = document.getElementById('emptyPlayer');
  const nowTitle = document.getElementById('nowTitle');
  const nowMeta = document.getElementById('nowMeta');
  const playerStatus = document.getElementById('playerStatus');
  const healthBadge = document.getElementById('healthBadge');
  const catalogStatus = document.getElementById('catalogStatus');
  const libraryGrid = document.getElementById('libraryGrid');
  const search = document.getElementById('search');
  const refreshButton = document.getElementById('refreshButton');
  const compatButton = document.getElementById('compatButton');

  let items = [];
  let selected = null;
  let usingCompatibility = false;
  let autoFallbackArmed = false;
  let statusClearTimer = 0;

  const setPlayerStatus = (message, clearAfterMs = 0) => {
    window.clearTimeout(statusClearTimer);
    playerStatus.textContent = message;
    if (clearAfterMs > 0) {
      statusClearTimer = window.setTimeout(() => {
        playerStatus.textContent = '';
      }, clearAfterMs);
    }
  };

  const mediaMeta = (item) => {
    const parts = [];
    if (item.type) parts.push(item.type === 'Episode' ? 'TV Episode' : item.type);
    if (Number(item.year) > 0) parts.push(String(item.year));
    parts.push(item.directPreferred ? 'Direct play preferred' : 'Browser compatibility preferred');
    return parts.join(' · ');
  };

  const streamUrl = (item, compatibility) => {
    const route = compatibility ? '/nougat/v1/transcode' : '/nougat/v1/media';
    return `${route}?id=${encodeURIComponent(item.id)}`;
  };

  const playItem = (item, compatibility = !item.directPreferred) => {
    selected = item;
    usingCompatibility = compatibility;
    autoFallbackArmed = !compatibility;
    nowTitle.textContent = item.name;
    nowMeta.textContent = mediaMeta(item);
    emptyPlayer.hidden = true;
    emptyPlayer.setAttribute('aria-hidden', 'true');
    compatButton.disabled = false;
    compatButton.textContent = compatibility ? 'Try Direct Play' : 'Browser Compatibility';
    setPlayerStatus(compatibility
      ? 'Starting Nougat browser compatibility stream…'
      : 'Starting direct LAN playback…');
    player.src = streamUrl(item, compatibility);
    player.load();
    const promise = player.play();
    if (promise && typeof promise.catch === 'function') {
      promise.catch(() => {
        setPlayerStatus('Playback is ready. Press Play if your browser blocked autoplay.', 3500);
      });
    }
  };

  const renderLibrary = () => {
    const term = search.value.trim().toLowerCase();
    const filtered = term
      ? items.filter((item) => `${item.name} ${item.type} ${item.year}`.toLowerCase().includes(term))
      : items;
    libraryGrid.textContent = '';
    if (!filtered.length) {
      const empty = document.createElement('div');
      empty.className = 'empty-library';
      empty.textContent = items.length ? 'No library items match that search.' : 'No playable media is currently indexed.';
      libraryGrid.appendChild(empty);
      return;
    }
    for (const item of filtered) {
      const card = document.createElement('button');
      card.type = 'button';
      card.className = 'media-card';
      card.setAttribute('aria-label', `Play ${item.name}`);

      const art = document.createElement('span');
      art.className = 'art';
      art.textContent = item.type === 'Episode' ? 'TV' : 'MOVIE';

      const details = document.createElement('span');
      details.className = 'details';
      const title = document.createElement('span');
      title.className = 'title';
      title.textContent = item.name;
      const meta = document.createElement('span');
      meta.className = 'meta';
      meta.textContent = mediaMeta(item);
      details.append(title, meta);
      card.append(art, details);
      card.addEventListener('click', () => playItem(item));
      libraryGrid.appendChild(card);
    }
  };

  const loadHealth = async () => {
    try {
      const response = await fetch('/nougat/v1/health', { cache: 'no-store' });
      if (!response.ok) throw new Error('health');
      const health = await response.json();
      healthBadge.textContent = health.lanOnly ? 'Private LAN · Online' : 'Online';
    } catch (_) {
      healthBadge.textContent = 'Web Player unavailable';
    }
  };

  const loadCatalog = async () => {
    refreshButton.disabled = true;
    catalogStatus.textContent = 'Loading your Nougat Library…';
    try {
      const response = await fetch('/nougat/v1/catalog', { cache: 'no-store' });
      const payload = await response.json();
      if (!response.ok || !payload.ok) throw new Error(payload.error || 'Catalog unavailable');
      items = Array.isArray(payload.items) ? payload.items : [];
      catalogStatus.textContent = `${items.length} playable ${items.length === 1 ? 'item' : 'items'} available on this Nougat server.`;
      renderLibrary();
    } catch (error) {
      items = [];
      renderLibrary();
      catalogStatus.textContent = `Library unavailable: ${error.message || 'Nougat server is not ready.'}`;
    } finally {
      refreshButton.disabled = false;
    }
  };

  search.addEventListener('input', renderLibrary);
  refreshButton.addEventListener('click', loadCatalog);
  compatButton.addEventListener('click', () => {
    if (!selected) return;
    playItem(selected, !usingCompatibility);
  });

  player.addEventListener('playing', () => {
    emptyPlayer.hidden = true;
    emptyPlayer.setAttribute('aria-hidden', 'true');
    setPlayerStatus(usingCompatibility
      ? 'Playing through Nougat browser compatibility mode.'
      : 'Playing directly from Nougat over your LAN.', 1800);
  });
  player.addEventListener('waiting', () => {
    setPlayerStatus('Buffering from Nougat…');
  });
  player.addEventListener('error', () => {
    if (selected && autoFallbackArmed) {
      autoFallbackArmed = false;
      setPlayerStatus('Direct play was not supported by this browser. Switching to compatibility mode…');
      window.setTimeout(() => playItem(selected, true), 150);
      return;
    }
    setPlayerStatus('This browser could not play the selected item. Try Browser Compatibility or refresh the Library.');
  });

  const initialLoad = async () => {
    await loadHealth();
    await loadCatalog();
    if (!items.length) {
      window.setTimeout(async () => {
        await loadHealth();
        await loadCatalog();
      }, 1500);
    }
  };

  initialLoad();
})();
