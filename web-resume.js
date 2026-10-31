(() => {
  'use strict';
  // Browser-local watch positions shared by the full player and Quick Watch.
  // Scoped to one media-server origin: identical catalog IDs on two servers
  // must never inherit one another's playback positions.
  const PREFIX = 'nougat-web-resume-v3:';
  const LEGACY_FULL = 'nougat-web-resume-v2';
  const LEGACY_WATCH = 'nougat-watch-position-v1:';
  const LIMIT = 200;
  function key(host) { return PREFIX + encodeURIComponent(String(host || location.origin)); }
  function entry(raw) {
    if (!raw || typeof raw !== 'object') return null;
    const position = Number(raw.position_ms);
    const duration = Number(raw.duration_ms || 0);
    if (!Number.isFinite(position) || position < 0 || position > 604800000 ||
        !Number.isFinite(duration) || duration < 0 || duration > 604800000) return null;
    const updated = Number(raw.updated);
    return {
      position_ms: Math.round(position),
      duration_ms: Math.round(duration),
      updated: Number.isFinite(updated) && updated > 0 ? updated : Date.now(),
      completed: raw.completed === true,
      title: String(raw.title || '').slice(0, 200)
    };
  }
  function compact(data) {
    const list = [];
    for (const [id,raw] of Object.entries(data && typeof data === 'object' ? data : {})) {
      if (!id || id.length > 512 || id === '__proto__' || id === 'constructor') continue;
      const normalized = entry(raw);
      if (normalized) list.push([id,normalized]);
    }
    list.sort((a,b) => b[1].updated - a[1].updated);
    const state = Object.create(null);
    for (const [id,record] of list.slice(0,LIMIT)) state[id] = record;
    return state;
  }
  function parse(value) {
    try { const object=JSON.parse(value || '{}'); return compact(object); }
    catch (_) { return Object.create(null); }
  }
  function getRaw(storageKey) {
    try { return localStorage.getItem(storageKey); } catch (_) { return null; }
  }
  function write(storageKey, state) {
    try {
      localStorage.setItem(storageKey, JSON.stringify(state));
      return true;
    } catch (_) { return false; }
  }
  function migrate(host) {
    const watch = getRaw(LEGACY_WATCH + host);
    const existing = parse(watch);
    const converted = Object.create(null);
    for (const [id,record] of Object.entries(existing)) {
      // Prior Quick Watch records used seconds instead of milliseconds.
      if (record.position_ms >= 0) converted[id] = record;
    }
    if (watch) {
      try {
        const raw=JSON.parse(watch);
        for (const [id,record] of Object.entries(raw || {})) {
          const seconds=Number(record?.seconds);
          if (id.length > 512 || !Number.isFinite(seconds) || seconds <= 0) continue;
          const normalized=entry({
            position_ms: seconds * 1000, duration_ms: 0,
            updated: record.updated, completed: false
          });
          if (normalized) converted[id]=normalized;
        }
      } catch (_) {}
    }
    // The old full-player store was global. Import it ONLY into the original
    // configured host, never into an unrelated user-selected media server.
    const defaultHost = window.NougatWebHost?.getDefault() || '';
    if (host === defaultHost || (!host && !defaultHost)) {
      const legacy = parse(getRaw(LEGACY_FULL));
      for (const [id,record] of Object.entries(legacy))
        if (!converted[id] || record.updated > converted[id].updated) converted[id]=record;
    }
    const result = compact(converted);
    write(key(host), result);
    return result;
  }
  function read(host = '') {
    const storageKey=key(host);
    const saved=getRaw(storageKey);
    return saved === null ? migrate(host) : parse(saved);
  }
  function set(host, id, record) {
    id=String(id || '');
    if (!id || id.length > 512 || id === '__proto__' || id === 'constructor')
      return false;
    const normalized=entry(record);
    if (!normalized) return false;
    const data=read(host);
    data[id]=normalized;
    return write(key(host), compact(data));
  }
  window.addEventListener('storage', event => {
    if (event.key && event.key.startsWith(PREFIX))
      window.dispatchEvent(new CustomEvent('nougat:resume-changed', {
        detail: { storageKey: event.key }
      }));
  });
  window.NougatWebResume=Object.freeze({ key,read,set });
})();
