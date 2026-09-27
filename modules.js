(() => {
  'use strict';

  let host = null;
  let activeModule = '';
  let discoverMode = 'usual';
  let discoverKind = 'movie';
  let discoverPick = null;
  let searchPanel = 'search';
  let streamPlatform = 'YouTube';
  const streamPlatforms = ['YouTube', 'Vimeo', 'Rumble', 'RuTube', 'VK', 'OK'];

  const esc = (value) => String(value ?? '').replace(/[&<>"']/g, (c) => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));

  function items() { return host && typeof host.getItems === 'function' ? host.getItems() : []; }
  function root() { return document.getElementById('moduleView'); }
  function workspace() {
    const container = root();
    if (!container) return null;
    container.innerHTML = '<div class="module-workspace" id="moduleWorkspace"></div>';
    return document.getElementById('moduleWorkspace');
  }
  function button(label, id, active = false, extra = '') {
    return `<button type="button" class="sheet-button${active ? ' active-tool' : ''}" id="${id}" ${extra}>${esc(label)}</button>`;
  }
  function bind(id, fn) { const el = document.getElementById(id); if (el) el.addEventListener('click', fn); }
  function setStatus(text) { const el = document.getElementById('moduleStatus'); if (el) el.textContent = text; }

  function chooseDiscover() {
    const pool = items().filter((item) => discoverKind === 'movie' ? item.type === 'Movie' : item.type === 'Episode');
    if (!pool.length) { discoverPick = null; return; }
    if (discoverMode === 'random') {
      discoverPick = pool[Math.floor(Math.random() * pool.length)];
      return;
    }
    const resume = host && typeof host.getResume === 'function' ? host.getResume : () => null;
    const unwatched = pool.filter((item) => {
      const r = resume(item);
      return !r || Number(r.position_ms || 0) < 10000;
    });
    const source = unwatched.length ? unwatched : pool;
    discoverPick = source[Math.floor(Math.random() * source.length)];
  }

  function renderDiscover() {
    const w = workspace(); if (!w) return;
    if (!discoverPick || !items().some((item) => item.id === discoverPick.id)) chooseDiscover();
    w.innerHTML = `
      <div class="module-toolbar">
        ${button('Usual','discoverUsual',discoverMode==='usual')}
        ${button('Random','discoverRandom',discoverMode==='random')}
        ${button('Local Movie','discoverMovie',discoverKind==='movie')}
        ${button('Local TV','discoverTv',discoverKind==='tv')}
        ${button('Live TV','discoverLiveTv')}
        ${button('External Movie','discoverExternalMovie')}
        ${button('External TV','discoverExternalTv')}
        ${button('My Services','discoverServices')}
      </div>
      <h2 class="module-heading">DISCOVER</h2>
      <p class="module-copy">Standalone-style recommendation surface. Local Movie and Local TV are connected to the live Nougat catalog in this browser.</p>
      <div id="discoverBody"></div>
      <div class="status-line" id="moduleStatus"></div>`;
    const body = document.getElementById('discoverBody');
    if (!discoverPick) {
      body.innerHTML = '<div class="module-empty">No matching local media is indexed.</div>';
    } else {
      body.innerHTML = `<div class="discover-result">
        <div class="discover-poster">${discoverPick.type === 'Episode' ? 'TV' : 'MOVIE'}</div>
        <div class="discover-info"><span class="module-badge">${esc(discoverMode.toUpperCase())}</span>
          <h3>${esc(discoverPick.name)}</h3><p>${esc(discoverPick.type)}${discoverPick.year ? ` • ${discoverPick.year}` : ''}</p>
          <p>This recommendation comes directly from the media currently exposed by the Nougat server.</p>
          <div class="discover-actions">${button('Another','discoverAnother')}${button('Watch','discoverWatch')}${button('Open Library','discoverLibrary')}</div>
        </div></div>`;
    }
    bind('discoverUsual', () => { discoverMode='usual'; chooseDiscover(); renderDiscover(); });
    bind('discoverRandom', () => { discoverMode='random'; chooseDiscover(); renderDiscover(); });
    bind('discoverMovie', () => { discoverKind='movie'; chooseDiscover(); renderDiscover(); });
    bind('discoverTv', () => { discoverKind='tv'; chooseDiscover(); renderDiscover(); });
    bind('discoverAnother', () => { chooseDiscover(); renderDiscover(); });
    bind('discoverWatch', () => { if (discoverPick && host.playItem) host.playItem(discoverPick); });
    bind('discoverLibrary', () => host.setView && host.setView('library'));
    bind('discoverLiveTv', () => host.setView && host.setView('livetv'));
    bind('discoverExternalMovie', () => { discoverKind='movie'; setStatus('External recommendation provider wiring is the next Discover slice. Local recommendation remains live.'); });
    bind('discoverExternalTv', () => { discoverKind='tv'; setStatus('External recommendation provider wiring is the next Discover slice. Local recommendation remains live.'); });
    bind('discoverServices', renderServices);
  }

  function renderServices() {
    const w = workspace(); if (!w) return;
    const services = ['Netflix','Prime Video','Hulu','Disney+','Max','Paramount+','Peacock','Apple TV+','Tubi','Pluto TV','Plex'];
    let selected = {};
    try { selected = JSON.parse(localStorage.getItem('nougat-web-services') || '{}') || {}; } catch (_) {}
    w.innerHTML = `<div class="module-toolbar">${button('Back to Discover','servicesBack')}</div>
      <h2 class="module-heading">MY STREAMING SERVICES - UNITED STATES</h2>
      <p class="module-copy">Choose the services you use. The selection is stored in this browser and is available to Discover.</p>
      <div class="module-panel-grid">${services.map((name,i)=>`<label class="module-panel-card"><h3><input type="checkbox" data-service="${esc(name)}" ${selected[name]?'checked':''}> ${esc(name)}</h3><p>Include ${esc(name)} when external provider recommendations are added.</p></label>`).join('')}</div>`;
    bind('servicesBack', renderDiscover);
    w.querySelectorAll('[data-service]').forEach((box) => box.addEventListener('change', () => {
      selected[box.dataset.service] = box.checked;
      try { localStorage.setItem('nougat-web-services', JSON.stringify(selected)); } catch (_) {}
    }));
  }

  function localSearch(term) {
    const q = String(term || '').trim().toLowerCase();
    if (!q) return [];
    return items().filter((item) => `${item.name} ${item.type} ${item.year || ''} ${item.filename || ''}`.toLowerCase().includes(q));
  }

  function renderSearch() {
    const w = workspace(); if (!w) return;
    w.innerHTML = `<div class="module-toolbar search-tabs">
      ${button('Search','searchTab',searchPanel==='search')}${button('Crawler','crawlerTab',searchPanel==='crawler')}${button('P2P','p2pTab',searchPanel==='p2p')}${button('Archive','archiveTab',searchPanel==='archive')}
    </div><div id="searchPane" class="search-pane"></div>`;
    bind('searchTab',()=>{searchPanel='search';renderSearch();});
    bind('crawlerTab',()=>{searchPanel='crawler';renderSearch();});
    bind('p2pTab',()=>{searchPanel='p2p';renderSearch();});
    bind('archiveTab',()=>{searchPanel='archive';renderSearch();});
    if (searchPanel === 'crawler') renderCrawlerPane();
    else if (searchPanel === 'p2p') renderP2pPane();
    else if (searchPanel === 'archive') renderArchivePane();
    else renderLocalSearchPane();
  }

  function renderLocalSearchPane() {
    const pane=document.getElementById('searchPane'); if(!pane)return;
    pane.innerHTML=`<h2 class="module-heading">SEARCH</h2><div class="module-field"><span>QUERY</span><input id="nougatModuleSearch" type="search" autocomplete="off"></div>
      <div class="module-toolbar">${button('Search','runModuleSearch')}${button('Raw','rawModuleSearch')}${button('Library','searchLibrary')}</div><div id="searchResults" class="module-result-list"></div>`;
    const input=document.getElementById('nougatModuleSearch');
    const run=()=>{
      const results=localSearch(input.value); const out=document.getElementById('searchResults');
      out.innerHTML=results.length?results.map((item)=>`<button type="button" class="module-result-row" data-media-id="${esc(item.id)}"><span>${esc(item.name)}</span><span>${esc(item.type)}</span><span>${item.year||''}</span></button>`).join(''):'<div class="module-empty">No local matches.</div>';
      out.querySelectorAll('[data-media-id]').forEach((row)=>row.addEventListener('click',()=>{const item=items().find((x)=>x.id===row.dataset.mediaId);if(item&&host.playItem)host.playItem(item);}));
    };
    bind('runModuleSearch',run); bind('rawModuleSearch',run); bind('searchLibrary',()=>host.setView&&host.setView('library'));
    input.addEventListener('keydown',(e)=>{if(e.key==='Enter')run();});
  }

  function renderCrawlerPane() {
    const pane=document.getElementById('searchPane'); if(!pane)return;
    pane.innerHTML=`<h2 class="module-heading">CRAWLER</h2><div class="module-field"><span>SEED URL</span><input id="crawlerSeed" type="url" placeholder="https://example.com"></div>
      <div class="module-toolbar">${button('Start Crawl','crawlStart')}${button('Same Domain','crawlDomain',true)}${button('Clear','crawlClear')}</div>
      <div class="module-output" id="crawlerOutput">Ready. Browser security allows crawling only URLs that permit cross-origin fetches.</div>`;
    bind('crawlClear',()=>{document.getElementById('crawlerOutput').textContent='Ready.';});
    bind('crawlStart',async()=>{
      const url=document.getElementById('crawlerSeed').value.trim(); const out=document.getElementById('crawlerOutput');
      if(!url){out.textContent='Enter a seed URL.';return;} out.textContent='Fetching '+url+' ...';
      try{const r=await fetch(url,{mode:'cors'});const text=await r.text();const base=new URL(url);const doc=new DOMParser().parseFromString(text,'text/html');const links=[...doc.querySelectorAll('a[href]')].map(a=>{try{return new URL(a.getAttribute('href'),base).href;}catch(_){return null;}}).filter(Boolean);const same=[...new Set(links.filter(h=>{try{return new URL(h).host===base.host;}catch(_){return false;}}))].slice(0,150);out.textContent=`Fetched ${r.status} ${r.statusText}\n${same.length} same-domain links\n\n${same.join('\n')}`;}catch(err){out.textContent='Crawler blocked or failed: '+(err.message||err);}
    });
  }

  function renderP2pPane() {
    const pane=document.getElementById('searchPane'); if(!pane)return;
    pane.innerHTML=`<h2 class="module-heading">P2P</h2><div class="module-field"><span>MAGNET</span><textarea id="magnetInput" placeholder="magnet:?xt=urn:btih:..."></textarea></div>
      <div class="module-toolbar">${button('Load Magnet','magnetLoad')}${button('Remove','magnetClear')}${button('Open Torrent','torrentOpen','', 'disabled')}</div>
      <div class="module-output" id="magnetOutput">Paste a magnet URI to inspect it. Native torrent transfer remains a standalone-engine function.</div>`;
    bind('magnetClear',()=>{document.getElementById('magnetInput').value='';document.getElementById('magnetOutput').textContent='Cleared.';});
    bind('magnetLoad',()=>{const value=document.getElementById('magnetInput').value.trim();const out=document.getElementById('magnetOutput');if(!value.startsWith('magnet:?')){out.textContent='That is not a magnet URI.';return;}const p=new URLSearchParams(value.slice(value.indexOf('?')+1));out.textContent=`Name: ${p.get('dn')||'Unknown'}\nInfo hash: ${(p.getAll('xt').find(x=>x.startsWith('urn:btih:'))||'').replace('urn:btih:','')||'Unknown'}\nTrackers: ${p.getAll('tr').length}\n\nMagnet parsed successfully.`;});
  }

  function renderArchivePane() {
    const pane=document.getElementById('searchPane'); if(!pane)return;
    pane.innerHTML=`<h2 class="module-heading">ARCHIVE</h2><div class="module-field"><span>SEARCH</span><input id="archiveQuery" type="search"></div>
      <div class="archive-links" id="archiveLinks"><a class="archive-link" href="https://archive.org" target="_blank" rel="noopener">Internet Archive</a><a class="archive-link" href="https://web.archive.org" target="_blank" rel="noopener">Wayback Machine</a></div>`;
    const q=document.getElementById('archiveQuery'); q.addEventListener('keydown',(e)=>{if(e.key==='Enter'){const term=q.value.trim();if(term)window.open('https://archive.org/search?query='+encodeURIComponent(term),'_blank','noopener');}});
  }

  function renderStream() {
    const w=workspace(); if(!w)return;
    w.innerHTML=`<div class="module-toolbar stream-platforms">${streamPlatforms.map((p,i)=>button(p,'streamP'+i,p===streamPlatform)).join('')}</div>
      <h2 class="module-heading">STREAM • ${esc(streamPlatform)}</h2>
      <div class="module-field"><span>URL</span><input id="streamUrl" type="url" placeholder="Paste a video or webpage URL"></div>
      <div class="module-field"><span>OUTPUT</span><input id="streamOutput" type="text" readonly value="Ready"></div>
      <div class="module-toolbar">${button('Direct Watch','streamWatch')}${button('Webpage','streamWeb')}${button('Download Link','streamDownload')}${button('Clear','streamClear')}</div>
      <div class="web-only-note">Direct Watch uses the browser player for URLs the browser can play. The standalone app's yt-dlp extraction is a native service and is not silently faked here.</div>`;
    streamPlatforms.forEach((p,i)=>bind('streamP'+i,()=>{streamPlatform=p;renderStream();}));
    bind('streamClear',()=>{document.getElementById('streamUrl').value='';document.getElementById('streamOutput').value='Ready';});
    bind('streamWeb',()=>{const u=document.getElementById('streamUrl').value.trim();if(u)window.open(u,'_blank','noopener');});
    bind('streamDownload',()=>{const u=document.getElementById('streamUrl').value.trim();if(!u)return;const a=document.createElement('a');a.href=u;a.download='';a.rel='noopener';a.target='_blank';a.click();document.getElementById('streamOutput').value='Download/open request sent to browser';});
    bind('streamWatch',()=>{const u=document.getElementById('streamUrl').value.trim();if(!u)return;if(host.playUrl){host.playUrl(u,`${streamPlatform} Stream`);document.getElementById('streamOutput').value='Sent to Nougat Player';}});
  }

  async function renderNetwork() {
    const w=workspace(); if(!w)return;
    w.innerHTML=`<div class="module-toolbar">${button('Overview','networkOverview',true)}${button('Connections','networkConnections')}${button('Devices','networkDevices')}${button('Security','networkSecurity')}${button('Diagnostics','networkDiagnostics')}${button('Logs','networkLogs')}</div>
      <h2 class="module-heading">NETWORK</h2><table class="diagnostic-table" id="networkTable"><tbody><tr><th>Nougat bridge</th><td>Checking...</td></tr></tbody></table><div class="module-toolbar">${button('Refresh','networkRefresh')}</div>`;
    bind('networkRefresh',renderNetwork);
    const table=document.getElementById('networkTable');
    try{const base=host.baseUrl?host.baseUrl():'';const start=performance.now();const r=await fetch(base+'/nougat/v1/health',{cache:'no-store'});const data=await r.json();const ms=Math.round(performance.now()-start);table.innerHTML=`<tbody><tr><th>Bridge</th><td>${r.ok?'Online':'Error'}</td></tr><tr><th>Endpoint</th><td>${esc(base||'same origin')}</td></tr><tr><th>Latency</th><td>${ms} ms</td></tr><tr><th>Service</th><td>${esc(data.service||'Nougat Web Player')}</td></tr><tr><th>Version</th><td>${esc(data.version||'Unknown')}</td></tr><tr><th>Browser online</th><td>${navigator.onLine?'Yes':'No'}</td></tr></tbody>`;}catch(err){table.innerHTML=`<tbody><tr><th>Bridge</th><td>Offline</td></tr><tr><th>Error</th><td>${esc(err.message||err)}</td></tr><tr><th>Browser online</th><td>${navigator.onLine?'Yes':'No'}</td></tr></tbody>`;}
  }

  async function renderSystem() {
    const w=workspace(); if(!w)return;
    let storage='Unavailable'; try{if(navigator.storage&&navigator.storage.estimate){const s=await navigator.storage.estimate();storage=`${Math.round((s.usage||0)/1048576)} MB / ${Math.round((s.quota||0)/1048576)} MB`;}}catch(_){}
    w.innerHTML=`<div class="module-toolbar">${button('Diagnostics','systemDiag',true)}${button('Refresh Server','systemRefresh')}${button('Clear Web Cache','systemClearCache')}</div>
      <h2 class="module-heading">SYSTEM</h2><table class="diagnostic-table"><tbody><tr><th>Browser</th><td>${esc(navigator.userAgent)}</td></tr><tr><th>Platform</th><td>${esc(navigator.platform||'Unknown')}</td></tr><tr><th>CPU threads</th><td>${navigator.hardwareConcurrency||'Unknown'}</td></tr><tr><th>Memory</th><td>${navigator.deviceMemory?navigator.deviceMemory+' GB':'Not reported'}</td></tr><tr><th>Web storage</th><td>${esc(storage)}</td></tr><tr><th>Service worker</th><td>${'serviceWorker' in navigator?'Supported':'Unavailable'}</td></tr><tr><th>Fullscreen</th><td>${document.fullscreenEnabled?'Supported':'Browser-managed'}</td></tr></tbody></table><div class="module-output" id="systemOutput">Web diagnostics ready.</div>`;
    bind('systemRefresh',async()=>{document.getElementById('systemOutput').textContent='Refreshing Nougat server state...';if(host.refresh){await host.refresh();document.getElementById('systemOutput').textContent='Nougat health and catalog refreshed.';}});
    bind('systemClearCache',async()=>{const out=document.getElementById('systemOutput');try{const keys=await caches.keys();await Promise.all(keys.map(k=>caches.delete(k)));out.textContent=`Cleared ${keys.length} Nougat web cache(s). Reloading shell...`;setTimeout(()=>location.reload(),350);}catch(err){out.textContent='Cache clear failed: '+(err.message||err);}});
  }

  function renderPending(name) {
    const w=workspace(); if(!w)return;
    w.innerHTML=`<h2 class="module-heading">${esc(name.toUpperCase())}</h2><div class="module-output">This module is being pulled from the standalone app piece by piece. Its rail button is wired to the correct module surface rather than being removed.</div>`;
  }

  function activate(name, context) {
    host=context||host; activeModule=name;
    if(name==='discover')renderDiscover();
    else if(name==='search')renderSearch();
    else if(name==='stream')renderStream();
    else if(name==='network')renderNetwork();
    else if(name==='system')renderSystem();
    else renderPending(name);
  }

  window.NougatWebModules={activate};
})();