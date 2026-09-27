(() => {
  'use strict';
  if (!window.NougatWebModules) return;
  const previous = window.NougatWebModules.activate.bind(window.NougatWebModules);
  let liveSelection = null;
  let radioService = 'Internet';
  const radioServices = ['Local','Internet','AM','FM','HD Radio','DAB / DAB+','DRM','Longwave','Mediumwave','Shortwave','Weather','Emergency','Public Safety','Police','Fire','EMS','Government','Military','Airband','Marine','Railroad','CB','FRS / GMRS','MURS','Amateur / Ham','Business','Utilities','Trunked','P25','DMR','NXDN','TETRA','Paging','ISS / Sat','Weather Sat','Amateur Sat','Favorites','Recordings','Cellular Lab'];
  const esc = (v) => String(v ?? '').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
  const root = () => document.getElementById('moduleView');
  const button = (label,id,active=false,extra='') => `<button type="button" class="sheet-button${active?' active-tool':''}" id="${id}" ${extra}>${esc(label)}</button>`;
  const bind = (id,fn) => { const e=document.getElementById(id); if(e)e.addEventListener('click',fn); };

  function shell(html){ const r=root(); if(!r)return null; r.innerHTML=`<div class="module-workspace">${html}</div>`; return r.querySelector('.module-workspace'); }
  async function jsonGet(host,path){ const response=await fetch(`${host.baseUrl()}${path}`,{cache:'no-store'}); let body={}; try{body=await response.json();}catch(_){body={error:`HTTP ${response.status}`};} if(!response.ok) throw new Error(body.error||`HTTP ${response.status}`); return body; }

  async function renderLiveTv(host){
    shell(`<div class="module-toolbar">${button('Guide','ltGuide',true)}${button('Detect Tuner','ltDetect')}${button('Refresh Tuner','ltRefresh')}${button('Scan Channels','ltScan')}${button('Watch Live','ltWatch')}${button('Stop Live','ltStop')}${button('Refresh Guide','ltGuideRefresh')}${button('Record','ltRecord')}</div>
      <h2 class="module-heading">LIVE TV GUIDE</h2><div class="module-output" id="ltStatus">Connecting to Nougat Live TV bridge...</div><div class="module-result-list" id="ltList"></div>`);
    const status=document.getElementById('ltStatus'), list=document.getElementById('ltList');
    const load=async(action='status')=>{status.textContent=`LIVE TV • ${action.toUpperCase()}...`; try{const data=await jsonGet(host,`/nougat/v1/live-tv?action=${encodeURIComponent(action)}`); const channels=Array.isArray(data.channels)?data.channels:[]; status.textContent=data.status||`Live TV bridge online • ${channels.length} channel(s)`; list.innerHTML=channels.length?channels.map((c,i)=>`<button type="button" class="module-result-row" data-live-index="${i}"><span>${esc(c.id||c.channel||'')}</span><span>${esc(c.name||c.service||'Channel')}</span><span>${esc(c.physical_channel||'')}</span></button>`).join(''):'<div class="module-empty">No channels returned.</div>'; list.querySelectorAll('[data-live-index]').forEach(row=>row.addEventListener('click',()=>{liveSelection=channels[Number(row.dataset.liveIndex)];list.querySelectorAll('.module-result-row').forEach(x=>x.classList.remove('active-tool'));row.classList.add('active-tool');status.textContent=`Selected ${liveSelection.name||liveSelection.id||'channel'}`;}));}catch(err){status.textContent=`Live TV bridge: ${err.message||err}`;list.innerHTML='<div class="module-empty">The web surface is wired. The local Nougat Live TV bridge must expose the tuner action for this control.</div>';}};
    bind('ltGuide',()=>load('guide')); bind('ltDetect',()=>load('detect')); bind('ltRefresh',()=>load('refresh')); bind('ltScan',()=>load('scan')); bind('ltGuideRefresh',()=>load('refresh-guide')); bind('ltStop',()=>load('stop')); bind('ltRecord',()=>load('record'));
    bind('ltWatch',async()=>{if(!liveSelection){status.textContent='Select a channel first.';return;} try{const id=liveSelection.id||liveSelection.channel||''; const data=await jsonGet(host,`/nougat/v1/live-tv?action=watch&id=${encodeURIComponent(id)}`); if(data.url&&host.playUrl)host.playUrl(data.url,liveSelection.name||'Live TV'); else status.textContent=data.status||'No browser stream URL returned.';}catch(err){status.textContent=`Watch Live: ${err.message||err}`;}});
    await load('guide');
  }

  function renderWorldTv(host){
    shell(`<div class="module-toolbar">${button('Watch Live','wtWatch')}${button('Official Site','wtOfficial')}</div><h2 class="module-heading">WORLD TV GUIDE</h2>
      <div class="module-field"><span>STATION / STREAM</span><input id="worldTvUrl" type="url" placeholder="Paste an official broadcaster stream or page URL"></div>
      <div class="module-field"><span>NAME</span><input id="worldTvName" type="text" placeholder="World TV station"></div>
      <div class="module-output" id="worldTvStatus">The standalone World TV controls are present. Paste an official broadcaster URL to use the browser-side player immediately.</div>`);
    bind('wtWatch',()=>{const u=document.getElementById('worldTvUrl').value.trim();const n=document.getElementById('worldTvName').value.trim()||'World TV';if(!u){document.getElementById('worldTvStatus').textContent='Enter a station stream URL.';return;}host.playUrl&&host.playUrl(u,n);});
    bind('wtOfficial',()=>{const u=document.getElementById('worldTvUrl').value.trim();if(u)window.open(u,'_blank','noopener');});
  }

  function radioPresetText(service){
    if(service==='Weather') return 'Weather preset • 162.550 MHz • NFM • 25 kHz step';
    if(service==='ISS / Sat'||service==='Weather Sat'||service==='Amateur Sat') return 'ISS / Satellite preset • 145.800 MHz • NFM • 5 kHz step';
    if(service==='Shortwave'||service==='Longwave'||service==='Mediumwave'||service==='DRM') return 'Shortwave / HF receiver profile';
    if(['Emergency','Public Safety','Police','Fire','EMS','Government','Military','Trunked','P25','DMR','NXDN','TETRA','Paging'].includes(service)) return 'Receive-only emergency / public-safety receiver profile';
    if(service==='FM'||service==='Local') return 'Local FM receiver profile • 88–108 MHz';
    return `${service} receiver profile`;
  }

  function renderRadio(host){
    shell(`<div class="module-toolbar" id="radioServices">${radioServices.map((s,i)=>button(s,`radioService${i}`,s===radioService)).join('')}</div><h2 class="module-heading">RADIO • ${esc(radioService)}</h2>
      <div class="module-output" id="radioStatus"></div><div class="module-toolbar">${button('Listen','radioListen')}${button('Scan','radioScan')}${button('Favorite','radioFavorite')}${button('Stop','radioStop')}</div>`);
    radioServices.forEach((s,i)=>bind(`radioService${i}`,()=>{radioService=s;renderRadio(host);}));
    const status=document.getElementById('radioStatus'); status.textContent=radioService==='Internet'?'Internet Radio • ready for the standalone default station.':radioPresetText(radioService);
    bind('radioListen',()=>{
      if(radioService==='Internet'){host.playUrl&&host.playUrl('https://ice1.somafm.com/groovesalad-128-mp3','SomaFM Groove Salad');return;}
      status.textContent=radioPresetText(radioService)+'\nLocal RF listening requires the Nougat host tuner/SDR bridge.';
    });
    bind('radioScan',async()=>{status.textContent=`Scanning ${radioService} through Nougat host...`;try{const data=await jsonGet(host,`/nougat/v1/radio?action=scan&service=${encodeURIComponent(radioService)}`);status.textContent=data.status||JSON.stringify(data,null,2);}catch(err){status.textContent=`Radio bridge: ${err.message||err}`;}});
    bind('radioFavorite',()=>{let f=[];try{f=JSON.parse(localStorage.getItem('nougat-web-radio-favorites')||'[]')||[];}catch(_){} if(!f.includes(radioService))f.push(radioService);localStorage.setItem('nougat-web-radio-favorites',JSON.stringify(f));status.textContent=`Saved ${radioService} to browser favorites.`;});
    bind('radioStop',()=>{const p=document.getElementById('player');if(p){p.pause();p.removeAttribute('src');p.load();}status.textContent='Radio stopped.';});
  }

  window.NougatWebModules.activate = function(name,host){
    if(name==='livetv') return renderLiveTv(host);
    if(name==='worldtv') return renderWorldTv(host);
    if(name==='radio') return renderRadio(host);
    return previous(name,host);
  };
})();