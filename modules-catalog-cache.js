(() => {
  'use strict';
  // An offline fallback for ONE media server at a time. Never let a catalog
  // fetched from server A appear in server B, or send duplicate background
  // requests that race the current host setting.
  const PREFIX='nougat-web-catalog-cache-v2:';
  const MAX_AGE=24*60*60*1000;
  const MAX_ITEMS=3000;
  const MAX_BYTES=2*1024*1024;
  const networkFetch=window.fetch.bind(window);

  function catalogUrl(input,init={}) {
    try {
      const url=new URL(typeof input==='string'?input:input.url,location.href);
      const method=String(init.method||(typeof input==='object'&&input.method)||'GET').toUpperCase();
      if (method!=='GET' || !['http:','https:'].includes(url.protocol) ||
          url.pathname.slice(-18)!=='/nougat/v1/catalog') return null;
      return url;
    } catch (_) { return null; }
  }
  function cacheKey(url) { return PREFIX + encodeURIComponent(url.origin + url.pathname); }
  function read(url) {
    try {
      const data=JSON.parse(localStorage.getItem(cacheKey(url))||'null');
      if (!data || !Array.isArray(data.items) || !Number.isFinite(data.savedAt) ||
          data.savedAt > Date.now() || Date.now()-data.savedAt > MAX_AGE)
        return null;
      return data.items.slice(0,MAX_ITEMS);
    } catch (_) { return null; }
  }
  function store(url,body) {
    if (!body || body.ok!==true || !Array.isArray(body.items)) return;
    const items=body.items.slice(0,MAX_ITEMS);
    const raw=JSON.stringify({savedAt:Date.now(),items});
    if (raw.length > MAX_BYTES) return;
    try { localStorage.setItem(cacheKey(url),raw); } catch (_) {}
  }
  window.fetch=async function(input,init={}) {
    const url=catalogUrl(input,init);
    if (!url || window.NougatLegalAccepted !== true) return networkFetch(input,init);
    try {
      const response=await networkFetch(input,init);
      if (response.ok) {
        // The response body belongs to the caller; inspect its clone only.
        void response.clone().json().then(body=>store(url,body)).catch(()=>{});
      }
      return response;
    } catch (error) {
      // Deliberate cancel/timeouts must NOT be disguised as success.
      if (init.signal?.aborted || error?.name==='AbortError') throw error;
      const items=read(url);
      if (!items) throw error;
      return new Response(JSON.stringify({
        ok:true,items,__nougatOfflineCatalog:true
      }),{status:200,headers:{
        'Content-Type':'application/json','X-Nougat-Catalog-Cache':'offline'
      }});
    }
  };
  // Clear only in-memory state on host changes; persistent per-host caches are
  // independent and bounded, and will never be returned for another origin.
})();
