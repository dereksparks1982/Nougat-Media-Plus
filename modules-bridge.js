(() => {
  'use strict';

  let catalog = [];
  let catalogPromise = null;
  let cardArtworkObserver = null;
  let cardArtworkQueued = false;

  const baseUrl = () => {
    const cfg = window.NOUGAT_WEB_PLAYER || {};
    return String(cfg.baseUrl || '').trim().replace(/\/+$/, '');
  };

  const normalizePoster = (raw) => {
    const value = String(raw.poster || raw.posterUrl || raw.image || raw.imageUrl || '').trim();
    if (!value) {
      const id = String(raw.id || '').trim();
      return id ? `${baseUrl()}/nougat/v1/artwork?id=${encodeURIComponent(id)}` : '';
    }
    if (/^https?:\/\//i.test(value) || value.startsWith('data:') || value.startsWith('blob:')) return value;
    if (value.startsWith('/')) return `${baseUrl()}${value}`;
    return value;
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
      directPreferred: typeof raw.directPreferred === 'boolean' ? raw.directPreferred : true,
      poster: normalizePoster(raw)
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
    const cards=[...document.querySelectorAll('.media-card')];
    const byId=cards.find((button)=>String(button.dataset.nougatMediaId||'')===String(item.id||''));
    if(byId)return byId;
    const wanted = `Play ${item.name}`;
    return cards.find((button) => button.getAttribute('aria-label') === wanted) || null;
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
    queueMediaCardArtwork();
  }

  function hydrateDiscoverPoster() {
    const poster = document.querySelector('.discover-result .discover-poster');
    const heading = document.querySelector('.discover-result .discover-info h3');
    if (!poster || !heading) return;
    const item = catalog.find((entry) => entry.name === heading.textContent);
    if (!item || !item.poster) return;
    poster.textContent = '';
    const image = document.createElement('img');
    image.src = item.poster;
    image.alt = '';
    image.loading = 'lazy';
    image.style.width = '100%';
    image.style.height = '100%';
    image.style.objectFit = 'cover';
    image.addEventListener('error', () => {
      image.remove();
      poster.textContent = item.type === 'Episode' ? 'TV' : 'MOVIE';
    }, { once: true });
    poster.appendChild(image);
  }

  function hydrateMediaCardArtwork() {
    cardArtworkQueued = false;
    if (!catalog.length) return;
    const byName = new Map(catalog.map((item) => [item.name, item]));
    const byId = new Map(catalog.map((item) => [item.id, item]));
    document.querySelectorAll('.media-card').forEach((card) => {
      const label = String(card.getAttribute('aria-label') || '');
      const name = label.startsWith('Play ') ? label.slice(5) : '';
      const item = byId.get(String(card.dataset.nougatMediaId||'')) || byName.get(name);
      const art = card.querySelector('.media-art');
      if (!item || !item.poster || !art || art.querySelector('img') || art.dataset.nougatArtwork === item.poster) return;

      art.dataset.nougatArtwork = item.poster;
      art.textContent = '';
      const image = document.createElement('img');
      image.src = item.poster;
      image.alt = '';
      image.loading = 'lazy';
      image.addEventListener('error', () => {
        image.remove();
        art.textContent = '';
        const fallback = document.createElement('span');
        fallback.className = 'fallback-mark';
        fallback.textContent = item.type === 'Episode' ? 'TV' : 'MOVIE';
        art.appendChild(fallback);
      }, { once: true });
      art.appendChild(image);
    });
  }

  function queueMediaCardArtwork() {
    if (cardArtworkQueued) return;
    cardArtworkQueued = true;
    requestAnimationFrame(hydrateMediaCardArtwork);
  }

  function observeMediaCards() {
    if (cardArtworkObserver || !document.body) return;
    cardArtworkObserver = new MutationObserver(() => queueMediaCardArtwork());
    cardArtworkObserver.observe(document.body, { childList: true, subtree: true });
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
    if (view === 'discover') requestAnimationFrame(hydrateDiscoverPoster);
  }

  document.querySelectorAll('.rail-button').forEach((button) => {
    button.addEventListener('click', () => {
      const view = button.dataset.view;
      if (['home','library'].includes(view)) queueMicrotask(queueMediaCardArtwork);
      if (!['home','player','library'].includes(view)) queueMicrotask(() => activate(view));
    });
  });

  window.addEventListener('popstate', () => {
    const view = location.hash.replace(/^#/, '').trim().toLowerCase();
    if (['home','library'].includes(view)) setTimeout(queueMediaCardArtwork, 0);
    if (view && !['home','player','library'].includes(view)) setTimeout(() => activate(view), 0);
  });

  ensureCatalog(false).then(() => {
    observeMediaCards();
    queueMediaCardArtwork();
    const initial = location.hash.replace(/^#/, '').trim().toLowerCase();
    if (initial && !['home','player','library'].includes(initial)) activate(initial);
  });
})();