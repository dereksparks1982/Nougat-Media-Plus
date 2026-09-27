(() => {
  'use strict';
  const player=document.getElementById('player');
  const frame=document.querySelector('.video-frame');
  const hud=document.getElementById('playerTitleHud');
  const nextButton=document.getElementById('nextItem');
  if(!player||!frame||!hud||!nextButton)return;

  let catalog=[];
  let timer=null;
  let remaining=10;

  const base=()=>String((window.NOUGAT_WEB_PLAYER||{}).baseUrl||'').trim().replace(/\/+$/,'');
  async function loadCatalog(){
    try{const r=await fetch(`${base()}/nougat/v1/catalog`,{cache:'no-store'});const data=await r.json();if(r.ok&&data&&Array.isArray(data.items))catalog=data.items.map(x=>({id:String(x.id||''),name:String(x.name||x.title||''),type:String(x.type||x.kind||'')}));}catch(_){}
  }
  function isEpisode(item){return /episode|television|tv/i.test(String(item?.type||''));}
  function currentIndex(){const title=hud.textContent.trim();return catalog.findIndex(x=>x.name===title);}
  function nextEpisode(){const i=currentIndex();if(i<0||i+1>=catalog.length)return null;const current=catalog[i],next=catalog[i+1];return isEpisode(current)&&isEpisode(next)?next:null;}
  function ensureOverlay(){
    let el=document.getElementById('upNextOverlay');if(el)return el;
    el=document.createElement('div');el.id='upNextOverlay';el.className='up-next-overlay';el.hidden=true;
    el.innerHTML='<div class="up-next-panel"><span class="up-next-kicker">UP NEXT</span><strong id="upNextTitle"></strong><span id="upNextCountdown"></span><div class="up-next-actions"><button type="button" class="sheet-button" id="upNextPlay">PLAY NEXT</button><button type="button" class="sheet-button" id="upNextTv">TV LIBRARY</button><button type="button" class="sheet-button" id="upNextReplay">REPLAY</button></div></div>';
    frame.appendChild(el);
    document.getElementById('upNextPlay').addEventListener('click',()=>{hide();nextButton.click();});
    document.getElementById('upNextTv').addEventListener('click',()=>{hide();document.querySelector('.rail-button[data-view="library"]')?.click();document.getElementById('tvFilter')?.click();});
    document.getElementById('upNextReplay').addEventListener('click',()=>{hide();player.currentTime=0;player.play().catch(()=>{});});
    return el;
  }
  function hide(){if(timer){clearInterval(timer);timer=null;}const el=document.getElementById('upNextOverlay');if(el)el.hidden=true;}
  function show(next){
    const el=ensureOverlay();remaining=10;document.getElementById('upNextTitle').textContent=next.name;document.getElementById('upNextCountdown').textContent=`Playing in ${remaining} seconds`;el.hidden=false;
    timer=setInterval(()=>{remaining-=1;const label=document.getElementById('upNextCountdown');if(label)label.textContent=`Playing in ${Math.max(0,remaining)} seconds`;if(remaining<=0){hide();nextButton.click();}},1000);
  }

  player.addEventListener('ended',()=>{const next=nextEpisode();if(next)show(next);});
  player.addEventListener('play',hide);
  player.addEventListener('loadedmetadata',hide);
  window.addEventListener('hashchange',()=>{if(location.hash!=='#player')hide();});
  loadCatalog();
  window.setInterval(loadCatalog,60000);
})();