(() => {
  'use strict';
  if(!('mediaSession' in navigator)||!('MediaMetadata' in window))return;
  const player=document.getElementById('player');
  const hud=document.getElementById('playerTitleHud');
  const prev=document.getElementById('prevItem');
  const next=document.getElementById('nextItem');
  if(!player||!hud)return;

  let catalog=[];
  let lastPositionUpdate=0;
  const cfg=window.NOUGAT_WEB_PLAYER||{};
  const base=()=>String(cfg.baseUrl||'').trim().replace(/\/+$/,'');

  function normalize(raw){
    const kind=String(raw.kind||raw.type||'').toLowerCase();
    return {
      id:String(raw.id||''),
      name:String(raw.name||raw.title||raw.filename||'Untitled media'),
      type:/^(tv|episode|television)$/.test(kind)?'Television':'Movie',
      year:Number(raw.year||raw.production_year)||0,
      poster:String(raw.poster||raw.posterUrl||raw.image||raw.imageUrl||'')
    };
  }
  function posterUrl(item){
    if(!item)return '';
    const raw=String(item.poster||'').trim();
    if(/^https?:\/\//i.test(raw)||raw.startsWith('data:')||raw.startsWith('blob:'))return raw;
    if(raw.startsWith('/'))return `${base()}${raw}`;
    if(raw)return raw;
    return item.id&&base()?`${base()}/nougat/v1/artwork?id=${encodeURIComponent(item.id)}`:'';
  }
  async function loadCatalog(){
    try{
      const response=await fetch(`${base()}/nougat/v1/catalog`,{cache:'no-store'});
      const data=await response.json();
      if(response.ok&&data&&Array.isArray(data.items))catalog=data.items.map(normalize).filter(x=>x.id);
    }catch(_){}
  }
  function currentItem(){
    const title=hud.textContent.trim();
    return catalog.find(x=>x.name===title)||catalog.find(x=>title.startsWith(`${x.name} •`))||null;
  }
  function updateMetadata(){
    const title=hud.textContent.trim();
    if(!title||title==='Nothing selected')return;
    const item=currentItem();
    const artwork=posterUrl(item);
    const metadata={title,artist:'Nougat Media Plus',album:item?[item.type,item.year||''].filter(Boolean).join(' • '):'Nougat Media Plus'};
    if(artwork)metadata.artwork=[{src:artwork,sizes:'512x512'}];
    try{navigator.mediaSession.metadata=new MediaMetadata(metadata);}catch(_){try{delete metadata.artwork;navigator.mediaSession.metadata=new MediaMetadata(metadata);}catch(__){}}
  }
  function setPlaybackState(state){try{navigator.mediaSession.playbackState=state;}catch(_){};}
  function updatePosition(force=false){
    const now=Date.now();if(!force&&now-lastPositionUpdate<1000)return;lastPositionUpdate=now;
    const duration=Number(player.duration),position=Number(player.currentTime),rate=Number(player.playbackRate)||1;
    if(!Number.isFinite(duration)||duration<=0||!Number.isFinite(position))return;
    try{navigator.mediaSession.setPositionState({duration,playbackRate:Math.max(.1,rate),position:Math.max(0,Math.min(duration,position))});}catch(_){}
  }
  function seekBy(delta){if(!Number.isFinite(player.duration))return;player.currentTime=Math.max(0,Math.min(player.duration,(Number(player.currentTime)||0)+delta));updatePosition(true);}
  function bindAction(name,fn){try{navigator.mediaSession.setActionHandler(name,fn);}catch(_){};}

  bindAction('play',()=>player.play().catch(()=>{}));
  bindAction('pause',()=>player.pause());
  bindAction('seekbackward',details=>seekBy(-Math.max(1,Number(details.seekOffset)||10)));
  bindAction('seekforward',details=>seekBy(Math.max(1,Number(details.seekOffset)||10)));
  bindAction('seekto',details=>{
    const target=Number(details.seekTime);if(!Number.isFinite(target))return;
    if(details.fastSeek&&typeof player.fastSeek==='function')player.fastSeek(target);else player.currentTime=Math.max(0,Math.min(Number(player.duration)||target,target));
    updatePosition(true);
  });
  bindAction('previoustrack',()=>prev?.click());
  bindAction('nexttrack',()=>next?.click());
  bindAction('stop',()=>player.pause());

  player.addEventListener('play',()=>{setPlaybackState('playing');updateMetadata();updatePosition(true);});
  player.addEventListener('pause',()=>{setPlaybackState('paused');updatePosition(true);});
  player.addEventListener('ended',()=>setPlaybackState('none'));
  player.addEventListener('loadedmetadata',()=>{updateMetadata();updatePosition(true);});
  player.addEventListener('timeupdate',()=>updatePosition(false));
  player.addEventListener('ratechange',()=>updatePosition(true));

  new MutationObserver(updateMetadata).observe(hud,{childList:true,characterData:true,subtree:true});
  loadCatalog().then(updateMetadata);
  setInterval(loadCatalog,60000);
})();