(() => {
  'use strict';
  if(!window.NougatWebModules)return;
  const previous=window.NougatWebModules.activate.bind(window.NougatWebModules);
  const root=()=>document.getElementById('moduleView');
  const esc=v=>String(v??'').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
  const button=(label,id,active=false,extra='')=>`<button type="button" class="sheet-button${active?' active-tool':''}" id="${id}" ${extra}>${esc(label)}</button>`;
  const bind=(id,fn)=>{const el=document.getElementById(id);if(el)el.addEventListener('click',fn);};
  const shell=html=>{const r=root();if(!r)return null;r.innerHTML=`<div class="module-workspace">${html}</div>`;return r.querySelector('.module-workspace');};
  let hostRef=null;
  let selectedId='';
  let channels=[];
  let programs=[];
  let tuner=null;
  let keyboardBound=false;

  function base(){return hostRef?.baseUrl?hostRef.baseUrl():'';}
  async function call(action,extra={}){const params=new URLSearchParams({action,...extra});const response=await fetch(`${base()}/nougat/v1/live-tv?${params}`,{cache:'no-store'});let data={};try{data=await response.json();}catch(_){data={};}if(!response.ok)throw new Error(data.error||`HTTP ${response.status}`);return data;}
  function remember(){try{localStorage.setItem('nougat-web-live-tv-cache',JSON.stringify({channels,programs,tuner,selectedId,when:Date.now()}));}catch(_){}}
  function restore(){try{const x=JSON.parse(localStorage.getItem('nougat-web-live-tv-cache')||'{}')||{};channels=Array.isArray(x.channels)?x.channels:[];programs=Array.isArray(x.programs)?x.programs:[];tuner=x.tuner||null;selectedId=String(x.selectedId||'');}catch(_){}}
  function channelId(c){return String(c.id||c.channel_id||c.channel||c.program_number||'');}
  function channelName(c){return String(c.name||c.service||`Channel ${channelId(c)}`);}
  function channelLogo(c){
    const value=String(c.logo||c.logo_url||c.logoUrl||c.image||c.image_url||c.icon||'').trim();
    if(!value)return '';
    if(/^https?:\/\//i.test(value)||value.startsWith('data:')||value.startsWith('blob:'))return value;
    if(value.startsWith('/'))return `${base()}${value}`;
    try{return new URL(value,`${base()}/`).href;}catch(_){return '';}
  }
  function channelIdentity(c){
    const logo=channelLogo(c);
    const number=esc(c.virtual_channel||c.channel||c.physical_channel||'');
    const image=logo?`<img data-live-channel-logo src="${esc(logo)}" alt="" loading="lazy" style="width:42px;height:28px;object-fit:contain;flex:0 0 42px">`:'';
    return `<span style="display:flex;align-items:center;gap:8px;min-width:0">${image}<span><b>${number}</b> ${esc(channelName(c))}</span></span>`;
  }
  function nowProgram(id){const now=Math.floor(Date.now()/1000);return programs.find(p=>String(p.channel_id||p.channelId||'')===id&&Number(p.start_unix||p.start||0)<=now&&now<Number(p.start_unix||p.start||0)+Number(p.duration_seconds||p.duration||0));}
  function nextProgram(id){const now=Math.floor(Date.now()/1000);return programs.filter(p=>String(p.channel_id||p.channelId||'')===id&&Number(p.start_unix||p.start||0)>now).sort((a,b)=>Number(a.start_unix||a.start||0)-Number(b.start_unix||b.start||0))[0];}
  function time(unix){if(!Number(unix))return '';return new Date(Number(unix)*1000).toLocaleTimeString([],{hour:'numeric',minute:'2-digit'});}
  function guideRows(){return channels.length?channels.map(c=>{const id=channelId(c),now=nowProgram(id),next=nextProgram(id);return `<button type="button" class="module-result-row${selectedId===id?' active-tool':''}" data-live-guide-id="${esc(id)}">${channelIdentity(c)}<span>${now?`${esc(time(now.start_unix||now.start))} ${esc(now.title||'')}`:'No current guide data'}</span><span>${next?`NEXT ${esc(time(next.start_unix||next.start))} ${esc(next.title||'')}`:''}</span></button>`;}).join(''):'<div class="module-empty">No saved channels. Scan the tuner from this page, or use System for tuner detection and refresh.</div>';}
  function tunerTable(){if(!tuner)return '<div class="module-output">No tuner status has been returned by the Nougat host. Tuner detection and refresh are in System.</div>';return `<table class="diagnostic-table"><tbody><tr><th>Device</th><td>${esc(tuner.name||tuner.id||'Tuner')}</td></tr><tr><th>Backend</th><td>${esc(tuner.backend||'Unknown')}</td></tr><tr><th>Status</th><td>${esc(tuner.status||'Unknown')}</td></tr><tr><th>Frontend</th><td>${tuner.frontend_accessible===false?'Unavailable':'Available'}</td></tr><tr><th>Signal</th><td>${Number.isFinite(Number(tuner.signal_percent))?`${esc(tuner.signal_percent)}%`:'Unknown'}</td></tr><tr><th>Quality</th><td>${Number.isFinite(Number(tuner.quality_percent))?`${esc(tuner.quality_percent)}%`:'Unknown'}</td></tr><tr><th>Delivery</th><td>${esc(tuner.delivery_systems||'Unknown')}</td></tr></tbody></table>`;}
  function renderBody(message=''){
    const body=document.getElementById('liveTvFullBody');if(!body)return;
    body.innerHTML=`<h2 class="module-heading">LIVE TV GUIDE</h2>${tunerTable()}<div class="status-line" id="liveTvFullStatus">${esc(message||`${channels.length} CHANNEL${channels.length===1?'':'S'} • ${programs.length} GUIDE EVENT${programs.length===1?'':'S'}`)}</div><div class="module-result-list live-tv-guide" id="liveTvGuideRows">${guideRows()}</div>`;
    body.querySelectorAll('[data-live-channel-logo]').forEach(image=>image.addEventListener('error',()=>image.remove(),{once:true}));
    body.querySelectorAll('[data-live-guide-id]').forEach(row=>row.addEventListener('click',()=>selectChannel(row.dataset.liveGuideId)));
  }
  function scrollSelected(behavior='smooth'){
    requestAnimationFrame(()=>{
      const row=[...document.querySelectorAll('[data-live-guide-id]')].find(x=>x.dataset.liveGuideId===selectedId);
      row?.scrollIntoView({block:'nearest',behavior});
      row?.focus({preventScroll:true});
    });
  }
  function selectChannel(id,message=''){
    const channel=channels.find(c=>channelId(c)===String(id||''));
    if(!channel)return;
    selectedId=channelId(channel);
    remember();
    const program=nowProgram(selectedId);
    renderBody(message||`Selected ${channelName(channel)}${program?.title?` • ${program.title}`:''}`);
    scrollSelected();
  }
  function moveSelection(delta){
    const selectable=channels.filter(c=>channelId(c));
    if(!selectable.length)return;
    let index=selectable.findIndex(c=>channelId(c)===selectedId);
    if(index<0)index=delta<0?selectable.length:-1;
    index=(index+delta+selectable.length)%selectable.length;
    selectChannel(channelId(selectable[index]));
  }
  function focusNow(){
    const current=channels.find(c=>nowProgram(channelId(c)));
    if(!current){renderBody('No current guide event is available to focus.');return;}
    const id=channelId(current),program=nowProgram(id);
    selectChannel(id,`NOW • ${channelName(current)}${program?.title?` • ${program.title}`:''}`);
  }
  async function watchSelected(){
    if(!selectedId){renderBody('Select a channel first.');return;}
    const data=await run('watch',{id:selectedId});
    const channel=channels.find(c=>channelId(c)===selectedId)||{};
    if(data&&(data.url||data.streamUrl)&&hostRef?.playUrl)hostRef.playUrl(data.url||data.streamUrl,channelName(channel));
  }
  function liveTvKeydown(event){
    const target=event.target;
    if(target&&(['INPUT','TEXTAREA','SELECT'].includes(target.tagName)||target.isContentEditable))return;
    if(event.key==='ArrowDown'){event.preventDefault();moveSelection(1);return;}
    if(event.key==='ArrowUp'){event.preventDefault();moveSelection(-1);return;}
    if(event.key==='Home'&&channels.length){event.preventDefault();selectChannel(channelId(channels[0]));return;}
    if(event.key==='End'&&channels.length){event.preventDefault();selectChannel(channelId(channels[channels.length-1]));return;}
    if(event.key==='Enter'&&selectedId){event.preventDefault();watchSelected();}
  }
  function bindKeyboard(){if(keyboardBound)return;document.addEventListener('keydown',liveTvKeydown);keyboardBound=true;}
  function unbindKeyboard(){if(!keyboardBound)return;document.removeEventListener('keydown',liveTvKeydown);keyboardBound=false;}
  function absorb(data){if(Array.isArray(data.channels))channels=data.channels;if(Array.isArray(data.programs))programs=data.programs;if(data.tuner)tuner=data.tuner;else if(Array.isArray(data.tuners)&&data.tuners[0])tuner=data.tuners[0];if(selectedId&&!channels.some(c=>channelId(c)===selectedId))selectedId='';remember();}
  async function run(action,extra={}){const status=document.getElementById('liveTvFullStatus');if(status)status.textContent=`LIVE TV • ${action.toUpperCase()}...`;try{const data=await call(action,extra);absorb(data);renderBody(data.status||`${action} complete.`);return data;}catch(err){renderBody(`Live TV host bridge unavailable: ${err.message||err}`);return null;}}
  async function renderLiveTv(host){
    hostRef=host;restore();bindKeyboard();
    shell(`<div class="module-toolbar">${button('Guide','liveGuide',true)}${button('Now','liveNow')}${button('Scan Channels','liveScan')}${button('Watch Live','liveWatch')}${button('Stop Live','liveStop')}${button('Refresh Guide','liveRefreshGuide')}${button('Record','liveRecord')}</div><div id="liveTvFullBody"></div>`);
    bind('liveGuide',()=>run('guide'));bind('liveNow',focusNow);bind('liveScan',()=>run('scan'));bind('liveRefreshGuide',()=>run('refresh-guide'));bind('liveStop',()=>run('stop'));bind('liveRecord',()=>selectedId?run('record',{id:selectedId}):renderBody('Select a channel before recording.'));
    bind('liveWatch',watchSelected);
    renderBody();
    if(selectedId)scrollSelected('auto');
    const data=await run('guide');if(!data&&channels.length)renderBody('Showing last saved Live TV guide because the host bridge is currently unavailable.');
  }
  window.NougatWebModules.activate=function(name,host){if(name==='livetv')return renderLiveTv(host);unbindKeyboard();return previous(name,host);};
})();