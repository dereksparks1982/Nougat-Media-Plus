// Static GitHub Pages app shell only. Never cache personal media catalogs,
// artwork, video, stream responses or cross-origin media host traffic.
const CACHE = 'nougat-web-player-v44-resume-host-offline-20261030';
const SHELL = [
  './legal-notice.css',
  './legal-notice.js',
  './LEGAL_NOTICE.md',
  './',
  './index.html',
  './styles.css',
  './modules.css',
  './modules-live-tv.css',
  './modules-scrollbars.css',
  './modules-media-card-actions.css',
  './library-polish.css',
  './player-activity.css',
  './player-chapters.css',
  './player-tracks.css',
  './player-context.css',
  './player-mobile.css',
  './enhancements.css',
  './config.js',
  './web-host.js',
  './web-resume.js',
  './web-api.js',
  './web-host-panel.js',
  './web-host-panel.css',
  './web-capabilities.js',
  './web-capabilities.css',
  './modules-catalog-cache.js',
  './app.js',
  './connection-monitor.js',
  './player-enhancements.js',
  './player-chapters.js',
  './player-tracks.js',
  './player-context.js',
  './player-input.js',
  './player-resume.js',
  './player-media-session.js',
  './modules.js',
  './modules-broadcast.js',
  './modules-world-tv-catalog.js',
  './modules-world-tv-guide.js',
  './modules-radio.js',
  './modules-radio-modes.js',
  './modules-radio-cellular.js',
  './modules-live-tv.js',
  './modules-live-tv-auto-refresh.js',
  './modules-studio-games.js',
  './modules-studio-extra.js',
  './modules-games-extra.js',
  './modules-network-satellite.js',
  './modules-system.js',
  './modules-system-extra.js',
  './modules-system-tuner.js',
  './modules-system-tmdb.js',
  './modules-search-discover.js',
  './modules-p2p-network.js',
  './modules-stream.js',
  './modules-bridge.js',
  './modules-media-card-actions.js',
  './manifest.webmanifest',
  './watch/',
  './watch/index.html',
  './watch/watch.css',
  './watch/watch.js',
  './watch/manifest.webmanifest',
  './assets/branding/nougat-media-plus-dock-N.png',
  './assets/branding/nougat-media-plus-master-N.png',
  './assets/branding/nougat-media-plus-topbar-lockup.png'
];
const CORE = new Set([
  './index.html', './watch/index.html', './app.js',
  './watch/watch.js', './legal-notice.js', './web-host.js',
  './web-resume.js'
]);
const ROOT = new URL(self.registration.scope);
const ALLOWED = new Set(SHELL.map(path => new URL(path, ROOT).pathname));
const STATIC_TYPES = new Set(['document','script','style','image','manifest','font','']);
function staticRequest(request) {
  const url=new URL(request.url);
  if (request.method !== 'GET' || url.origin !== ROOT.origin ||
      !url.pathname.startsWith(ROOT.pathname) || url.search ||
      !ALLOWED.has(url.pathname)) return false;
  if (url.pathname.includes('/nougat/v1/') ||
      url.pathname.includes('/api/') ||
      request.headers.get('range') ||
      request.cache === 'no-store') return false;
  return STATIC_TYPES.has(request.destination);
}
self.addEventListener('install', event => {
  event.waitUntil((async () => {
    const cache=await caches.open(CACHE);
    const results=await Promise.allSettled(SHELL.map(async path => {
      // An unavailable optional asset must not make the whole SW installation
      // fail, keeping phones on an older build indefinitely.
      const url=new URL(path,ROOT);
      const response=await fetch(url.href,{
        cache:'reload',credentials:'omit',redirect:'error'
      });
      if (!response.ok || response.type !== 'basic')
        throw new Error('Unavailable shell asset: ' + path);
      await cache.put(url.href,response);
      return path;
    }));
    const cachedCore=results.filter((outcome,i) =>
      outcome.status==='fulfilled' && CORE.has(SHELL[i])).length;
    if (cachedCore < CORE.size)
      throw new Error('Essential Nougat web files were not cached.');
    await self.skipWaiting();
  })());
});
self.addEventListener('activate', event => {
  event.waitUntil((async () => {
    const keys=await caches.keys();
    await Promise.all(keys.filter(key =>
      key.startsWith('nougat-web-player-') && key!==CACHE
    ).map(key=>caches.delete(key)));
    await self.clients.claim();
  })());
});
self.addEventListener('fetch', event => {
  const request=event.request;
  if (!staticRequest(request)) return;
  event.respondWith((async () => {
    const cache=await caches.open(CACHE);
    const lookup=new URL(request.url);
    const key=lookup.origin+lookup.pathname;
    try {
      const response=await fetch(request);
      if (response.ok && response.type==='basic') {
        await cache.put(key,response.clone()).catch(()=>{});
      }
      return response;
    } catch (_) {
      const offline=await cache.match(key);
      if (offline) return offline;
      if (request.mode==='navigate') {
        const shell=lookup.pathname.startsWith(new URL('./watch/',ROOT).pathname)
          ? './watch/index.html' : './index.html';
        const cached=await cache.match(new URL(shell,ROOT).href);
        if (cached) return cached;
      }
      return Response.error();
    }
  })());
});
