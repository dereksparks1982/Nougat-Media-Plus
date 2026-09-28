(() => {
  'use strict';
  if(!window.NougatWebModules)return;

  const previous=window.NougatWebModules.activate.bind(window.NougatWebModules);
  const root=()=>document.getElementById('moduleView');
  const esc=v=>String(v??'').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
  let hostRef=null;
  let observer=null;
  let cellularProfile=localStorage.getItem('nougat-web-cellular-profile')||'2G GSM';

  const button=(label,id,active=false)=>`<button type="button" class="sheet-button${active?' active-tool':''}" id="${id}">${esc(label)}</button>`;
  const radioMode=()=>localStorage.getItem('nougat-web-radio-mode')==='PRO'?'PRO':'RADIO';
  const simpleService=()=>localStorage.getItem('nougat-web-radio-simple-service')||'Internet';
  const base=()=>hostRef?.baseUrl?hostRef.baseUrl():'';

  async function action(name,extra={}){
    const params=new URLSearchParams({action:name,...extra});
    const response=await fetch(`${base()}/nougat/v1/radio?${params}`,{cache:'no-store'});
    let data={};try{data=await response.json();}catch(_){data={};}
    if(!response.ok)throw new Error(data.error||`HTTP ${response.status}`);
    return data;
  }

  function setService(name){localStorage.setItem('nougat-web-radio-simple-service',name);window.NougatWebModules.activate('radio',hostRef);}
  function setMode(name){localStorage.setItem('nougat-web-radio-mode',name);if(name==='RADIO')localStorage.setItem('nougat-web-radio-simple-service','Internet');window.NougatWebModules.activate('radio',hostRef);}

  function renderCellular(){
    const r=root();if(!r)return;
    r.innerHTML=`<div class="module-workspace">
      <div class="module-toolbar radio-mode-switch">${button('RADIO','cellRadio',true)}${button('PRO','cellPro')}</div>
      <div class="module-toolbar">${button('Internet','cellInternet')}${button('FM','cellFm')}${button('Weather','cellWeather')}${button('Cellular Lab','cellLab',true)}${button('TV Antenna Scan','cellTvScan')}</div>
      <h2 class="module-heading">CELLULAR LAB • RECEIVE-ONLY</h2>
      <div class="module-toolbar">${button('2G GSM','cell2g',cellularProfile==='2G GSM')}${button('1G Foundation','cell1g',cellularProfile==='1G Foundation')}${button('Detect Hardware','cellDetect')}${button('Scan Receive Bands','cellScan')}${button('Stop','cellStop')}</div>
      <div class="module-panel-grid">
        <div class="module-panel-card"><h3>${esc(cellularProfile.toUpperCase())}</h3><p>${cellularProfile==='2G GSM'?'Receive-only GSM-era spectrum/lab profile. Demodulation or hardware work must come from the Nougat host radio backend.':'Architecture foundation for first-generation analog cellular study. No transmit path is exposed in the web client.'}</p></div>
        <div class="module-panel-card"><h3>SAFETY STATE</h3><p>Web Cellular Lab is receive-only. No RF transmit controls are exposed. Encrypted/private communications are not decoded by this page.</p></div>
        <div class="module-panel-card"><h3>HARDWARE</h3><p id="cellHardware">Waiting for Nougat host capability detection.</p></div>
      </div>
      <div class="module-output" id="cellStatus">CELLULAR LAB READY\n${esc(cellularProfile)}\nHardware detection and scanning require the local Nougat Radio bridge.</div>
    </div>`;

    document.getElementById('cellRadio')?.addEventListener('click',()=>setMode('RADIO'));
    document.getElementById('cellPro')?.addEventListener('click',()=>setMode('PRO'));
    document.getElementById('cellInternet')?.addEventListener('click',()=>setService('Internet'));
    document.getElementById('cellFm')?.addEventListener('click',()=>setService('FM'));
    document.getElementById('cellWeather')?.addEventListener('click',()=>setService('Weather'));
    document.getElementById('cellLab')?.addEventListener('click',()=>setService('Cellular Lab'));
    document.getElementById('cell2g')?.addEventListener('click',()=>{cellularProfile='2G GSM';localStorage.setItem('nougat-web-cellular-profile',cellularProfile);renderCellular();});
    document.getElementById('cell1g')?.addEventListener('click',()=>{cellularProfile='1G Foundation';localStorage.setItem('nougat-web-cellular-profile',cellularProfile);renderCellular();});
    document.getElementById('cellTvScan')?.addEventListener('click',()=>openTvScan());
    document.getElementById('cellDetect')?.addEventListener('click',async()=>{
      const out=document.getElementById('cellStatus'),hardware=document.getElementById('cellHardware');out.textContent='Detecting receive hardware through Nougat host...';
      try{const data=await action('cellular-detect',{profile:cellularProfile});const text=data.status||'Hardware detection completed.';out.textContent=text;hardware.textContent=data.hardware||data.device||data.backend||text;}
      catch(err){out.textContent=`Cellular host bridge unavailable: ${err.message||err}\n\nNo hardware-ready claim is being made.`;hardware.textContent='Host capability endpoint unavailable.';}
    });
    document.getElementById('cellScan')?.addEventListener('click',async()=>{
      const out=document.getElementById('cellStatus');out.textContent=`Starting ${cellularProfile} receive-only scan through Nougat host...`;
      try{const data=await action('cellular-scan',{profile:cellularProfile});out.textContent=data.status||JSON.stringify(data,null,2);}
      catch(err){out.textContent=`Cellular scan bridge unavailable: ${err.message||err}\n\nThe browser did not pretend an RF scan occurred.`;}
    });
    document.getElementById('cellStop')?.addEventListener('click',async()=>{
      const out=document.getElementById('cellStatus');
      try{const data=await action('cellular-stop',{profile:cellularProfile});out.textContent=data.status||'Cellular receive scan stopped.';}
      catch(_){out.textContent='No active host Cellular Lab session was confirmed.';}
    });
  }

  function openTvScan(){
    hostRef?.setView&&hostRef.setView('livetv');
    setTimeout(()=>document.getElementById('liveScan')?.click(),120);
  }

  function injectButtons(){
    const r=root();if(!r||!r.querySelector('.module-workspace'))return;
    const bars=[...r.querySelectorAll('.module-toolbar')];
    const serviceBar=bars.find(bar=>/Internet|FM|AM|Weather/.test(bar.textContent||''));
    if(serviceBar&&!document.getElementById('radioCellularLab')){
      const cell=document.createElement('button');cell.type='button';cell.className='sheet-button';cell.id='radioCellularLab';cell.textContent='Cellular Lab';cell.addEventListener('click',()=>setService('Cellular Lab'));serviceBar.appendChild(cell);
    }
    const target=serviceBar||bars[0];
    if(target&&!document.getElementById('radioTvAntennaScan')){
      const tv=document.createElement('button');tv.type='button';tv.className='sheet-button';tv.id='radioTvAntennaScan';tv.textContent='TV Antenna Scan';tv.addEventListener('click',openTvScan);target.appendChild(tv);
    }
  }

  function startObserver(){if(observer)observer.disconnect();observer=new MutationObserver(()=>injectButtons());const r=root();if(r)observer.observe(r,{childList:true,subtree:true});queueMicrotask(injectButtons);}
  function stopObserver(){if(observer){observer.disconnect();observer=null;}}

  window.NougatWebModules.activate=function(name,host){
    if(name!=='radio'){stopObserver();return previous(name,host);}
    hostRef=host;
    if(radioMode()==='RADIO'&&simpleService()==='Cellular Lab'){stopObserver();return renderCellular();}
    const result=previous(name,host);queueMicrotask(startObserver);return result;
  };
})();