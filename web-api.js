(() => {
  'use strict';
  const DEFAULT_TIMEOUT = 9000;
  async function request(url, init = {}) {
    const controller = new AbortController();
    const ms = Math.max(1000, Math.min(30000, Number(init.timeout) || DEFAULT_TIMEOUT));
    const timer = setTimeout(() => controller.abort(), ms);
    let unsubscribe = null;
    if (init.signal) {
      if (init.signal.aborted) controller.abort();
      else {
        const callback = () => controller.abort();
        init.signal.addEventListener('abort', callback, { once: true });
        unsubscribe = () => init.signal.removeEventListener('abort', callback);
      }
    }
    try {
      const { timeout, ...options } = init;
      // Local LAN terms cookies are same-origin only; never transmit them
      // to a remotely configured GitHub Pages HTTPS bridge.
      const sameOrigin = new URL(url, location.href).origin === location.origin;
      return await fetch(url, {
        cache: 'no-store',
        mode: 'cors',
        credentials: sameOrigin ? 'same-origin' : 'omit',
        redirect: 'error',
        ...options,
        signal: controller.signal
      });
    } catch (error) {
      if (error?.name === 'AbortError') {
        throw new Error('Request timed out or was cancelled.');
      }
      throw error;
    } finally {
      clearTimeout(timer);
      if (unsubscribe) unsubscribe();
    }
  }
  async function json(url, init = {}) {
    // Keep the deadline active while reading the JSON response body too.
    const controller = new AbortController();
    const ms = Math.max(1000, Math.min(30000, Number(init.timeout) || DEFAULT_TIMEOUT));
    const timer = setTimeout(() => controller.abort(), ms);
    try {
      const response = await request(url, { ...init, signal: controller.signal });
      if (!response.ok) throw new Error('HTTP ' + response.status);
      const data = await response.json();
      if (data?.ok === false) throw new Error(data.error || 'Nougat service unavailable.');
      return data;
    } finally {
      clearTimeout(timer);
    }
  }
  window.NougatWebAPI = Object.freeze({ request, json });
})();
