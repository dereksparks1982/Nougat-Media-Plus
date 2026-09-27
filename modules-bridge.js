(() => {
  'use strict';

  let catalog = [];
  let catalogPromise = null;

  const baseUrl = () => {
    const cfg = window.NOUGAT_WEB_PLAYER || {};
    return String(cfg.baseUrl || '').trim().replace(/\/+$/, '');
  };

  const normalize = (raw) => {
    const kind = String(raw.kind || raw.type || '').toLowerCase();
    const episode = kind === 'tv' || kind === 'episode' || kind === 'television';
    return {
      id: String(raw.id || ''),
      name: String(raw.name || raw.title || raw.filename || 'Untitled media'),
      filename: String(raw.filename || raw.path || ''),
      type: episode ? 'Episode' : 'Movie',
      year: Number(raw.year || raw.production_year) || 0,
      ready: raw.ready !== false,
      directPreferred: typeof raw.directPreferred === 'boolean' ? raw.directPreferred : true
    };
  };

  async function ensureCatalog(force = false) {
    if (catalogPromise && !force) return catalogPromise;
    catalogPromise = (async () => {
      try {
        const response = await fetch(`${baseUrl()}/nougat/v1/catalog`, { cache: 'no-store' });
        const data = await response.json();
        if (!response.ok || !data.ok) throw new Error(data.error || 'Catalog unavailable');
        catalog = (Array.isArray(data.items) ? data.items : []).map(normalize).filter((item) => item.id);
      } catch (_) {
        catalog = [];
      }
      return catalog;
    })();
    return catalogPromise;
  }

  function getResume(item) {
    try {
      const state = JSON.parse(localStorage.getItem('nougat-web-resume-v2') || '{}');
      return state && item ? state[item.id] || null : null;
    } catch (_) {
      return null;
    }
  }

  function rail(view) {
    return document.querySelector(`.rail-button[data-view="${CSS.escape(view)}"]`);
  }

  function navigate(view) {
    const button = rail(view);
    if (button) button.click();
  }

  function findCard(item) {
    if (!item) return null;
    const wanted = `Play ${item.name}`;
    return [...document.querySelectorAll('.media-card')].find((button) => button.getAttribute('aria-label') === wanted) || null;
  }

  function playItem(item) {
    const card = findCard(item);
    if (card) {
      const video = document.getElementById('player');
      if (video) video.controls = false;
      card.click();
      return;
    }
    navigate('library');
    const search = document.getElementById('search');
    if (search) {
      search.value = item ? item.name : '';
      search.dispatchEvent(new Event('input', { bubbles: true }));
    }
  }

  function playUrl(url, label) {
    const player = document.getElementById('player');
    const empty = document.getElementById('emptyPlayer');
    const title = document.getElementById('playerTitleHud');
    const status = document.getElementById('playerStatus');
    if (!player) return;
    player.controls = true;
    player.src = url;
    if (empty) empty.hidden = true;
    if (title) { title.hidden = false; title.textContent = label || 'Stream'; }
    if (status) status.textContent = 'DIRECT WEB STREAM';
    navigate('player');
    player.load();
    const result = player.play();
    if (result && typeof result.catch === 'function') result.catch(() => {
      if (status) status.textContent = 'READY • PRESS PLAY';
    });
  }

  async function refresh() {
    const button = document.getElementById('refreshButton');
    if (button) button.click();
    await ensureCatalog(true);
  }

  const host = {
    getItems: () => catalog.slice(),
    getResume,
    playItem,
    playUrl,
    setView: navigate,
    refresh,
    baseUrl
  };

  async function activate(view) {
    if (!window.NougatWebModules || !view || ['home','player','library'].includes(view)) return;
    await ensureCatalog(false);
    window.NougatWebModules.activate(view, host);
  }

  document.querySelectorAll('.rail-button').forEach((button) => {
    button.addEventListener('click', () => {
      const view = button.dataset.view;
      if (!['home','player','library'].includes(view)) queueMicrotask(() => activate(view));
    });
  });

  window.addEventListener('popstate', () => {
    const view = location.hash.replace(/^#/, '').trim().toLowerCase();
    if (view && !['home','player','library'].includes(view)) setTimeout(() => activate(view), 0);
  });

  ensureCatalog(false).then(() => {
    const initial = location.hash.replace(/^#/, '').trim().toLowerCase();
    if (initial && !['home','player','library'].includes(initial)) activate(initial);
  });
})();