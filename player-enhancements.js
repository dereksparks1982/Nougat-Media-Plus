(() => {
  'use strict';
  const player=document.getElementById('player');
  const frame=document.querySelector('.video-frame');
  const page=document.querySelector('.player-page');
  const hud=document.getElementById('playerTitleHud');
  const prevButton=document.getElementById('prevItem');
  const nextButton=document.getElementById('nextItem');
  const fullscreenButton=document.getElementById('fullscreenButton');
  if(!player||!frame||!page||!hud||!prevButton||!nextButton)return;

  let catalog=[];
  let timer=null;
  let remaining=10;
  let activityTimer=null;
  const ACTIVITY_MS=3000;

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
    document.getElementById('upNextPlay').addEventListener('click',()=>{hideUpNext();nextButton.click();});
    document.getElementById('upNextTv').addEventListener('click',()=>{hideUpNext();document.querySelector('.rail-button[data-view="library"]')?.click();document.getElementById('tvFilter')?.click();});
    document.getElementById('upNextReplay').addEventListener('click',()=>{hideUpNext();player.currentTime=0;player.play().catch(()=>{});});
    return el;
  }
  function hideUpNext(){if(timer){clearInterval(timer);timer=null;}const el=document.getElementById('upNextOverlay');if(el)el.hidden=true;}
  function showUpNext(next){
    const el=ensureOverlay();remaining=10;document.getElementById('upNextTitle').textContent=next.name;document.getElementById('upNextCountdown').textContent=`Playing in ${remaining} seconds`;el.hidden=false;showActivity();
    timer=setInterval(()=>{remaining-=1;const label=document.getElementById('upNextCountdown');if(label)label.textContent=`Playing in ${Math.max(0,remaining)} seconds`;if(remaining<=0){hideUpNext();nextButton.click();}},1000);
  }

  function ensureFullscreenOverlay(){
    let controls=document.getElementById('fullscreenTransport');if(controls)return controls;
    controls=document.createElement('div');controls.id='fullscreenTransport';controls.className='fullscreen-transport';controls.hidden=true;
    controls.innerHTML='<button type="button" class="sheet-button fullscreen-square" id="fullscreenRewind" aria-label="Rewind 10 seconds">&lt;&lt;</button><button type="button" class="sheet-button fullscreen-square" id="fullscreenPrevious" aria-label="Previous item">&lt;</button><button type="button" class="sheet-button fullscreen-square" id="fullscreenPlay" aria-label="Play or pause">^</button><button type="button" class="sheet-button fullscreen-square" id="fullscreenNext" aria-label="Next item">&gt;</button><button type="button" class="sheet-button fullscreen-square" id="fullscreenForward" aria-label="Forward 10 seconds">&gt;&gt;</button>';
    frame.appendChild(controls);
    document.getElementById('fullscreenRewind').addEventListener('click',()=>{if(Number.isFinite(player.duration))player.currentTime=Math.max(0,player.currentTime-10);showActivity();});
    document.getElementById('fullscreenPrevious').addEventListener('click',()=>{prevButton.click();showActivity();});
    document.getElementById('fullscreenPlay').addEventListener('click',()=>{if(player.paused)player.play().catch(()=>{});else player.pause();showActivity();});
    document.getElementById('fullscreenNext').addEventListener('click',()=>{nextButton.click();showActivity();});
    document.getElementById('fullscreenForward').addEventListener('click',()=>{if(Number.isFinite(player.duration))player.currentTime=Math.min(player.duration,player.currentTime+10);showActivity();});
    return controls;
  }
  function hideActivity(){
    page.classList.add('player-activity-hidden');
    const controls=document.getElementById('fullscreenTransport');if(controls)controls.hidden=true;
  }
  function showActivity(){
    page.classList.remove('player-activity-hidden');
    if(document.fullscreenElement){const controls=ensureFullscreenOverlay();controls.hidden=false;}
    if(activityTimer)clearTimeout(activityTimer);
    activityTimer=setTimeout(hideActivity,ACTIVITY_MS);
  }

  if(fullscreenButton){fullscreenButton.addEventListener('click',async(event)=>{
    if(!page.requestFullscreen)return;
    event.preventDefault();event.stopImmediatePropagation();
    try{if(document.fullscreenElement)await document.exitFullscreen();else await page.requestFullscreen();}catch(_){if(player.webkitEnterFullscreen)player.webkitEnterFullscreen();}
  },true);}
  document.addEventListener('fullscreenchange',()=>{
    const controls=ensureFullscreenOverlay();controls.hidden=!document.fullscreenElement;
    showActivity();
  });
  ['pointermove','pointerdown','touchstart'].forEach(type=>page.addEventListener(type,showActivity,{passive:true}));
  page.addEventListener('keydown',showActivity);

  player.addEventListener('ended',()=>{const next=nextEpisode();if(next)showUpNext(next);});
  player.addEventListener('play',()=>{hideUpNext();showActivity();});
  player.addEventListener('pause',showActivity);
  player.addEventListener('loadedmetadata',()=>{hideUpNext();showActivity();});
  window.addEventListener('hashchange',()=>{if(location.hash!=='#player'){hideUpNext();if(activityTimer){clearTimeout(activityTimer);activityTimer=null;}page.classList.remove('player-activity-hidden');}});
  loadCatalog();
  window.setInterval(loadCatalog,60000);
  showActivity();
})();