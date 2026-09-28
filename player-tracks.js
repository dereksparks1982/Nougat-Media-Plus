(() => {
  'use strict';
  const player=document.getElementById('player');
  const frame=document.querySelector('.video-frame');
  const transport=document.querySelector('.transport-row');
  const fullscreen=document.getElementById('fullscreenButton');
  if(!player||!frame||!transport||!fullscreen)return;

  let subtitleObjectUrl='';
  let menuMode='subs';
  let subtitleDelay=0;
  let trackNotice='';

  function esc(v){return String(v??'').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));}
  function ensureButtons(){
    if(document.getElementById('audioTracksButton'))return;
    const audio=document.createElement('button');audio.type='button';audio.className='sheet-button transport-button track-button';audio.id='audioTracksButton';audio.textContent='AUDIO';audio.title='Audio tracks';
    const subs=document.createElement('button');subs.type='button';subs.className='sheet-button transport-button track-button';subs.id='subtitleTracksButton';subs.textContent='SUBS';subs.title='Subtitles';
    fullscreen.insertAdjacentElement('beforebegin',audio);fullscreen.insertAdjacentElement('beforebegin',subs);
    audio.addEventListener('click',()=>showMenu('audio'));subs.addEventListener('click',()=>showMenu('subs'));
  }
  function ensureMenu(){let menu=document.getElementById('playerTrackMenu');if(menu)return menu;menu=document.createElement('div');menu.id='playerTrackMenu';menu.className='player-track-menu';menu.hidden=true;frame.appendChild(menu);return menu;}
  function audioTracks(){try{return player.audioTracks?Array.from(player.audioTracks):[];}catch(_){return [];}}
  function textTracks(){try{return player.textTracks?Array.from(player.textTracks):[];}catch(_){return [];}}
  function subtitleTracks(){return textTracks().filter(t=>t.kind==='subtitles'||t.kind==='captions');}
  function closeMenu(){const menu=document.getElementById('playerTrackMenu');if(menu)menu.hidden=true;}
  function audioMenu(){const tracks=audioTracks();return `<div class="player-track-head"><strong>AUDIO TRACKS</strong><button type="button" class="sheet-button" data-track-close>Close</button></div><div class="player-track-list">${tracks.length?tracks.map((t,i)=>`<button type="button" class="sheet-button${t.enabled?' active-tool':''}" data-audio-track="${i}">${esc(t.label||t.language||`Track ${i+1}`)}</button>`).join(''):'<div class="player-track-empty">This browser does not expose selectable audio tracks for the current stream.</div>'}</div>`;}
  function subtitleMenu(){
    const tracks=subtitleTracks();
    return `<div class="player-track-head"><strong>SUBTITLES</strong><button type="button" class="sheet-button" data-track-close>Close</button></div><div class="player-track-list"><button type="button" class="sheet-button${tracks.every(t=>t.mode!=='showing')?' active-tool':''}" data-subtitle-off>Off</button>${tracks.map((t,i)=>`<button type="button" class="sheet-button${t.mode==='showing'?' active-tool':''}" data-subtitle-track="${i}">${esc(t.label||t.language||`Subtitle ${i+1}`)}</button>`).join('')}<button type="button" class="sheet-button" data-subtitle-load>Load Subtitle File</button></div><div class="player-track-list player-sub-delay"><button type="button" class="sheet-button" data-subtitle-earlier>Delay Earlier</button><button type="button" class="sheet-button" data-subtitle-reset>Reset ${subtitleDelay?`(${subtitleDelay>0?'+':''}${subtitleDelay.toFixed(1)}s)`:''}</button><button type="button" class="sheet-button" data-subtitle-later>Delay Later</button></div><div class="player-track-note">${esc(trackNotice||'Embedded tracks appear only when the browser exposes them. Local WebVTT and SRT files can be loaded directly. Delay adjusts exposed browser cues in 0.5-second steps.')}</div><input type="file" id="subtitleFilePicker" accept=".vtt,.srt,text/vtt,application/x-subrip" hidden>`;
  }
  function shiftSubtitleCues(delta){
    const tracks=subtitleTracks().filter(t=>t.mode==='showing');const targets=tracks.length?tracks:subtitleTracks();let changed=0;
    for(const track of targets){const cues=track.cues;if(!cues)continue;for(const cue of Array.from(cues)){try{const start=Math.max(0,Number(cue.startTime)+delta);const end=Math.max(start+.01,Number(cue.endTime)+delta);cue.startTime=start;cue.endTime=end;changed++;}catch(_){}}}
    if(changed){subtitleDelay+=delta;trackNotice=`Adjusted ${changed} subtitle cue${changed===1?'':'s'} to ${subtitleDelay>0?'+':''}${subtitleDelay.toFixed(1)} seconds.`;}else trackNotice='The selected browser subtitle track does not expose editable cues, so its timing was not changed.';
    showMenu('subs');
  }
  function resetSubtitleDelay(){if(!subtitleDelay){trackNotice='Subtitle delay is already reset.';showMenu('subs');return;}const delta=-subtitleDelay;subtitleDelay=0;const tracks=subtitleTracks().filter(t=>t.mode==='showing');const targets=tracks.length?tracks:subtitleTracks();let changed=0;for(const track of targets){if(!track.cues)continue;for(const cue of Array.from(track.cues)){try{const start=Math.max(0,Number(cue.startTime)+delta);const end=Math.max(start+.01,Number(cue.endTime)+delta);cue.startTime=start;cue.endTime=end;changed++;}catch(_){}}}trackNotice=changed?'Subtitle delay reset.':'The selected browser subtitle track did not expose editable cues.';showMenu('subs');}
  function bindMenu(){
    const menu=ensureMenu();menu.querySelector('[data-track-close]')?.addEventListener('click',closeMenu);
    menu.querySelectorAll('[data-audio-track]').forEach(btn=>btn.addEventListener('click',()=>{const tracks=audioTracks(),pick=Number(btn.dataset.audioTrack);tracks.forEach((t,i)=>{try{t.enabled=i===pick;}catch(_){}});showMenu('audio');}));
    menu.querySelector('[data-subtitle-off]')?.addEventListener('click',()=>{subtitleTracks().forEach(t=>t.mode='disabled');trackNotice='Subtitles off.';showMenu('subs');});
    const subs=subtitleTracks();menu.querySelectorAll('[data-subtitle-track]').forEach(btn=>btn.addEventListener('click',()=>{const pick=Number(btn.dataset.subtitleTrack);subs.forEach((t,i)=>{t.mode=i===pick?'showing':'disabled';});trackNotice=`Subtitle track ${pick+1} selected.`;showMenu('subs');}));
    const picker=menu.querySelector('#subtitleFilePicker');menu.querySelector('[data-subtitle-load]')?.addEventListener('click',()=>picker?.click());picker?.addEventListener('change',e=>loadSubtitle(e.target.files?.[0]));
    menu.querySelector('[data-subtitle-earlier]')?.addEventListener('click',()=>shiftSubtitleCues(-.5));
    menu.querySelector('[data-subtitle-later]')?.addEventListener('click',()=>shiftSubtitleCues(.5));
    menu.querySelector('[data-subtitle-reset]')?.addEventListener('click',resetSubtitleDelay);
  }
  function showMenu(mode){menuMode=mode;const menu=ensureMenu();menu.innerHTML=mode==='audio'?audioMenu():subtitleMenu();menu.hidden=false;bindMenu();}
  function srtToVtt(text){return `WEBVTT\n\n${String(text||'').replace(/^\uFEFF/,'').replace(/(\d{2}:\d{2}:\d{2}),(\d{3})/g,'$1.$2')}`;}
  async function loadSubtitle(file){
    if(!file)return;
    try{
      let text=await file.text();if(/\.srt$/i.test(file.name)||!/^\s*WEBVTT/i.test(text))text=srtToVtt(text);
      if(subtitleObjectUrl)URL.revokeObjectURL(subtitleObjectUrl);subtitleObjectUrl=URL.createObjectURL(new Blob([text],{type:'text/vtt'}));
      document.querySelectorAll('track[data-nougat-local-subtitle]').forEach(x=>x.remove());subtitleDelay=0;
      const track=document.createElement('track');track.kind='subtitles';track.label=file.name;track.srclang='und';track.src=subtitleObjectUrl;track.default=true;track.dataset.nougatLocalSubtitle='1';
      track.addEventListener('load',()=>{subtitleTracks().forEach(t=>t.mode=t===track.track?'showing':'disabled');trackNotice=`Loaded ${file.name}.`;showMenu('subs');},{once:true});
      player.appendChild(track);
    }catch(err){trackNotice=`Subtitle load failed: ${err.message||err}`;showMenu('subs');}
  }

  ensureButtons();ensureMenu();
  player.addEventListener('loadedmetadata',()=>{subtitleDelay=0;trackNotice='';if(!ensureMenu().hidden)showMenu(menuMode);});
  document.addEventListener('pointerdown',e=>{const menu=document.getElementById('playerTrackMenu');if(menu&&!menu.hidden&&!menu.contains(e.target)&&!e.target.closest('#audioTracksButton,#subtitleTracksButton'))closeMenu();});
  window.addEventListener('beforeunload',()=>{if(subtitleObjectUrl)URL.revokeObjectURL(subtitleObjectUrl);});
})();