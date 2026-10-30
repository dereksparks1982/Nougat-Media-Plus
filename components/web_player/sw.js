const CACHE = 'nougat-web-player-v1-legal-20261030-v2-lan-20261030';
const SHELL = [
  './legal-notice.css',
  './legal-notice.js',
  './LEGAL_NOTICE.md',
  './',
  './index.html',
  './styles.css',
  './config.js',
  './app.js',
  './manifest.webmanifest',
  './nougat-icon.png'
];

self.addEventListener('install', (event) => {
  // Optional icon must not invalidate the core app shell if an installation
  // omits artwork. The approved icon is served by the host when present.
  event.waitUntil(caches.open(CACHE).then((cache) =>
    cache.addAll(SHELL.filter((asset) => asset !== './nougat-icon.png'))
      .then(() => cache.add('./nougat-icon.png').catch(() => {}))
  ));
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
    caches.match(request).then((cached) => cached || fetch(request).then((response) => {
      if (response && response.ok && response.type === 'basic') {
        const copy = response.clone();
        caches.open(CACHE).then((cache) => cache.put(request, copy));
      }
      return response;
    }))
  );
});
