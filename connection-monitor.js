(() => {
  'use strict';
  // Single health-monitor source of truth: app.js performs the bounded
  // media-host checks. This listener must not request a different hardcoded
  // server, bypass the legal gate, or trigger overlapping catalog fetches.
  const refresh=document.getElementById('refreshButton');
  let lastOnline=null;
  let lastRefresh=0;
  let lastServer=null;
  const COOLDOWN=15000;
  window.addEventListener('nougat:health-state', (event) => {
    if (window.NougatLegalAccepted !== true) return;
    const {state,server}=event.detail||{};
    if (server !== lastServer) {
      lastServer=server;
      lastOnline=null;
    }
    if (state === 'offline') { lastOnline=false; return; }
    if (state !== 'online') return;
    const recovered=lastOnline===false;
    lastOnline=true;
    const now=Date.now();
    if (recovered && now-lastRefresh>=COOLDOWN && refresh && !refresh.disabled) {
      lastRefresh=now;
      refresh.click();
    }
  });
  window.addEventListener('nougat:host-changed', () => {
    lastOnline=null;
    lastServer=null;
  });
})();
