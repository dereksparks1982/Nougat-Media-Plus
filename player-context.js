(() => {
  'use strict';
  const frame=document.querySelector('.video-frame');
  const player=document.getElementById('player');
  const playPause=document.getElementById('playPause');
  const fullscreen=document.getElementById('fullscreenButton');
  if(!frame||!player||!playPause||!fullscreen)return;

  function subtitleTracks(){try{return Array.from(player.textTracks||[]).filter(t=>t.kind==='subtitles'||t.kind==='captions');}catch(_){return [];}}
  function subtitlesShowing(){return subtitleTracks().some(t=>t.mode==='showing');}
  function toggleSubtitles(){const tracks=subtitleTracks();if(!tracks.length)return false;const turnOn=!subtitlesShowing();tracks.forEach((t,i)=>{t.mode=turnOn&&i===0?'showing':'disabled';});return true;}
  function close(){document.getElementById('playerContextMenu')?.remove();}
  function makeButton(label,fn,disabled=false){const b=document.createElement('button');b.type='button';b.className='player-context-item';b.textContent=label;b.disabled=disabled;b.addEventListener('click',()=>{close();fn();});return b;}
  function show(event){
    event.preventDefault();close();
    const menu=document.createElement('div');menu.id='playerContextMenu';menu.className='player-context-menu';
    menu.append(
      makeButton(player.paused?'Play':'Pause',()=>playPause.click()),
      makeButton(subtitlesShowing()?'Subtitles Off':'Subtitles On',()=>toggleSubtitles(),!subtitleTracks().length),
      makeButton('Previous Chapter',()=>document.getElementById('prevChapter')?.click(),document.getElementById('prevChapter')?.disabled!==false),
      makeButton('Next Chapter',()=>document.getElementById('nextChapter')?.click(),document.getElementById('nextChapter')?.disabled!==false),
      makeButton('Audio Tracks',()=>document.getElementById('audioTracksButton')?.click(),!document.getElementById('audioTracksButton')),
      makeButton('Subtitles…',()=>document.getElementById('subtitleTracksButton')?.click(),!document.getElementById('subtitleTracksButton')),
      makeButton(document.fullscreenElement?'Exit Fullscreen':'Fullscreen',()=>fullscreen.click())
    );
    frame.appendChild(menu);
    const rect=frame.getBoundingClientRect();const maxX=Math.max(8,rect.width-190),maxY=Math.max(8,rect.height-250);
    menu.style.left=`${Math.max(8,Math.min(maxX,event.clientX-rect.left))}px`;
    menu.style.top=`${Math.max(8,Math.min(maxY,event.clientY-rect.top))}px`;
  }

  frame.addEventListener('contextmenu',show);
  document.addEventListener('pointerdown',e=>{const menu=document.getElementById('playerContextMenu');if(menu&&!menu.contains(e.target))close();});
  document.addEventListener('keydown',e=>{if(e.key==='Escape')close();});
  window.addEventListener('hashchange',close);
})();