(() => {
  'use strict';
  if(!window.NougatWebModules)return;
  const previous=window.NougatWebModules.activate.bind(window.NougatWebModules);
  const root=()=>document.getElementById('moduleView');
  const esc=v=>String(v??'').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
  const button=(label,id,active=false)=>`<button type="button" class="sheet-button${active?' active-tool':''}" id="${id}">${esc(label)}</button>`;
  let mode=localStorage.getItem('nougat-web-radio-mode')==='PRO'?'PRO':'RADIO';
  let simpleService=localStorage.getItem('nougat-web-radio-simple-service')||'Internet';
  const simpleServices=['Internet','FM','AM','Weather','Airband','Marine','Amateur / Ham','Shortwave','Favorites'];

  function state(){try{return {...{frequencyHz:100100000,modulation:'WFM',stepHz:100000,gain:50,squelch:0},...(JSON.parse(localStorage.getItem('nougat-web-radio-state')||'{}')||{})};}catch(_){return {frequencyHz:100100000,modulation:'WFM',stepHz:100000,gain:50,squelch:0};}}
  function save(s){localStorage.setItem('nougat-web-radio-state',JSON.stringify(s));}
  function favorites(){try{return JSON.parse(localStorage.getItem('nougat-web-radio-presets')||'[]')||[];}catch(_){return [];}}
  function saveFavorites(list){localStorage.setItem('nougat-web-radio-presets',JSON.stringify(list.slice(0,100)));}
  function mhz(hz){return (Number(hz)/1000000).toFixed(3);}
  function preset(name,s){const map={FM:[100100000,'WFM',100000],AM:[1000000,'AM',10000],Weather:[162550000,'NFM',25000],Airband:[121500000,'AM',25000],Marine:[156800000,'NFM',25000],'Amateur / Ham':[146520000,'NFM',5000],Shortwave:[10000000,'AM',5000]};const p=map[name];if(p){s.frequencyHz=p[0];s.modulation=p[1];s.stepHz=p[2];save(s);}return s;}
  function setMode(next,host){mode=next;localStorage.setItem('nougat-web-radio-mode',mode);window.NougatWebModules.activate('radio',host);}
  function addModeBar(host){const work=root()?.querySelector('.module-workspace');if(!work)return;const bar=document.createElement('div');bar.className='module-toolbar radio-mode-switch';bar.innerHTML=button('RADIO','radioModeSimple',mode==='RADIO')+button('PRO','radioModePro',mode==='PRO');work.prepend(bar);document.getElementById('radioModeSimple')?.addEventListener('click',()=>setMode('RADIO',host));document.getElementById('radioModePro')?.addEventListener('click',()=>setMode('PRO',host));}
  async function radioAction(host,action,service,s){const base=host?.baseUrl?host.baseUrl():'';const params=new URLSearchParams({action,service,frequency:String(s.frequencyHz),modulation:s.modulation,step:String(s.stepHz),gain:String(s.gain),squelch:String(s.squelch)});const response=await fetch(`${base}/nougat/v1/radio?${params}`,{cache:'no-store'});let data={};try{data=await response.json();}catch(_){data={};}if(!response.ok)throw new Error(data.error||`HTTP ${response.status}`);return data;}
  function serviceBars(host){
    document.getElementById('radioModePro')?.addEventListener('click',()=>setMode('PRO',host));
    simpleServices.forEach((x,i)=>document.getElementById(`radioSimpleService${i}`)?.addEventListener('click',()=>{simpleService=x;localStorage.setItem('nougat-web-radio-simple-service',x);renderSimple(host);}));
  }
  function renderFavorites(host){
    const list=favorites(),r=root();if(!r)return;
    r.innerHTML=`<div class="module-workspace"><div class="module-toolbar radio-mode-switch">${button('RADIO','radioModeSimple',true)}${button('PRO','radioModePro')}</div><div class="module-toolbar">${simpleServices.map((x,i)=>button(x,`radioSimpleService${i}`,x===simpleService)).join('')}</div><h2 class="module-heading">RADIO FAVORITES</h2><div class="module-result-list">${list.length?list.map((f,i)=>`<button type="button" class="module-result-row" data-radio-simple-favorite="${i}"><span>${esc(f.name||`${f.service||'Radio'} ${mhz(f.frequencyHz)} MHz`)}</span><span>${esc(mhz(f.frequencyHz))} MHz</span><span>${esc(f.modulation||'')}</span></button>`).join(''):'<div class="module-empty">No saved radio favorites yet.</div>'}</div><div class="module-toolbar">${button('Clear Favorites','radioSimpleClearFavorites')}</div><div class="module-output" id="radioSimpleStatus">Select a saved preset to return to that receiver profile.</div></div>`;
    serviceBars(host);
    r.querySelectorAll('[data-radio-simple-favorite]').forEach(row=>row.addEventListener('click',()=>{const item=favorites()[Number(row.dataset.radioSimpleFavorite)];if(!item)return;const s=state();s.frequencyHz=Math.max(1000,Number(item.frequencyHz)||s.frequencyHz);if(item.modulation)s.modulation=String(item.modulation);if(Number(item.stepHz)>0)s.stepHz=Number(item.stepHz);save(s);simpleService=item.service&&item.service!=='Favorites'?String(item.service):'Local';localStorage.setItem('nougat-web-radio-simple-service',simpleService);renderSimple(host);}));
    document.getElementById('radioSimpleClearFavorites')?.addEventListener('click',()=>{saveFavorites([]);renderFavorites(host);});
  }
  function renderSimple(host){
    if(simpleService==='Favorites')return renderFavorites(host);
    let s=preset(simpleService,state());
    const r=root();if(!r)return;r.innerHTML=`<div class="module-workspace"><div class="module-toolbar radio-mode-switch">${button('RADIO','radioModeSimple',true)}${button('PRO','radioModePro')}</div><div class="module-toolbar">${simpleServices.map((x,i)=>button(x,`radioSimpleService${i}`,x===simpleService)).join('')}</div><div class="radio-simple-face"><span class="module-badge">${esc(simpleService.toUpperCase())}</span><strong id="radioSimpleFrequency">${esc(mhz(s.frequencyHz))} MHz</strong><span>${esc(s.modulation)} • step ${s.stepHz>=1000?`${s.stepHz/1000} kHz`:`${s.stepHz} Hz`}</span></div><div class="module-toolbar">${button('◀','radioSimpleDown')}${button('LISTEN','radioSimpleListen',true)}${button('STOP','radioSimpleStop')}${button('▶','radioSimpleUp')}${button('FAVORITE','radioSimpleFavorite')}</div><div class="module-output" id="radioSimpleStatus">${simpleService==='Internet'?'Internet Radio ready.':'Simple receiver ready. Hardware receive starts only through the Nougat host bridge.'}</div></div>`;
    serviceBars(host);
    const refresh=()=>{s=state();const el=document.getElementById('radioSimpleFrequency');if(el)el.textContent=`${mhz(s.frequencyHz)} MHz`;};
    document.getElementById('radioSimpleDown')?.addEventListener('click',()=>{s=state();s.frequencyHz=Math.max(1000,s.frequencyHz-s.stepHz);save(s);refresh();});
    document.getElementById('radioSimpleUp')?.addEventListener('click',()=>{s=state();s.frequencyHz=Math.min(6000000000,s.frequencyHz+s.stepHz);save(s);refresh();});
    document.getElementById('radioSimpleListen')?.addEventListener('click',async()=>{const out=document.getElementById('radioSimpleStatus');if(simpleService==='Internet'){host?.playUrl&&host.playUrl('https://ice1.somafm.com/groovesalad-128-mp3','SomaFM Groove Salad');return;}out.textContent='Starting receiver through Nougat host...';try{const data=await radioAction(host,'receive',simpleService,state());out.textContent=data.status||'Receiver started.';}catch(err){out.textContent=`Radio host bridge unavailable: ${err.message||err}`;}});
    document.getElementById('radioSimpleStop')?.addEventListener('click',async()=>{const p=document.getElementById('player');if(p){p.pause();p.removeAttribute('src');p.load();}const out=document.getElementById('radioSimpleStatus');try{const data=await radioAction(host,'stop',simpleService,state());out.textContent=data.status||'Radio stopped.';}catch(_){out.textContent='Browser playback stopped. Host RF stop is unavailable through the current bridge.';}});
    document.getElementById('radioSimpleFavorite')?.addEventListener('click',()=>{const f=favorites();s=state();const name=`${simpleService} ${mhz(s.frequencyHz)} MHz`;if(!f.some(v=>Number(v.frequencyHz)===Number(s.frequencyHz)&&v.modulation===s.modulation))f.push({name,service:simpleService,frequencyHz:s.frequencyHz,modulation:s.modulation,stepHz:s.stepHz});saveFavorites(f);document.getElementById('radioSimpleStatus').textContent=`Saved ${name}.`;});
  }
  window.NougatWebModules.activate=function(name,host){if(name!=='radio')return previous(name,host);if(mode==='RADIO')return renderSimple(host);const result=previous(name,host);queueMicrotask(()=>addModeBar(host));return result;};
})();