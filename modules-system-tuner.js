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
  function saveTuner(data){
    const tuner=data?.tuner||(Array.isArray(data?.tuners)?data.tuners[0]:null);
    if(!tuner)return;
    try{
      const cache=JSON.parse(localStorage.getItem('nougat-web-live-tv-cache')||'{}')||{};
      cache.tuner=tuner;cache.when=Date.now();
      localStorage.setItem('nougat-web-live-tv-cache',JSON.stringify(cache));
    }catch(_){}
  }
  function summary(data,action){
    const tuner=data?.tuner||(Array.isArray(data?.tuners)?data.tuners[0]:null);
    if(!tuner)return data?.status||`Tuner ${action} completed.`;
    const name=tuner.name||tuner.id||'Tuner';
    const backend=tuner.backend||'Unknown backend';
    const status=tuner.status||'Status returned';
    return `${name} • ${backend} • ${status}`;
  }
  async function run(action){
    const out=document.getElementById('systemTunerStatus');if(!out)return;
    out.textContent=`TUNER • ${action.toUpperCase()}...`;
    try{const data=await call(action);saveTuner(data);out.textContent=summary(data,action);}
    catch(err){out.textContent=`Tuner host bridge unavailable: ${err.message||err}`;}
  }
  function augment(){
    const r=root();if(!r||document.getElementById('systemTunerToolbar'))return;
    const tabs=r.querySelector('.module-workspace .search-tabs');if(!tabs)return;
    const bar=document.createElement('div');
    bar.className='module-toolbar';bar.id='systemTunerToolbar';
    bar.innerHTML=`<button type="button" class="sheet-button" id="systemDetectTuner">Detect Tuner</button><button type="button" class="sheet-button" id="systemRefreshTuner">Refresh Tuner</button><span class="status-line" id="systemTunerStatus">TUNER ADMIN • SYSTEM</span>`;
    tabs.insertAdjacentElement('afterend',bar);
    document.getElementById('systemDetectTuner')?.addEventListener('click',()=>run('detect'));
    document.getElementById('systemRefreshTuner')?.addEventListener('click',()=>run('refresh'));
  }
  function start(){
    stop();augment();
    observer=new MutationObserver(()=>augment());
    const r=root();if(r)observer.observe(r,{childList:true,subtree:true});
  }
  function stop(){if(observer){observer.disconnect();observer=null;}}

  window.NougatWebModules.activate=function(name,host){
    if(name==='system'){
      hostRef=host;
      const result=previous(name,host);
      queueMicrotask(start);
      return result;
    }
    stop();
    return previous(name,host);
  };
})();