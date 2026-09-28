(() => {
  'use strict';
  const player=document.getElementById('player');
  const hud=document.getElementById('playerTitleHud');
  const seek=document.getElementById('seekControl');
  const back=document.getElementById('back30');
  const nextItem=document.getElementById('nextItem');
  if(!player||!hud||!seek||!back||!nextItem)return;

  let chapters=[];
  let catalog=[];
  const base=()=>String((window.NOUGAT_WEB_PLAYER||{}).baseUrl||'').trim().replace(/\/+$/,'');

  function ensureButtons(){
    let prev=document.getElementById('prevChapter'),next=document.getElementById('nextChapter');
    if(prev&&next)return {prev,next};
    prev=document.createElement('button');prev.type='button';prev.className='sheet-button transport-button chapter-button';prev.id='prevChapter';prev.title='Previous chapter';prev.textContent='|◀';prev.disabled=true;
    next=document.createElement('button');next.type='button';next.className='sheet-button transport-button chapter-button';next.id='nextChapter';next.title='Next chapter';next.textContent='▶|';next.disabled=true;
    back.insertAdjacentElement('beforebegin',prev);
    nextItem.insertAdjacentElement('beforebegin',next);
    prev.addEventListener('click',()=>jumpChapter(-1));next.addEventListener('click',()=>jumpChapter(1));
    return {prev,next};
  }
  function ensureMarks(){
    let layer=document.getElementById('chapterMarks');if(layer)return layer;
    const row=seek.closest('.seek-row');if(!row)return null;
    const wrap=document.createElement('span');wrap.className='seek-chapter-wrap';
    seek.replaceWith(wrap);wrap.appendChild(seek);
    layer=document.createElement('span');layer.id='chapterMarks';layer.className='chapter-marks';wrap.appendChild(layer);return layer;
  }
  function normalize(raw){
    const rows=Array.isArray(raw)?raw:Array.isArray(raw?.chapters)?raw.chapters:[];
    return rows.map((c,i)=>({title:String(c.title||c.name||`Chapter ${i+1}`),start:Number(c.start_seconds??c.start??c.time??0)})).filter(c=>Number.isFinite(c.start)&&c.start>=0).sort((a,b)=>a.start-b.start);
  }
  function textTrackChapters(){
    const found=[];
    for(const track of player.textTracks||[]){
      if(track.kind!=='chapters'||!track.cues)continue;
      for(const cue of track.cues)found.push({title:String(cue.text||`Chapter ${found.length+1}`),start:Number(cue.startTime)||0});
    }
    return found.sort((a,b)=>a.start-b.start);
  }
  function renderMarks(){
    const layer=ensureMarks();if(!layer)return;layer.textContent='';
    const duration=Number(player.duration)||0;
    const marks=chapters.length&&duration>0?chapters.filter(c=>c.start>0&&c.start<duration).map(c=>({pct:c.start/duration*100,real:true,title:c.title})):[25,50,75].map(pct=>({pct,real:false,title:'Chapter-style mark'}));
    marks.forEach(mark=>{const tick=document.createElement('i');tick.className=mark.real?'chapter-mark real':'chapter-mark decorative';tick.style.left=`${Math.max(0,Math.min(100,mark.pct))}%`;tick.title=mark.title;layer.appendChild(tick);});
    const buttons=ensureButtons();buttons.prev.disabled=!chapters.length;buttons.next.disabled=!chapters.length;
  }
  function currentChapterIndex(){
    const now=Number(player.currentTime)||0;let index=-1;
    chapters.forEach((chapter,i)=>{if(chapter.start<=now+0.2)index=i;});return index;
  }
  function jumpChapter(direction){
    if(!chapters.length)return;
    let index=currentChapterIndex();
    if(direction<0){const current=index>=0?chapters[index]:null;if(current&&player.currentTime-current.start>3){}else index=Math.max(0,index-1);}
    else index=Math.min(chapters.length-1,index+1);
    const target=chapters[Math.max(0,index)];if(target){player.currentTime=Math.max(0,target.start);player.play().catch(()=>{});}
  }
  async function loadCatalog(){
    try{const r=await fetch(`${base()}/nougat/v1/catalog`,{cache:'no-store'});const data=await r.json();if(r.ok&&Array.isArray(data.items))catalog=data.items.map(x=>({id:String(x.id||''),name:String(x.name||x.title||x.filename||'')}));}catch(_){}
  }
  async function loadChapters(){
    chapters=[];const embedded=textTrackChapters();if(embedded.length){chapters=embedded;renderMarks();return;}
    const title=hud.textContent.trim();const item=catalog.find(x=>x.name===title);
    if(item?.id){
      try{const r=await fetch(`${base()}/nougat/v1/chapters?id=${encodeURIComponent(item.id)}`,{cache:'no-store'});let data={};try{data=await r.json();}catch(_){}if(r.ok)chapters=normalize(data);}catch(_){}
    }
    renderMarks();
  }

  ensureButtons();ensureMarks();renderMarks();
  player.addEventListener('loadedmetadata',loadChapters);
  player.addEventListener('durationchange',renderMarks);
  loadCatalog().then(loadChapters);
  setInterval(loadCatalog,60000);
})();