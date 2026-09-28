(() => {
  'use strict';
  if(!window.NougatWebModules)return;
  const previous=window.NougatWebModules.activate.bind(window.NougatWebModules);
  const root=()=>document.getElementById('moduleView');
  const esc=v=>String(v??'').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
  let hostRef=null;
  let observer=null;

  function base(){return hostRef?.baseUrl?hostRef.baseUrl():'';}
  async function call(action){
    const response=await fetch(`${base()}/nougat/v1/live-tv?action=${encodeURIComponent(action)}`,{cache:'no-store'});
    let data={};try{data=await response.json();}catch(_){data={};}
    if(!response.ok)throw new Error(data.error||`HTTP ${response.status}`);
    return data;
  }
  function tunerFrom(data){return data?.tuner||(Array.isArray(data?.tuners)?data.tuners[0]:null);}
  function cachedTuner(){try{return (JSON.parse(localStorage.getItem('nougat-web-live-tv-cache')||'{}')||{}).tuner||null;}catch(_){return null;}}
  function saveTuner(data){
    const tuner=tunerFrom(data);if(!tuner)return;
    try{const cache=JSON.parse(localStorage.getItem('nougat-web-live-tv-cache')||'{}')||{};cache.tuner=tuner;cache.when=Date.now();localStorage.setItem('nougat-web-live-tv-cache',JSON.stringify(cache));}catch(_){}
  }
  function percent(value){const n=Number(value);return Number.isFinite(n)?`${Math.max(0,Math.min(100,n))}%`:'Unknown';}
  function yesNo(value){if(value===true)return 'YES';if(value===false)return 'NO';return 'Unknown';}
  function tunerTable(tuner){
    if(!tuner)return '<div class="module-output">No tuner state has been returned by the Nougat host yet.</div>';
    const delivery=Array.isArray(tuner.delivery_systems)?tuner.delivery_systems.join(', '):(tuner.delivery_systems||tuner.delivery||'Unknown');
    return `<table class="diagnostic-table"><tbody>
      <tr><th>Device</th><td>${esc(tuner.name||tuner.id||tuner.device||'Tuner')}</td></tr>
      <tr><th>Backend</th><td>${esc(tuner.backend||'Unknown')}</td></tr>
      <tr><th>Status</th><td>${esc(tuner.status||'Unknown')}</td></tr>
      <tr><th>Frontend accessible</th><td>${yesNo(tuner.frontend_accessible)}</td></tr>
      <tr><th>Signal</th><td>${percent(tuner.signal_percent)}</td></tr>
      <tr><th>Quality</th><td>${percent(tuner.quality_percent)}</td></tr>
      <tr><th>Delivery systems</th><td>${esc(delivery)}</td></tr>
      ${tuner.device_path?`<tr><th>Device path</th><td>${esc(tuner.device_path)}</td></tr>`:''}
      ${tuner.driver?`<tr><th>Driver</th><td>${esc(tuner.driver)}</td></tr>`:''}
      ${tuner.frequency_hz?`<tr><th>Frequency</th><td>${esc(tuner.frequency_hz)} Hz</td></tr>`:''}
    </tbody></table>`;
  }
  function renderTuner(tuner,message=''){
    const panel=document.getElementById('systemTunerDetails'),status=document.getElementById('systemTunerStatus');
    if(status&&message)status.textContent=message;
    if(panel)panel.innerHTML=tunerTable(tuner);
  }
  async function run(action){
    const out=document.getElementById('systemTunerStatus');if(!out)return;
    out.textContent=`TUNER • ${action.toUpperCase()}...`;
    try{
      const data=await call(action),tuner=tunerFrom(data);saveTuner(data);
      renderTuner(tuner,data.status||`${tuner?.name||tuner?.id||'Tuner'} • ${tuner?.backend||'Unknown backend'} • ${tuner?.status||'Status returned'}`);
    }catch(err){renderTuner(cachedTuner(),`Tuner host bridge unavailable: ${err.message||err}`);}
  }
  function augment(){
    const r=root();if(!r||document.getElementById('systemTunerToolbar'))return;
    const tabs=r.querySelector('.module-workspace .search-tabs');if(!tabs)return;
    const wrapper=document.createElement('div');wrapper.id='systemTunerAdmin';
    wrapper.innerHTML=`<div class="module-toolbar" id="systemTunerToolbar"><button type="button" class="sheet-button" id="systemDetectTuner">Detect Tuner</button><button type="button" class="sheet-button" id="systemRefreshTuner">Refresh Tuner</button><span class="status-line" id="systemTunerStatus">TUNER ADMIN • SYSTEM</span></div><div id="systemTunerDetails"></div>`;
    tabs.insertAdjacentElement('afterend',wrapper);
    document.getElementById('systemDetectTuner')?.addEventListener('click',()=>run('detect'));
    document.getElementById('systemRefreshTuner')?.addEventListener('click',()=>run('refresh'));
    renderTuner(cachedTuner());
  }
  function start(){stop();augment();observer=new MutationObserver(()=>augment());const r=root();if(r)observer.observe(r,{childList:true,subtree:true});}
  function stop(){if(observer){observer.disconnect();observer=null;}}

  window.NougatWebModules.activate=function(name,host){
    if(name==='system'){hostRef=host;const result=previous(name,host);queueMicrotask(start);return result;}
    stop();return previous(name,host);
  };
})();