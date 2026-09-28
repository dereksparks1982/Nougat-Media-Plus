(() => {
  'use strict';
  const player=document.getElementById('player');
  const status=document.getElementById('playerStatus');
  if(!player||!status)return;

  let wasPlayingBeforeHide=false;
  let hiddenAt=0;
  let recovering=false;

  function hasMedia(){return !!String(player.currentSrc||player.src||'').trim();}
  function playerActive(){return location.hash==='#player'||document.querySelector('[data-view-panel="player"]')?.classList.contains('active');}
  function setStatus(text){status.textContent=String(text||'').toUpperCase();}
  function needsReload(){return hasMedia()&&(player.error||player.networkState===HTMLMediaElement.NETWORK_NO_SOURCE);}

  async function resumePlayback(){
    if(!wasPlayingBeforeHide||!playerActive()||!hasMedia())return;
    try{await player.play();setStatus('PLAYING');}
    catch(_){setStatus('READY • TAP PLAY TO RESUME');}
  }

  async function recover(reason='resume'){
    if(recovering||!hasMedia())return;
    recovering=true;
    const position=Number.isFinite(player.currentTime)?Math.max(0,player.currentTime):0;
    const source=player.currentSrc||player.src;
    try{
      if(needsReload()){
        setStatus(`RECONNECTING AFTER ${reason}`);
        player.src=source;
        player.load();
        await new Promise(resolve=>{
          const done=()=>{player.removeEventListener('loadedmetadata',done);player.removeEventListener('error',done);resolve();};
          player.addEventListener('loadedmetadata',done,{once:true});
          player.addEventListener('error',done,{once:true});
          setTimeout(done,5000);
        });
        if(position>0&&Number.isFinite(player.duration))player.currentTime=Math.min(position,Math.max(0,player.duration-1));
      }
      await resumePlayback();
    }finally{recovering=false;}
  }

  document.addEventListener('visibilitychange',()=>{
    if(document.hidden){wasPlayingBeforeHide=hasMedia()&&!player.paused&&!player.ended;hiddenAt=Date.now();return;}
    if(!hiddenAt)return;
    const sleptFor=Date.now()-hiddenAt;hiddenAt=0;
    if(sleptFor>3000||needsReload())recover('PHONE SLEEP');else resumePlayback();
  });

  window.addEventListener('pageshow',event=>{
    if(event.persisted&&hasMedia())recover('PAGE RESTORE');
  });
  window.addEventListener('online',()=>{
    if(needsReload())recover('NETWORK RETURN');
  });
  player.addEventListener('error',()=>{
    if(!navigator.onLine)setStatus('NETWORK OFFLINE • PLAYBACK POSITION SAVED');
  });
})();