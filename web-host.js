(() => {
  'use strict';
  const KEY = 'nougat-web-host-v1';
  const OLD_WATCH_KEY = 'nougat-watch-server-v1';
  const config = window.NOUGAT_WEB_PLAYER || {};
  const initial = String(config.baseUrl || '').trim().replace(/\/+$/, '');
  function validate(value) {
    try {
      const parsed = new URL(String(value || '').trim());
      // Do not store credentials, URL paths, queries or hashes in the
      // browser. Public GitHub Pages needs HTTPS, not mixed-content HTTP.
      if (parsed.protocol !== 'https:' || parsed.username || parsed.password ||
          parsed.pathname !== '/' || parsed.search || parsed.hash ||
          !parsed.hostname || !parsed.hostname.includes('.')) return '';
      return parsed.origin;
    } catch (_) { return ''; }
  }
  function getSaved() {
    try {
      const current = validate(localStorage.getItem(KEY));
      if (current) return current;
      const prior = validate(localStorage.getItem(OLD_WATCH_KEY));
      if (prior) { localStorage.setItem(KEY, prior); return prior; }
    } catch (_) {}
    return '';
  }
  function getDefault() {
    return config.enabled === false ? '' : validate(initial);
  }
  function getBase() {
    if (config.enabled === false) return '';
    // Empty base is significant for the local same-origin HTTP/LAN viewer.
    if (location.protocol === 'http:' && !initial) return '';
    return getSaved() || getDefault();
  }
  function announce() {
    window.dispatchEvent(new CustomEvent('nougat:host-changed', {
      detail: { baseUrl: getBase() }
    }));
  }
  function setServer(value) {
    const base = validate(value);
    if (!base || config.enabled === false) return false;
    try { localStorage.setItem(KEY, base); }
    catch (_) { return false; }
    announce();
    return true;
  }
  function reset() {
    try {
      localStorage.removeItem(KEY);
      localStorage.removeItem(OLD_WATCH_KEY);
    } catch (_) { return false; }
    announce();
    return true;
  }
  window.addEventListener('storage', (event) => {
    if ([KEY, OLD_WATCH_KEY].includes(event.key)) announce();
  });
  window.NougatWebHost = Object.freeze({
    key: KEY, validate, getSaved, getBase, getDefault, setServer, reset
  });
})();
