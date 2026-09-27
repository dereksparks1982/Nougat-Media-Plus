(() => {
  'use strict';
  if (!window.NougatWebModules) return;

  const previous = window.NougatWebModules.activate.bind(window.NougatWebModules);
  const esc = (v) => String(v ?? '').replace(/[&<>"']/g, (c) => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
  const root = () => document.getElementById('moduleView');
  const button = (label,id,active=false,extra='') => `<button type="button" class="sheet-button${active?' active-tool':''}" id="${id}" ${extra}>${esc(label)}</button>`;
  const bind = (id,fn) => { const el=document.getElementById(id); if(el) el.addEventListener('click',fn); };
  const shell = (html) => { const r=root(); if(!r)return null; r.innerHTML=`<div class="module-workspace">${html}</div>`; return r.querySelector('.module-workspace'); };

  let hostRef=null;
  let discoverMode='Usual';
  let discoverSource='Local Movie';
  let discoverPick=null;
  let searchPanel='Search';
  const searchPanels=['Search','Crawler','P2P','Archive'];

  function items(){return hostRef?.getItems?hostRef.getItems():[];}
  async function getJson(path){
    const base=hostRef?.baseUrl?hostRef.baseUrl():'';
    const response=await fetch(`${base}${path}`,{cache:'no-store'});
    let data={};try{data=await response.json();}catch(_){data={};}
    if(!response.ok)throw new Error(data.error||`HTTP ${response.status}`);
    return data;
  }
  function mediaPool(){
    const type=discoverSource==='Local TV'?'Episode':'Movie';return items().filter(x=>x.type===type);
  }
  function chooseLocal(){
    const pool=mediaPool();if(!pool.length){discoverPick=null;return;}
    if(discoverMode==='Random'){discoverPick=pool[Math.floor(Math.random()*pool.length)];return;}
    const unseen=pool.filter(x=>{const r=hostRef?.getResume?hostRef.getResume(x):null;return !r||Number(r.position_ms||0)<10000;});
    const source=unseen.length?unseen:pool;discoverPick=source[Math.floor(Math.random()*source.length)];
  }
  function poster(item){
    if(item?.poster)return `<img src="${esc(item.poster)}" alt="" loading="lazy">`;
    return `<span>${item?.type==='Episode'?'TV':'MOVIE'}</span>`;
  }
  function selectedServices(){
    try{return JSON.parse(localStorage.getItem('nougat-web-services')||'{}')||{};}catch(_){return {};}
  }

  function renderDiscover(host){
    hostRef=host;
    if(discoverSource==='Local Movie'||discoverSource==='Local TV'){if(!discoverPick||!items().some(x=>x.id===discoverPick.id)||discoverPick.type!==(discoverSource==='Local TV'?'Episode':'Movie'))chooseLocal();}
    shell(`<div class="module-toolbar">${button('Usual','dUsual',discoverMode==='Usual')}${button('Random','dRandom',discoverMode==='Random')}${button('Local Movie','dLocalMovie',discoverSource==='Local Movie')}${button('Local TV','dLocalTv',discoverSource==='Local TV')}${button('Live TV','dLiveTv')}${button('External Movie','dExternalMovie',discoverSource==='External Movie')}${button('External TV','dExternalTv',discoverSource==='External TV')}${button('TMDb Test','dTmdbTest')}${button('TMDb Replace','dTmdbReplace')}${button('TMDb Clear','dTmdbClear')}${button('My Services','dServices')}</div><div id="discoverCompleteBody"></div><div class="status-line" id="discoverCompleteStatus"></div>`);
    bind('dUsual',()=>{discoverMode='Usual';chooseLocal();renderDiscover(hostRef);});bind('dRandom',()=>{discoverMode='Random';chooseLocal();renderDiscover(hostRef);});
    bind('dLocalMovie',()=>{discoverSource='Local Movie';chooseLocal();renderDiscover(hostRef);});bind('dLocalTv',()=>{discoverSource='Local TV';chooseLocal();renderDiscover(hostRef);});bind('dLiveTv',()=>hostRef?.setView&&hostRef.setView('livetv'));
    bind('dExternalMovie',()=>{discoverSource='External Movie';discoverPick=null;renderDiscover(hostRef);renderExternal('movie');});bind('dExternalTv',()=>{discoverSource='External TV';discoverPick=null;renderDiscover(hostRef);renderExternal('tv');});
    bind('dServices',()=>renderServices());
    bind('dTmdbTest',()=>tmdbAction('test'));bind('dTmdbReplace',()=>tmdbAction('replace'));bind('dTmdbClear',()=>tmdbAction('clear'));
    if(discoverSource==='External Movie')renderExternal('movie');else if(discoverSource==='External TV')renderExternal('tv');else renderLocalDiscover();
  }
  function renderLocalDiscover(){
    const body=document.getElementById('discoverCompleteBody');if(!body)return;
    body.innerHTML=`<h2 class="module-heading">DISCOVER • ${esc(discoverSource.toUpperCase())}</h2>${discoverPick?`<div class="discover-result"><div class="discover-poster">${poster(discoverPick)}</div><div class="discover-info"><span class="module-badge">${esc(discoverMode.toUpperCase())}</span><h3>${esc(discoverPick.name)}</h3><p>${esc(discoverPick.type)}${discoverPick.year?` • ${discoverPick.year}`:''}</p><p>Recommended from the media exposed by the live Nougat catalog.</p><div class="discover-actions">${button('Another','dAnother')}${button('Watch','dWatch')}${button('Open Library','dLibrary')}</div></div></div>`:'<div class="module-empty">No matching local media is indexed.</div>'}`;
    bind('dAnother',()=>{chooseLocal();renderLocalDiscover();});bind('dWatch',()=>discoverPick&&hostRef?.playItem&&hostRef.playItem(discoverPick));bind('dLibrary',()=>hostRef?.setView&&hostRef.setView('library'));
  }
  async function renderExternal(kind){
    const body=document.getElementById('discoverCompleteBody');if(!body)return;
    const services=Object.entries(selectedServices()).filter(([,on])=>on).map(([name])=>name);
    body.innerHTML=`<h2 class="module-heading">DISCOVER • EXTERNAL ${kind.toUpperCase()}</h2><div class="module-output" id="externalDiscoverOutput">Asking the Nougat host for external recommendations...</div><div class="module-toolbar">${button('Open TMDb Browse','externalTmdb')}${button('Open JustWatch','externalJustWatch')}</div><div class="module-copy">Selected services: ${esc(services.length?services.join(', '):'none')}</div>`;
    bind('externalTmdb',()=>window.open(kind==='movie'?'https://www.themoviedb.org/movie':'https://www.themoviedb.org/tv','_blank','noopener'));
    bind('externalJustWatch',()=>window.open(kind==='movie'?'https://www.justwatch.com/us/movies':'https://www.justwatch.com/us/tv-shows','_blank','noopener'));
    const out=document.getElementById('externalDiscoverOutput');
    try{const data=await getJson(`/nougat/v1/discover?kind=${encodeURIComponent(kind)}&mode=${encodeURIComponent(discoverMode.toLowerCase())}`);const list=Array.isArray(data.items)?data.items:[];if(list.length){out.innerHTML=list.map(x=>`${esc(x.title||x.name||'Recommendation')}${x.year?` • ${esc(x.year)}`:''}`).join('\n');}else out.textContent=data.status||'Nougat host returned no external recommendations.';}catch(err){out.textContent=`External recommendation bridge unavailable: ${err.message||err}\n\nThe browser does not invent a TMDb recommendation. TMDb and JustWatch browse links remain available above.`;}
  }
  function renderServices(){
    const services=['Netflix','Prime Video','Hulu','Disney+','Max','Paramount+','Peacock','Apple TV+','Tubi','Pluto TV','Plex'];let selected=selectedServices();
    shell(`<div class="module-toolbar">${button('Back to Discover','dServicesBack')}</div><h2 class="module-heading">MY STREAMING SERVICES • UNITED STATES</h2><div class="module-panel-grid">${services.map(name=>`<label class="module-panel-card"><h3><input type="checkbox" data-service="${esc(name)}" ${selected[name]?'checked':''}> ${esc(name)}</h3><p>Include ${esc(name)} in provider filtering when the Nougat recommendation bridge supplies provider data.</p></label>`).join('')}</div>`);
    bind('dServicesBack',()=>renderDiscover(hostRef));root().querySelectorAll('[data-service]').forEach(box=>box.addEventListener('change',()=>{selected[box.dataset.service]=box.checked;localStorage.setItem('nougat-web-services',JSON.stringify(selected));}));
  }
  async function tmdbAction(action){
    const status=document.getElementById('discoverCompleteStatus');if(!status)return;status.textContent=`TMDB ${action.toUpperCase()}...`;
    try{const data=await getJson(`/nougat/v1/discover?action=tmdb-${encodeURIComponent(action)}`);status.textContent=data.status||`TMDb ${action} completed.`;}catch(err){status.textContent=`TMDb host control unavailable: ${err.message||err}`;}
  }

  function renderSearch(host){hostRef=host;shell(`<div class="module-toolbar search-tabs">${searchPanels.map((p,i)=>button(p,`completeSearchTab${i}`,searchPanel===p)).join('')}</div><div id="completeSearchBody"></div>`);searchPanels.forEach((p,i)=>bind(`completeSearchTab${i}`,()=>{searchPanel=p;renderSearch(hostRef);}));renderSearchPanel();}
  function renderSearchPanel(){if(searchPanel==='Crawler')return renderCrawler();if(searchPanel==='P2P')return renderP2p();if(searchPanel==='Archive')return renderArchive();return renderSecureSearch();}
  function localMatches(q){const term=q.trim().toLowerCase();return term?items().filter(x=>`${x.name} ${x.type} ${x.year||''} ${x.filename||''}`.toLowerCase().includes(term)):[];}
  function renderRows(rows){return rows.length?rows.map(x=>`<button type="button" class="module-result-row" data-local-id="${esc(x.id)}"><span>${esc(x.name)}</span><span>${esc(x.type)}</span><span>${x.year||''}</span></button>`).join(''):'<div class="module-empty">No local matches.</div>';}
  function renderSecureSearch(){
    const body=document.getElementById('completeSearchBody');if(!body)return;body.innerHTML=`<h2 class="module-heading">SEARCH</h2><div class="module-field"><span>QUERY</span><input id="secureQuery" type="search" autocomplete="off" spellcheck="false"></div><div class="module-toolbar">${button('Search','secureSearch')}${button('Raw','secureRaw')}${button('Library','secureLibrary')}</div><div class="status-line" id="secureStatus">LOCAL INDEX READY</div><div class="module-result-list" id="secureResults"></div>`;
    const run=async(raw=false)=>{const q=document.getElementById('secureQuery').value.trim(),status=document.getElementById('secureStatus'),out=document.getElementById('secureResults');if(!q){out.innerHTML='<div class="module-empty">Enter a search query.</div>';return;}status.textContent='SEARCHING LOCAL INDEX';let rows=localMatches(q);let remote=null;try{remote=await getJson(`/nougat/v1/search?q=${encodeURIComponent(q)}&raw=${raw?'1':'0'}`);}catch(_){}if(remote&&Array.isArray(remote.results)){out.innerHTML=remote.results.length?remote.results.map(r=>`<a class="module-result-row" href="${esc(r.url||'#')}" target="_blank" rel="noopener"><span>${esc(r.title||r.url||'Result')}</span><span>${esc(r.domain||r.source_network||'Nougat')}</span><span>${esc(r.snippet||'')}</span></a>`).join(''):'<div class="module-empty">No Nougat index matches.</div>';status.textContent=remote.privacy_stage||'NOUGAT SEARCH COMPLETE';return;}out.innerHTML=renderRows(rows);out.querySelectorAll('[data-local-id]').forEach(row=>row.addEventListener('click',()=>{const item=items().find(x=>x.id===row.dataset.localId);if(item&&hostRef?.playItem)hostRef.playItem(item);}));status.textContent=`LOCAL CATALOG • ${rows.length} MATCH${rows.length===1?'':'ES'}`;};
    bind('secureSearch',()=>run(false));bind('secureRaw',()=>run(true));bind('secureLibrary',()=>hostRef?.setView&&hostRef.setView('library'));document.getElementById('secureQuery').addEventListener('keydown',e=>{if(e.key==='Enter')run(false);});
  }
  function renderCrawler(){
    const body=document.getElementById('completeSearchBody');if(!body)return;body.innerHTML=`<h2 class="module-heading">CRAWLER</h2><div class="module-field"><span>SEED URL</span><input id="crawlSeed" type="url" placeholder="https://example.com"></div><div class="module-field"><span>MAX PAGES</span><input id="crawlMax" type="number" min="1" max="500" value="25"></div><label class="module-panel-card"><h3><input id="crawlSame" type="checkbox" checked> SAME DOMAIN</h3></label><div class="module-toolbar">${button('Start Crawl','crawlStart')}${button('Clear','crawlClear')}</div><div class="module-output" id="crawlOutput">CRAWLER READY</div>`;
    bind('crawlClear',()=>document.getElementById('crawlOutput').textContent='CRAWLER READY');bind('crawlStart',async()=>{const seed=document.getElementById('crawlSeed').value.trim(),max=Math.max(1,Math.min(500,Number(document.getElementById('crawlMax').value)||25)),same=document.getElementById('crawlSame').checked,out=document.getElementById('crawlOutput');if(!seed){out.textContent='Enter a seed URL.';return;}out.textContent='Requesting Nougat crawler...';try{const data=await getJson(`/nougat/v1/crawl?seed=${encodeURIComponent(seed)}&max=${max}&sameDomain=${same?'1':'0'}`);out.textContent=data.summary||data.status||JSON.stringify(data,null,2);return;}catch(_){}out.textContent='Host crawler unavailable. Trying browser CORS crawl...';try{const r=await fetch(seed,{mode:'cors'});const text=await r.text();const base=new URL(seed);const doc=new DOMParser().parseFromString(text,'text/html');const links=[...new Set([...doc.querySelectorAll('a[href]')].map(a=>{try{return new URL(a.getAttribute('href'),base).href;}catch(_){return null;}}).filter(Boolean).filter(h=>!same||new URL(h).host===base.host))].slice(0,max);out.textContent=`HTTP ${r.status}\n${links.length} link(s)\n\n${links.join('\n')}`;}catch(err){out.textContent=`Crawler unavailable: ${err.message||err}`;}});
  }
  function parseMagnet(value){if(!value.startsWith('magnet:?'))return null;const p=new URLSearchParams(value.slice(value.indexOf('?')+1));return {name:p.get('dn')||'Unknown',hash:(p.getAll('xt').find(x=>x.startsWith('urn:btih:'))||'').replace('urn:btih:','')||'Unknown',trackers:p.getAll('tr'),uri:value};}
  function p2pQueue(){try{return JSON.parse(localStorage.getItem('nougat-web-p2p-queue')||'[]')||[];}catch(_){return [];}}
  function saveQueue(q){localStorage.setItem('nougat-web-p2p-queue',JSON.stringify(q.slice(0,50)));}
  function renderP2p(){
    const body=document.getElementById('completeSearchBody');if(!body)return;const queue=p2pQueue();body.innerHTML=`<h2 class="module-heading">P2P</h2><div class="module-field"><span>MAGNET</span><textarea id="p2pMagnet" placeholder="magnet:?xt=urn:btih:..."></textarea></div><div class="module-toolbar">${button('Inspect Magnet','p2pInspect')}${button('Open Magnet','p2pOpen')}${button('Queue','p2pQueueAdd')}${button('Clear','p2pClear')}</div><div class="module-output" id="p2pOutput">Native transfer and stream-while-downloading remain host-engine functions. Magnet inspection and system-handler opening work in the browser.</div><h3 class="module-heading">QUEUE</h3><div class="module-result-list" id="p2pQueueList">${queue.length?queue.map((x,i)=>`<button type="button" class="module-result-row" data-p2p-remove="${i}"><span>${esc(x.name)}</span><span>${esc(x.hash)}</span><span>Remove</span></button>`).join(''):'<div class="module-empty">No queued magnets.</div>'}</div>`;
    const inspect=()=>{const m=parseMagnet(document.getElementById('p2pMagnet').value.trim()),out=document.getElementById('p2pOutput');out.textContent=m?`Name: ${m.name}\nInfo hash: ${m.hash}\nTrackers: ${m.trackers.length}`:'That is not a magnet URI.';return m;};bind('p2pInspect',inspect);bind('p2pOpen',()=>{const m=inspect();if(m)location.href=m.uri;});bind('p2pQueueAdd',()=>{const m=inspect();if(!m)return;const q=p2pQueue();q.push(m);saveQueue(q);renderP2p();});bind('p2pClear',()=>{document.getElementById('p2pMagnet').value='';document.getElementById('p2pOutput').textContent='Cleared.';});body.querySelectorAll('[data-p2p-remove]').forEach(row=>row.addEventListener('click',()=>{const q=p2pQueue();q.splice(Number(row.dataset.p2pRemove),1);saveQueue(q);renderP2p();}));
  }
  function renderArchive(){
    const body=document.getElementById('completeSearchBody');if(!body)return;body.innerHTML=`<h2 class="module-heading">ARCHIVE</h2><div class="module-field"><span>SEARCH</span><input id="archiveCompleteQuery" type="search"></div><div class="module-toolbar">${button('Search Internet Archive','archiveSearch')}${button('Open Wayback','archiveWayback')}</div><div class="archive-links"><a class="archive-link" href="https://archive.org" target="_blank" rel="noopener">Internet Archive</a><a class="archive-link" href="https://web.archive.org" target="_blank" rel="noopener">Wayback Machine</a></div>`;
    const run=()=>{const q=document.getElementById('archiveCompleteQuery').value.trim();if(q)window.open(`https://archive.org/search?query=${encodeURIComponent(q)}`,'_blank','noopener');};bind('archiveSearch',run);bind('archiveWayback',()=>{const q=document.getElementById('archiveCompleteQuery').value.trim();window.open(q&&/^https?:\/\//i.test(q)?`https://web.archive.org/web/*/${encodeURIComponent(q)}`:'https://web.archive.org','_blank','noopener');});document.getElementById('archiveCompleteQuery').addEventListener('keydown',e=>{if(e.key==='Enter')run();});
  }

  window.NougatWebModules.activate=function(name,host){
    if(name==='discover')return renderDiscover(host);
    if(name==='search')return renderSearch(host);
    return previous(name,host);
  };
})();