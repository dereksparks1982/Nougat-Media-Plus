(() => {
  'use strict';
  const cfg=window.NOUGAT_WEB_PLAYER||{};
  const light=document.getElementById('serverLight');
  const badge=document.getElementById('healthBadge');
  const refresh=document.getElementById('refreshButton');
  if(!light||!badge)return;

  let lastOnline=null;
  let checking=false;
  let timer=0;
  const base=()=>String(cfg.baseUrl||'').trim().replace(/\/+$/,'');

  function state(kind,text){
    light.classList.remove('online','busy','offline');
    light.classList.add(kind);
    badge.textContent=text;
  }
  async function check(){
    if(checking||document.hidden)return;
    checking=true;
    try{
      const response=await fetch(`${base()}/nougat/v1/health`,{cache:'no-store'});
      if(!response.ok)throw new Error(`HTTP ${response.status}`);
      const data=await response.json();
      const total=Number(data.total)||0,available=Number(data.available);
      state('online',Number.isFinite(available)&&total>0?`${available}/${total}`:'Online');
      const recovered=lastOnline===false;
      lastOnline=true;
      if(recovered&&refresh&&!refresh.disabled){
        badge.textContent='Reconnected';
        setTimeout(()=>refresh.click(),150);
      }
    }catch(_){
      state('offline',navigator.onLine?'Server Offline':'Network Offline');
      lastOnline=false;
    }finally{checking=false;}
  }
  function schedule(){if(timer)clearInterval(timer);timer=setInterval(check,30000);}

  window.addEventListener('online',()=>{state('busy','Reconnecting');check();});
  window.addEventListener('offline',()=>{lastOnline=false;state('offline','Network Offline');});
  document.addEventListener('visibilitychange',()=>{if(!document.hidden)check();});
  window.addEventListener('pageshow',check);
  schedule();
  setTimeout(check,2500);
})();