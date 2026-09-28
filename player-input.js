(() => {
  'use strict';
  const player=document.getElementById('player');
  const frame=document.querySelector('.video-frame');
  const playPause=document.getElementById('playPause');
  const fullscreen=document.getElementById('fullscreenButton');
  if(!player||!frame||!playPause||!fullscreen)return;

  let clickTimer=0;
  const playerActive=()=>location.hash==='#player'||document.querySelector('[data-view-panel="player"]')?.classList.contains('active');
  const editable=target=>target&&(['INPUT','TEXTAREA','SELECT','BUTTON'].includes(target.tagName)||target.isContentEditable);
  const togglePlay=()=>{if(player.paused)player.play().catch(()=>{});else player.pause();};
  const volumeBy=delta=>{player.volume=Math.max(0,Math.min(1,player.volume+delta));if(delta>0&&player.muted)player.muted=false;};

  document.addEventListener('keydown',event=>{
    if(!playerActive()||editable(event.target))return;
    if(event.key==='ArrowUp'){event.preventDefault();volumeBy(.05);return;}
    if(event.key==='ArrowDown'){event.preventDefault();volumeBy(-.05);}
  });

  player.addEventListener('click',event=>{
    if(event.button!==0)return;
    if(clickTimer)clearTimeout(clickTimer);
    clickTimer=setTimeout(()=>{clickTimer=0;togglePlay();},230);
  });
  player.addEventListener('dblclick',event=>{
    if(event.button!==0)return;
    event.preventDefault();
    if(clickTimer){clearTimeout(clickTimer);clickTimer=0;}
    fullscreen.click();
  });
  frame.addEventListener('wheel',event=>{
    if(!playerActive())return;
    event.preventDefault();
    volumeBy(event.deltaY<0?.05:-.05);
  },{passive:false});
})();