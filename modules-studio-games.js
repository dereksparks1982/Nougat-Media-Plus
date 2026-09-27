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

  function readJson(key,fallback){try{return {...fallback,...(JSON.parse(localStorage.getItem(key)||'{}')||{})};}catch(_){return {...fallback};}}
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
    if(studioPanel==='GIMBAL CONTROL') return renderGimbal(pane);
    if(studioPanel==='PAYLOAD') return renderPayload(pane);
    if(studioPanel==='MAP TOOLS') return renderMapTools(pane);
    if(studioPanel==='FLIGHT LOG') return renderFlightLog(pane);
    if(studioPanel==='PREFERENCES') return renderStudioPrefs(pane);
  }

  function renderFlightPlan(pane){
    const points=readWaypoints();
    pane.innerHTML=`<h2 class="module-heading">${esc(studioPanel)}</h2><div class="module-field"><span>LATITUDE</span><input id="wpLat" inputmode="decimal" placeholder="37.6872"></div><div class="module-field"><span>LONGITUDE</span><input id="wpLon" inputmode="decimal" placeholder="-97.3301"></div><div class="module-field"><span>ALTITUDE</span><input id="wpAlt" inputmode="decimal" placeholder="100"></div><div class="module-toolbar">${button('Add Waypoint','wpAdd')}${button('Import Plan','wpImport')}${button('Export Plan','wpExport')}${button('Clear Plan','wpClear')}</div><input id="wpFile" type="file" accept="application/json,.json" hidden><div class="module-result-list" id="wpList">${points.length?points.map((p,i)=>`<button type="button" class="module-result-row" data-wp="${i}"><span>WP ${i+1} • ${esc(p.lat)}, ${esc(p.lon)}</span><span>${esc(p.alt)} alt</span><span>Remove</span></button>`).join(''):'<div class="module-empty">No waypoints yet.</div>'}</div>`;
    bind('wpAdd',()=>{const lat=parseFloat(document.getElementById('wpLat').value),lon=parseFloat(document.getElementById('wpLon').value),alt=parseFloat(document.getElementById('wpAlt').value||'0');if(!Number.isFinite(lat)||!Number.isFinite(lon)||lat<-90||lat>90||lon<-180||lon>180)return;const next=readWaypoints();next.push({lat,lon,alt:Number.isFinite(alt)?alt:0});saveWaypoints(next);renderStudioPane();});
    bind('wpImport',()=>document.getElementById('wpFile').click());
    document.getElementById('wpFile').addEventListener('change',async(e)=>{const file=e.target.files&&e.target.files[0];if(!file)return;try{const parsed=JSON.parse(await file.text());const raw=Array.isArray(parsed)?parsed:parsed.waypoints;if(!Array.isArray(raw))return;const clean=raw.map(p=>({lat:Number(p.lat),lon:Number(p.lon),alt:Number(p.alt)||0})).filter(p=>Number.isFinite(p.lat)&&Number.isFinite(p.lon)&&p.lat>=-90&&p.lat<=90&&p.lon>=-180&&p.lon<=180);saveWaypoints(clean);renderStudioPane();}catch(_){}});
    bind('wpExport',()=>downloadText('nougat-flight-plan.json',JSON.stringify({version:1,waypoints:readWaypoints()},null,2)));
    bind('wpClear',()=>{saveWaypoints([]);renderStudioPane();});
    pane.querySelectorAll('[data-wp]').forEach(row=>row.addEventListener('click',()=>{const p=readWaypoints();p.splice(Number(row.dataset.wp),1);saveWaypoints(p);renderStudioPane();}));
  }

  function renderGeoFence(pane){
    const state=readJson('nougat-web-geofence',{enabled:false,radius:500,ceiling:120});
    pane.innerHTML=`<h2 class="module-heading">GEO-FENCE</h2><div class="module-panel-grid"><label class="module-panel-card"><h3><input id="geoEnabled" type="checkbox" ${state.enabled?'checked':''}> ENABLED</h3><p>Mission-plan boundary state.</p></label><div class="module-panel-card"><h3>RADIUS</h3><div class="module-field"><input id="geoRadius" type="number" min="1" value="${esc(state.radius)}"></div></div><div class="module-panel-card"><h3>CEILING</h3><div class="module-field"><input id="geoCeiling" type="number" min="1" value="${esc(state.ceiling)}"></div></div></div>${button('Save','geoSave')}`;
    bind('geoSave',()=>{const next={enabled:document.getElementById('geoEnabled').checked,radius:Number(document.getElementById('geoRadius').value)||500,ceiling:Number(document.getElementById('geoCeiling').value)||120};localStorage.setItem('nougat-web-geofence',JSON.stringify(next));});
  }

  function renderRth(pane){
    const state=readJson('nougat-web-rth',{altitude:60,action:'Return Home'});
    pane.innerHTML=`<h2 class="module-heading">RTH SETTINGS</h2><div class="module-field"><span>RTH ALTITUDE</span><input id="rthAltitude" type="number" value="${esc(state.altitude)}"></div><div class="module-field"><span>ACTION</span><select id="rthAction"><option ${state.action==='Return Home'?'selected':''}>Return Home</option><option ${state.action==='Hover'?'selected':''}>Hover</option><option ${state.action==='Land'?'selected':''}>Land</option></select></div>${button('Save','rthSave')}`;
    bind('rthSave',()=>localStorage.setItem('nougat-web-rth',JSON.stringify({altitude:Number(document.getElementById('rthAltitude').value)||60,action:document.getElementById('rthAction').value})));
  }

  async function startCamera(){
    const status=document.getElementById('studioCameraStatus');
    const prefs=readJson('nougat-web-camera',{facingMode:'environment',width:1280,height:720,audio:true});
    try{if(!navigator.mediaDevices||!navigator.mediaDevices.getUserMedia)throw new Error('Camera API unavailable');if(!studioStream)studioStream=await navigator.mediaDevices.getUserMedia({video:{facingMode:prefs.facingMode,width:{ideal:Number(prefs.width)||1280},height:{ideal:Number(prefs.height)||720}},audio:!!prefs.audio});const video=document.getElementById('studioCamera');if(video){video.srcObject=studioStream;await video.play();}if(status)status.textContent='LIVE VIEW ACTIVE';}catch(err){if(status)status.textContent=`LIVE VIEW ERROR • ${err.message||err}`;}
  }
  function stopCamera(){if(studioRecorder&&studioRecorder.state!=='inactive')studioRecorder.stop();if(studioStream){studioStream.getTracks().forEach(t=>t.stop());studioStream=null;}const v=document.getElementById('studioCamera');if(v)v.srcObject=null;}
  async function toggleRecord(){
    const status=document.getElementById('studioCameraStatus');
    if(!studioStream)await startCamera(); if(!studioStream)return;
    if(studioRecorder&&studioRecorder.state==='recording'){studioRecorder.stop();return;}
    studioChunks=[]; try{studioRecorder=new MediaRecorder(studioStream);studioRecorder.ondataavailable=e=>{if(e.data&&e.data.size)studioChunks.push(e.data);};studioRecorder.onstop=()=>{const blob=new Blob(studioChunks,{type:studioRecorder.mimeType||'video/webm'});const u=URL.createObjectURL(blob);const a=document.createElement('a');a.href=u;a.download=`nougat-studio-${new Date().toISOString().replace(/[:.]/g,'-')}.webm`;a.click();setTimeout(()=>URL.revokeObjectURL(u),1000);if(status)status.textContent='RECORDING SAVED';};studioRecorder.start();if(status)status.textContent='RECORDING';}catch(err){if(status)status.textContent=`RECORD ERROR • ${err.message||err}`;}
  }
  function renderCamera(pane){
    const prefs=readJson('nougat-web-camera',{facingMode:'environment',width:1280,height:720,audio:true});
    pane.innerHTML=`<h2 class="module-heading">${esc(studioPanel)}</h2><div class="studio-camera-frame"><video id="studioCamera" playsinline muted></video></div><div class="module-panel-grid"><div class="module-field"><span>CAMERA</span><select id="cameraFacing"><option value="environment" ${prefs.facingMode==='environment'?'selected':''}>Rear / Environment</option><option value="user" ${prefs.facingMode==='user'?'selected':''}>Front / User</option></select></div><div class="module-field"><span>RESOLUTION</span><select id="cameraResolution"><option value="1280x720" ${Number(prefs.width)===1280?'selected':''}>1280 x 720</option><option value="1920x1080" ${Number(prefs.width)===1920?'selected':''}>1920 x 1080</option><option value="640x480" ${Number(prefs.width)===640?'selected':''}>640 x 480</option></select></div></div><label class="module-panel-card"><h3><input id="cameraAudio" type="checkbox" ${prefs.audio?'checked':''}> RECORD AUDIO</h3></label><div class="module-toolbar">${button('Save Camera Settings','studioCamSave')}${button('Start Live View','studioCamStart')}${button('Record','studioCamRecord')}${button('Stop','studioCamStop')}</div><div class="status-line" id="studioCameraStatus">CAMERA READY</div>`;
    bind('studioCamSave',()=>{const [width,height]=document.getElementById('cameraResolution').value.split('x').map(Number);localStorage.setItem('nougat-web-camera',JSON.stringify({facingMode:document.getElementById('cameraFacing').value,width,height,audio:document.getElementById('cameraAudio').checked}));document.getElementById('studioCameraStatus').textContent='CAMERA SETTINGS SAVED';});
    bind('studioCamStart',startCamera);bind('studioCamRecord',toggleRecord);bind('studioCamStop',()=>{stopCamera();document.getElementById('studioCameraStatus').textContent='CAMERA STOPPED';});if(studioStream){const v=document.getElementById('studioCamera');v.srcObject=studioStream;v.play().catch(()=>{});}
  }
  function renderTelemetry(pane){
    pane.innerHTML=`<h2 class="module-heading">${esc(studioPanel)}</h2><table class="diagnostic-table"><tbody><tr><th>Browser online</th><td>${navigator.onLine?'YES':'NO'}</td></tr><tr><th>Orientation API</th><td>${'DeviceOrientationEvent' in window?'AVAILABLE':'UNAVAILABLE'}</td></tr><tr><th>Motion API</th><td>${'DeviceMotionEvent' in window?'AVAILABLE':'UNAVAILABLE'}</td></tr><tr><th>Geolocation API</th><td>${'geolocation' in navigator?'AVAILABLE':'UNAVAILABLE'}</td></tr><tr><th>Camera API</th><td>${navigator.mediaDevices?'AVAILABLE':'UNAVAILABLE'}</td></tr><tr><th>Hardware bridge</th><td>NO AIRCRAFT CONNECTED THROUGH WEB BRIDGE</td></tr></tbody></table>`;
  }
  function renderGimbal(pane){
    const state=readJson('nougat-web-gimbal',{pan:0,tilt:0,roll:0});
    pane.innerHTML=`<h2 class="module-heading">GIMBAL CONTROL</h2><p class="module-copy">Browser mission-planning values. They are not sent to aircraft hardware unless a Nougat hardware bridge is present.</p><div class="module-field"><span>PAN ${esc(state.pan)}°</span><input id="gimbalPan" type="range" min="-180" max="180" value="${esc(state.pan)}"></div><div class="module-field"><span>TILT ${esc(state.tilt)}°</span><input id="gimbalTilt" type="range" min="-90" max="90" value="${esc(state.tilt)}"></div><div class="module-field"><span>ROLL ${esc(state.roll)}°</span><input id="gimbalRoll" type="range" min="-45" max="45" value="${esc(state.roll)}"></div><div class="module-toolbar">${button('Center','gimbalCenter')}${button('Save','gimbalSave')}</div><div class="status-line" id="gimbalStatus">PLANNING MODE</div>`;
    const sync=()=>{['Pan','Tilt','Roll'].forEach(k=>{const el=document.getElementById('gimbal'+k);if(el&&el.parentElement&&el.parentElement.querySelector('span'))el.parentElement.querySelector('span').textContent=`${k.toUpperCase()} ${el.value}°`;});};
    ['gimbalPan','gimbalTilt','gimbalRoll'].forEach(id=>document.getElementById(id).addEventListener('input',sync));
    bind('gimbalCenter',()=>{document.getElementById('gimbalPan').value=0;document.getElementById('gimbalTilt').value=0;document.getElementById('gimbalRoll').value=0;sync();});
    bind('gimbalSave',()=>{localStorage.setItem('nougat-web-gimbal',JSON.stringify({pan:Number(document.getElementById('gimbalPan').value),tilt:Number(document.getElementById('gimbalTilt').value),roll:Number(document.getElementById('gimbalRoll').value)}));document.getElementById('gimbalStatus').textContent='GIMBAL PLAN SAVED';});
  }
  function renderPayload(pane){
    const state=readJson('nougat-web-payload',{type:'Camera',weight:0,notes:''});
    pane.innerHTML=`<h2 class="module-heading">PAYLOAD</h2><div class="module-field"><span>PAYLOAD TYPE</span><select id="payloadType"><option ${state.type==='None'?'selected':''}>None</option><option ${state.type==='Camera'?'selected':''}>Camera</option><option ${state.type==='Sensor'?'selected':''}>Sensor</option><option ${state.type==='Other'?'selected':''}>Other</option></select></div><div class="module-field"><span>WEIGHT</span><input id="payloadWeight" type="number" min="0" step="0.1" value="${esc(state.weight)}"></div><div class="module-field"><span>NOTES</span><textarea id="payloadNotes">${esc(state.notes)}</textarea></div>${button('Save Payload','payloadSave')}<div class="status-line" id="payloadStatus">PAYLOAD PLAN READY</div>`;
    bind('payloadSave',()=>{localStorage.setItem('nougat-web-payload',JSON.stringify({type:document.getElementById('payloadType').value,weight:Number(document.getElementById('payloadWeight').value)||0,notes:document.getElementById('payloadNotes').value.trim()}));document.getElementById('payloadStatus').textContent='PAYLOAD PLAN SAVED';});
  }
  function renderMapTools(pane){
    const points=readWaypoints();
    const geo={type:'FeatureCollection',features:points.map((p,i)=>({type:'Feature',properties:{name:`WP ${i+1}`,altitude:p.alt},geometry:{type:'Point',coordinates:[p.lon,p.lat]}}))};
    pane.innerHTML=`<h2 class="module-heading">MAP TOOLS</h2><div class="module-toolbar">${button('Export GeoJSON','mapGeoJson')}${button('Open First Waypoint','mapOpenFirst',false,points.length?'':'disabled')}</div><div class="module-result-list">${points.length?points.map((p,i)=>`<a class="module-result-row" href="https://www.openstreetmap.org/?mlat=${encodeURIComponent(p.lat)}&mlon=${encodeURIComponent(p.lon)}#map=15/${encodeURIComponent(p.lat)}/${encodeURIComponent(p.lon)}" target="_blank" rel="noopener"><span>WP ${i+1}</span><span>${esc(p.lat)}, ${esc(p.lon)}</span><span>${esc(p.alt)} alt</span></a>`).join(''):'<div class="module-empty">Add waypoints in Flight Plan to populate Map Tools.</div>'}</div>`;
    bind('mapGeoJson',()=>downloadText('nougat-flight-plan.geojson',JSON.stringify(geo,null,2),'application/geo+json'));
    bind('mapOpenFirst',()=>{if(points[0])window.open(`https://www.openstreetmap.org/?mlat=${encodeURIComponent(points[0].lat)}&mlon=${encodeURIComponent(points[0].lon)}#map=15/${encodeURIComponent(points[0].lat)}/${encodeURIComponent(points[0].lon)}`,'_blank','noopener');});
  }
  function renderFlightLog(pane){
    const points=readWaypoints();pane.innerHTML=`<h2 class="module-heading">FLIGHT LOG</h2><div class="module-output">Mission plan contains ${points.length} waypoint(s).\nBrowser Studio stores planning state locally.\nNo aircraft telemetry log has been received in this browser session.</div>${button('Export Mission State','flightLogExport')}`;bind('flightLogExport',()=>downloadText('nougat-studio-state.json',JSON.stringify({waypoints:points,geofence:readJson('nougat-web-geofence',{}),rth:readJson('nougat-web-rth',{}),gimbal:readJson('nougat-web-gimbal',{}),payload:readJson('nougat-web-payload',{}),camera:readJson('nougat-web-camera',{})},null,2)));
  }
  function renderStudioPrefs(pane){
    const state=readJson('nougat-web-studio-prefs',{units:'Imperial',confirm:true});
    pane.innerHTML=`<h2 class="module-heading">PREFERENCES</h2><div class="module-field"><span>UNITS</span><select id="studioUnits"><option ${state.units==='Imperial'?'selected':''}>Imperial</option><option ${state.units==='Metric'?'selected':''}>Metric</option></select></div><label class="module-panel-card"><h3><input id="studioConfirm" type="checkbox" ${state.confirm?'checked':''}> CONFIRM PLAN CHANGES</h3></label>${button('Save','studioPrefsSave')}`;bind('studioPrefsSave',()=>localStorage.setItem('nougat-web-studio-prefs',JSON.stringify({units:document.getElementById('studioUnits').value,confirm:document.getElementById('studioConfirm').checked})));
  }

  const systems = [
    ['NES','Mesen / FCEUX / Nestopia'],['SNES','Mesen / Snes9x'],['Game Boy','SameBoy / mGBA / Mesen'],['Game Boy Color','SameBoy / mGBA / Mesen'],['Game Boy Advance','mGBA / Mesen'],['Nintendo 64','RMG / Mupen64Plus'],['Sega Genesis','BlastEm'],['Sega Master System','BlastEm'],['Sega Game Gear','BlastEm'],['Atari 2600','Stella'],['Atari 5200','Atari800'],['Atari 7800','A7800'],['Atari 8-bit','Atari800'],['Atari Lynx','Mednafen'],['PlayStation','DuckStation'],['PlayStation 2','PCSX2'],['PlayStation Portable','PPSSPP'],['PlayStation 3','RPCS3'],['GameCube','Dolphin'],['Wii','Dolphin'],['Wii U','Cemu'],['Arcade','MAME'],['Nintendo Switch','Ryujinx / Suyu / Yuzu'],['DOS','DOSBox'],['Xbox 360','Xenia Canary']
  ];
  const bundledGames=[
    {title:'2048',system:'NES',art:'./components/games/bundled/artwork/2048.png',file:'./components/games/bundled/2048.nes'},
    {title:'Waveforms',system:'NES',art:'./components/games/bundled/artwork/Waveforms.png',file:'./components/games/bundled/Waveforms.nes'},
    {title:'Ultima Online - The Second Age',system:'Nougat UO',art:'',file:'./components/games/bundled/Ultima%20Online%20-%20The%20Second%20Age/Ultima%20Online%20-%20The%20Second%20Age.nougat-uo',meta:'T2A • Client 1.25.35 • SphereServer-X'}
  ];
  function renderGames(host){
    shell(`<div class="module-toolbar">${button('Library','gamesLibrary',gamesPanel==='Library')}${button('Systems','gamesSystems',gamesPanel==='Systems')}</div><div id="gamesPane"></div>`);
    bind('gamesLibrary',()=>{gamesPanel='Library';renderGames(host);});bind('gamesSystems',()=>{gamesPanel='Systems';renderGames(host);});
    if(gamesPanel==='Systems')renderGameSystems();else renderGameLibrary();
  }
  function renderGameLibrary(){
    const pane=document.getElementById('gamesPane');if(!pane)return;
    pane.innerHTML=`<h2 class="module-heading">GAMES • LIBRARY</h2><div class="game-card-grid">${bundledGames.map((g)=>`<article class="game-card"><div class="game-art">${g.art?`<img src="${g.art}" alt="">`:'<span>UO</span>'}</div><div class="game-body"><strong>${esc(g.title)}</strong><span>${esc(g.meta||g.system)}</span><a class="sheet-button game-action" href="${g.file}" download>Download / Open File</a></div></article>`).join('')}</div><div class="web-only-note">These are the actual bundled Nougat game files from the standalone project. Native emulator launching remains a host-runtime job; the browser does not pretend to run a native emulator.</div>`;
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