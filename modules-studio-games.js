(() => {
  'use strict';
  if (!window.NougatWebModules) return;

  const previous = window.NougatWebModules.activate.bind(window.NougatWebModules);
  const esc = (v) => String(v ?? '').replace(/[&<>"']/g, (c) => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
  const root = () => document.getElementById('moduleView');
  const button = (label,id,active=false,extra='') => `<button type="button" class="sheet-button${active?' active-tool':''}" id="${id}" ${extra}>${esc(label)}</button>`;
  const bind = (id,fn) => { const el=document.getElementById(id); if(el) el.addEventListener('click',fn); };
  const shell = (html) => { const r=root(); if(!r)return null; r.innerHTML=`<div class="module-workspace">${html}</div>`; return r.querySelector('.module-workspace'); };

  let studioPanel = 'FLIGHT PLAN';
  let studioStream = null;
  let studioRecorder = null;
  let studioChunks = [];
  let gamesPanel = 'Library';

  const studioItems = [
    'FLIGHT PLAN','WAYPOINTS','ROUTES','GEO-FENCE','RTH SETTINGS',
    'AIRCRAFT / TELEMETRY','SENSORS','DIAGNOSTICS','LIVE VIEW','RECORDING',
    'CAMERA SETTINGS','GIMBAL CONTROL','PAYLOAD','MAP TOOLS','FLIGHT LOG','PREFERENCES'
  ];

  function readWaypoints(){ try{return JSON.parse(localStorage.getItem('nougat-web-studio-waypoints')||'[]')||[];}catch(_){return [];} }
  function saveWaypoints(points){ try{localStorage.setItem('nougat-web-studio-waypoints',JSON.stringify(points));}catch(_){} }
  function downloadText(name,text,type='application/json'){
    const blob=new Blob([text],{type}); const url=URL.createObjectURL(blob); const a=document.createElement('a'); a.href=url; a.download=name; a.click(); setTimeout(()=>URL.revokeObjectURL(url),1000);
  }

  function renderStudio(host){
    const nav = studioItems.map((item,i)=>button(item,`studioNav${i}`,studioPanel===item)).join('');
    shell(`<div class="module-panel-grid studio-layout"><div class="module-panel-card studio-nav-card"><h3>MISSION</h3><div class="studio-nav">${nav}</div>${button('REFRESH STACK','studioRefresh')}</div><div class="module-panel-card studio-main-card"><div id="studioPane"></div></div></div>`);
    studioItems.forEach((item,i)=>bind(`studioNav${i}`,()=>{studioPanel=item;renderStudio(host);}));
    bind('studioRefresh',()=>renderStudio(host));
    renderStudioPane();
  }

  function renderStudioPane(){
    const pane=document.getElementById('studioPane'); if(!pane)return;
    if(studioPanel==='FLIGHT PLAN'||studioPanel==='WAYPOINTS'||studioPanel==='ROUTES') return renderFlightPlan(pane);
    if(studioPanel==='LIVE VIEW'||studioPanel==='RECORDING'||studioPanel==='CAMERA SETTINGS') return renderCamera(pane);
    if(studioPanel==='GEO-FENCE') return renderGeoFence(pane);
    if(studioPanel==='RTH SETTINGS') return renderRth(pane);
    if(studioPanel==='AIRCRAFT / TELEMETRY'||studioPanel==='SENSORS'||studioPanel==='DIAGNOSTICS') return renderTelemetry(pane);
    if(studioPanel==='FLIGHT LOG') return renderFlightLog(pane);
    if(studioPanel==='PREFERENCES') return renderStudioPrefs(pane);
    pane.innerHTML=`<h2 class="module-heading">${esc(studioPanel)}</h2><div class="module-output">${esc(studioPanel)} controls are kept inside the Studio mission stack. Browser-side controls are stored locally and do not pretend to command aircraft hardware unless a real connected bridge is present.</div>`;
  }

  function renderFlightPlan(pane){
    const points=readWaypoints();
    pane.innerHTML=`<h2 class="module-heading">${esc(studioPanel)}</h2><div class="module-field"><span>LATITUDE</span><input id="wpLat" inputmode="decimal" placeholder="37.6872"></div><div class="module-field"><span>LONGITUDE</span><input id="wpLon" inputmode="decimal" placeholder="-97.3301"></div><div class="module-field"><span>ALTITUDE</span><input id="wpAlt" inputmode="decimal" placeholder="100"></div><div class="module-toolbar">${button('Add Waypoint','wpAdd')}${button('Export Plan','wpExport')}${button('Clear Plan','wpClear')}</div><div class="module-result-list" id="wpList">${points.length?points.map((p,i)=>`<button type="button" class="module-result-row" data-wp="${i}"><span>WP ${i+1} • ${esc(p.lat)}, ${esc(p.lon)}</span><span>${esc(p.alt)} alt</span><span>Remove</span></button>`).join(''):'<div class="module-empty">No waypoints yet.</div>'}</div>`;
    bind('wpAdd',()=>{const lat=parseFloat(document.getElementById('wpLat').value),lon=parseFloat(document.getElementById('wpLon').value),alt=parseFloat(document.getElementById('wpAlt').value||'0');if(!Number.isFinite(lat)||!Number.isFinite(lon)||lat<-90||lat>90||lon<-180||lon>180)return;const next=readWaypoints();next.push({lat,lon,alt:Number.isFinite(alt)?alt:0});saveWaypoints(next);renderStudioPane();});
    bind('wpExport',()=>downloadText('nougat-flight-plan.json',JSON.stringify({version:1,waypoints:readWaypoints()},null,2)));
    bind('wpClear',()=>{saveWaypoints([]);renderStudioPane();});
    pane.querySelectorAll('[data-wp]').forEach(row=>row.addEventListener('click',()=>{const p=readWaypoints();p.splice(Number(row.dataset.wp),1);saveWaypoints(p);renderStudioPane();}));
  }

  function renderGeoFence(pane){
    let state={enabled:false,radius:500,ceiling:120}; try{state={...state,...JSON.parse(localStorage.getItem('nougat-web-geofence')||'{}')};}catch(_){}
    pane.innerHTML=`<h2 class="module-heading">GEO-FENCE</h2><div class="module-panel-grid"><label class="module-panel-card"><h3><input id="geoEnabled" type="checkbox" ${state.enabled?'checked':''}> ENABLED</h3><p>Mission-plan boundary state.</p></label><div class="module-panel-card"><h3>RADIUS</h3><div class="module-field"><input id="geoRadius" type="number" min="1" value="${esc(state.radius)}"></div></div><div class="module-panel-card"><h3>CEILING</h3><div class="module-field"><input id="geoCeiling" type="number" min="1" value="${esc(state.ceiling)}"></div></div></div>${button('Save','geoSave')}`;
    bind('geoSave',()=>{const next={enabled:document.getElementById('geoEnabled').checked,radius:Number(document.getElementById('geoRadius').value)||500,ceiling:Number(document.getElementById('geoCeiling').value)||120};localStorage.setItem('nougat-web-geofence',JSON.stringify(next));});
  }

  function renderRth(pane){
    let state={altitude:60,action:'Return Home'}; try{state={...state,...JSON.parse(localStorage.getItem('nougat-web-rth')||'{}')};}catch(_){}
    pane.innerHTML=`<h2 class="module-heading">RTH SETTINGS</h2><div class="module-field"><span>RTH ALTITUDE</span><input id="rthAltitude" type="number" value="${esc(state.altitude)}"></div><div class="module-field"><span>ACTION</span><input id="rthAction" value="${esc(state.action)}"></div>${button('Save','rthSave')}`;
    bind('rthSave',()=>localStorage.setItem('nougat-web-rth',JSON.stringify({altitude:Number(document.getElementById('rthAltitude').value)||60,action:document.getElementById('rthAction').value.trim()||'Return Home'})));
  }

  async function startCamera(){
    const status=document.getElementById('studioCameraStatus');
    try{if(!navigator.mediaDevices||!navigator.mediaDevices.getUserMedia)throw new Error('Camera API unavailable');if(!studioStream)studioStream=await navigator.mediaDevices.getUserMedia({video:true,audio:true});const video=document.getElementById('studioCamera');if(video){video.srcObject=studioStream;await video.play();}if(status)status.textContent='LIVE VIEW ACTIVE';}catch(err){if(status)status.textContent=`LIVE VIEW ERROR • ${err.message||err}`;}
  }
  function stopCamera(){if(studioRecorder&&studioRecorder.state!=='inactive')studioRecorder.stop();if(studioStream){studioStream.getTracks().forEach(t=>t.stop());studioStream=null;}const v=document.getElementById('studioCamera');if(v)v.srcObject=null;}
  async function toggleRecord(){
    const status=document.getElementById('studioCameraStatus');
    if(!studioStream)await startCamera(); if(!studioStream)return;
    if(studioRecorder&&studioRecorder.state==='recording'){studioRecorder.stop();return;}
    studioChunks=[]; try{studioRecorder=new MediaRecorder(studioStream);studioRecorder.ondataavailable=e=>{if(e.data&&e.data.size)studioChunks.push(e.data);};studioRecorder.onstop=()=>{const blob=new Blob(studioChunks,{type:studioRecorder.mimeType||'video/webm'});const u=URL.createObjectURL(blob);const a=document.createElement('a');a.href=u;a.download=`nougat-studio-${new Date().toISOString().replace(/[:.]/g,'-')}.webm`;a.click();setTimeout(()=>URL.revokeObjectURL(u),1000);if(status)status.textContent='RECORDING SAVED';};studioRecorder.start();if(status)status.textContent='RECORDING';}catch(err){if(status)status.textContent=`RECORD ERROR • ${err.message||err}`;}
  }
  function renderCamera(pane){
    pane.innerHTML=`<h2 class="module-heading">${esc(studioPanel)}</h2><div class="studio-camera-frame"><video id="studioCamera" playsinline muted></video></div><div class="module-toolbar">${button('Start Live View','studioCamStart')}${button('Record','studioCamRecord')}${button('Stop','studioCamStop')}</div><div class="status-line" id="studioCameraStatus">CAMERA READY</div>`;
    bind('studioCamStart',startCamera);bind('studioCamRecord',toggleRecord);bind('studioCamStop',()=>{stopCamera();document.getElementById('studioCameraStatus').textContent='CAMERA STOPPED';});if(studioStream){const v=document.getElementById('studioCamera');v.srcObject=studioStream;v.play().catch(()=>{});}
  }
  function renderTelemetry(pane){
    pane.innerHTML=`<h2 class="module-heading">${esc(studioPanel)}</h2><table class="diagnostic-table"><tbody><tr><th>Browser online</th><td>${navigator.onLine?'YES':'NO'}</td></tr><tr><th>Orientation API</th><td>${'DeviceOrientationEvent' in window?'AVAILABLE':'UNAVAILABLE'}</td></tr><tr><th>Motion API</th><td>${'DeviceMotionEvent' in window?'AVAILABLE':'UNAVAILABLE'}</td></tr><tr><th>Camera API</th><td>${navigator.mediaDevices?'AVAILABLE':'UNAVAILABLE'}</td></tr><tr><th>Hardware bridge</th><td>NO AIRCRAFT CONNECTED THROUGH WEB BRIDGE</td></tr></tbody></table>`;
  }
  function renderFlightLog(pane){
    const points=readWaypoints();pane.innerHTML=`<h2 class="module-heading">FLIGHT LOG</h2><div class="module-output">Mission plan contains ${points.length} waypoint(s).\nBrowser Studio stores planning state locally.\nNo aircraft telemetry log has been received in this browser session.</div>${button('Export Mission State','flightLogExport')}`;bind('flightLogExport',()=>downloadText('nougat-studio-state.json',JSON.stringify({waypoints:points,geofence:JSON.parse(localStorage.getItem('nougat-web-geofence')||'{}'),rth:JSON.parse(localStorage.getItem('nougat-web-rth')||'{}')},null,2)));
  }
  function renderStudioPrefs(pane){
    let state={units:'Imperial',confirm:true};try{state={...state,...JSON.parse(localStorage.getItem('nougat-web-studio-prefs')||'{}')};}catch(_){}
    pane.innerHTML=`<h2 class="module-heading">PREFERENCES</h2><div class="module-field"><span>UNITS</span><select id="studioUnits"><option ${state.units==='Imperial'?'selected':''}>Imperial</option><option ${state.units==='Metric'?'selected':''}>Metric</option></select></div><label class="module-panel-card"><h3><input id="studioConfirm" type="checkbox" ${state.confirm?'checked':''}> CONFIRM PLAN CHANGES</h3></label>${button('Save','studioPrefsSave')}`;bind('studioPrefsSave',()=>localStorage.setItem('nougat-web-studio-prefs',JSON.stringify({units:document.getElementById('studioUnits').value,confirm:document.getElementById('studioConfirm').checked})));
  }

  const systems = [
    ['NES','Mesen / FCEUX / Nestopia'],['SNES','Mesen / Snes9x'],['Game Boy','SameBoy / mGBA / Mesen'],['Game Boy Color','SameBoy / mGBA / Mesen'],['Game Boy Advance','mGBA / Mesen'],['Nintendo 64','RMG / Mupen64Plus'],['Sega Genesis','BlastEm'],['Sega Master System','BlastEm'],['Sega Game Gear','BlastEm'],['Atari 2600','Stella'],['Atari 5200','Atari800'],['Atari 7800','A7800'],['Atari 8-bit','Atari800'],['Atari Lynx','Mednafen'],['PlayStation','DuckStation'],['PlayStation 2','PCSX2'],['PlayStation Portable','PPSSPP'],['PlayStation 3','RPCS3'],['GameCube','Dolphin'],['Wii','Dolphin'],['Wii U','Cemu'],['Arcade','MAME'],['Nintendo Switch','Ryujinx / Suyu / Yuzu'],['DOS','DOSBox'],['Xbox 360','Xenia Canary']
  ];
  const bundledGames=[
    {title:'2048',system:'NES',art:'./games/artwork/2048.png',file:'./games/2048.nes'},
    {title:'Waveforms',system:'NES',art:'./games/artwork/Waveforms.png',file:'./games/Waveforms.nes'},
    {title:'Ultima Online - The Second Age',system:'Nougat UO',art:'',file:'./games/Ultima Online - The Second Age.nougat-uo',meta:'T2A • Client 1.25.35 • SphereServer-X'}
  ];
  function renderGames(host){
    shell(`<div class="module-toolbar">${button('Library','gamesLibrary',gamesPanel==='Library')}${button('Systems','gamesSystems',gamesPanel==='Systems')}</div><div id="gamesPane"></div>`);
    bind('gamesLibrary',()=>{gamesPanel='Library';renderGames(host);});bind('gamesSystems',()=>{gamesPanel='Systems';renderGames(host);});
    if(gamesPanel==='Systems')renderGameSystems();else renderGameLibrary();
  }
  function renderGameLibrary(){
    const pane=document.getElementById('gamesPane');if(!pane)return;
    pane.innerHTML=`<h2 class="module-heading">GAMES • LIBRARY</h2><div class="game-card-grid">${bundledGames.map((g)=>`<article class="game-card"><div class="game-art">${g.art?`<img src="${g.art}" alt="">`:'<span>UO</span>'}</div><div class="game-body"><strong>${esc(g.title)}</strong><span>${esc(g.meta||g.system)}</span><a class="sheet-button game-action" href="${g.file}" download>Download / Open File</a></div></article>`).join('')}</div><div class="web-only-note">The web Library exposes the actual bundled Nougat game files. Native emulator launching still belongs to the host runtime, so this page does not fake a browser emulator.</div>`;
  }
  function renderGameSystems(){
    const pane=document.getElementById('gamesPane');if(!pane)return;
    pane.innerHTML=`<h2 class="module-heading">GAMES • SYSTEMS</h2><div class="module-panel-grid">${systems.map(([system,backend])=>`<div class="module-panel-card"><h3>${esc(system)}</h3><p>${esc(backend)}</p></div>`).join('')}</div>`;
  }

  window.NougatWebModules.activate=function(name,host){
    if(name==='studio')return renderStudio(host);
    if(name==='games')return renderGames(host);
    return previous(name,host);
  };
})();