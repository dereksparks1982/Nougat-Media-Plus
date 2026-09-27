const CACHE = 'nougat-web-player-v9';
const SHELL = [
  './',
  './index.html',
  './styles.css',
  './modules.css',
  './enhancements.css',
  './config.js',
  './app.js',
  './player-enhancements.js',
  './modules.js',
  './modules-broadcast.js',
  './modules-world-tv-catalog.js',
  './modules-radio.js',
  './modules-live-tv.js',
  './modules-studio-games.js',
  './modules-network-satellite.js',
  './modules-system.js',
  './modules-search-discover.js',
  './modules-stream.js',
  './modules-bridge.js',
  './manifest.webmanifest',
  './assets/branding/nougat-media-plus-dock-N.png',
  './assets/branding/nougat-media-plus-topbar-lockup.png'
];

self.addEventListener('install', (event) => {
  event.waitUntil(caches.open(CACHE).then((cache) => cache.addAll(SHELL)));
  self.skipWaiting();
});

self.addEventListener('activate', (event) => {
  event.waitUntil(
    caches.keys().then((keys) => Promise.all(
      keys.filter((key) => key !== CACHE).map((key) => caches.delete(key))
    ))
  );
  self.clients.claim();
});

self.addEventListener('fetch', (event) => {
  const request = event.request;
  if (request.method !== 'GET') return;
  const url = new URL(request.url);
  if (url.pathname.startsWith('/nougat/v1/')) return;
  if (url.origin !== self.location.origin) return;
  event.respondWith(
    fetch(request).then((response) => {
      if (response && response.ok && response.type === 'basic') {
        const copy = response.clone();
        caches.open(CACHE).then((cache) => cache.put(request, copy));
      }
      return response;
    }).catch(() => caches.match(request))
  );
});
