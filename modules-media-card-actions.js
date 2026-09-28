(() => {
  'use strict';

  const MATCH_KEY='nougat-web-manual-matches-v1';
  const cfg=window.NOUGAT_WEB_PLAYER||{};
  const base=()=>String(cfg.baseUrl||'').trim().replace(/\/+$/,'');
  let catalog=[];
  let observer=null;
  let menu=null;
  let dialog=null;
  let activeItem=null;

  const esc=v=>String(v??'').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
  const readMatches=()=>{try{return JSON.parse(localStorage.getItem(MATCH_KEY)||'{}')||{};}catch(_){return {};}};
  const writeMatches=value=>{try{localStorage.setItem(MATCH_KEY,JSON.stringify(value));}catch(_){}};
  const normalize=raw=>({
    id:String(raw.id||''),
    name:String(raw.name||raw.title||raw.filename||'Untitled media'),
    filename:String(raw.filename||raw.path||''),
    type:/^(tv|episode|television)$/i.test(String(raw.kind||raw.type||''))?'Episode':'Movie',
    year:Number(raw.year||raw.production_year)||0,
    poster:String(raw.poster||raw.posterUrl||raw.image||raw.imageUrl||'')
  });

  async function loadCatalog(){
    try{
      const r=await fetch(`${base()}/nougat/v1/catalog`,{cache:'no-store'});
      const data=await r.json();
      if(!r.ok||!data.ok)throw new Error(data.error||`HTTP ${r.status}`);
      catalog=(Array.isArray(data.items)?data.items:[]).map(normalize).filter(x=>x.id);
    }catch(_){catalog=[];}
    applyAll();
  }

  function findItemForCard(card){
    const id=card?.dataset?.nougatMediaId;
    if(id){const hit=catalog.find(x=>x.id===id);if(hit)return hit;}
    const label=String(card?.getAttribute?.('aria-label')||'');
    const name=label.startsWith('Play ')?label.slice(5):String(card?.querySelector?.('.media-title')?.textContent||'');
    const matches=readMatches();
    return catalog.find(x=>x.name===name||matches[x.id]?.name===name)||null;
  }

  function posterUrl(value,item){
    const raw=String(value||'').trim();
    if(raw){
      if(/^https?:\/\//i.test(raw)||raw.startsWith('data:')||raw.startsWith('blob:'))return raw;
      if(raw.startsWith('/'))return `${base()}${raw}`;
      return raw;
    }
    return item.id?`${base()}/nougat/v1/artwork?id=${encodeURIComponent(item.id)}`:'';
  }

  function applyCard(card,item){
    if(!card||!item)return;
    card.dataset.nougatMediaId=item.id;
    const match=readMatches()[item.id]||null;
    const shown={name:match?.name||item.name,year:Number(match?.year)||item.year,type:match?.type||item.type,poster:match?.poster||item.poster};
    const title=card.querySelector('.media-title');if(title)title.textContent=shown.name;
    const meta=card.querySelector('.media-meta');if(meta)meta.textContent=[shown.type==='Episode'?'TV Episode':'Movie',shown.year||''].filter(Boolean).join(' • ');
    card.setAttribute('aria-label',`Play ${shown.name}`);
    card.title=`${shown.name}${shown.year?` (${shown.year})`:''}${match?' • MANUAL MATCH':''}`;
    const art=card.querySelector('.media-art');if(art&&shown.poster){
      const url=posterUrl(shown.poster,item);
      let img=art.querySelector('img');
      if(!img){art.textContent='';img=document.createElement('img');img.alt='';img.loading='lazy';art.appendChild(img);}
      if(img.dataset.manualPoster!==url){img.dataset.manualPoster=url;img.src=url;}
    }
  }

  function applyRows(){
    const matches=readMatches();
    document.querySelectorAll('.library-row').forEach(row=>{
      let id=row.dataset.nougatMediaId;
      let item=id?catalog.find(x=>x.id===id):null;
      if(!item){const name=String(row.querySelector('span')?.textContent||'');item=catalog.find(x=>x.name===name||matches[x.id]?.name===name);}
      if(!item)return;
      row.dataset.nougatMediaId=item.id;
      const m=matches[item.id];if(!m)return;
      const cells=row.querySelectorAll('span');
      if(cells[0])cells[0].textContent=m.name||item.name;
      if(cells[1])cells[1].textContent=(m.type||item.type)==='Episode'?'Television':'Movie';
      if(cells[2])cells[2].textContent=Number(m.year)||item.year||'';
    });
  }

  function applyAll(){
    if(!catalog.length)return;
    const matches=readMatches();
    document.querySelectorAll('.media-card').forEach(card=>{
      let item=findItemForCard(card);
      if(!item){const name=String(card.querySelector('.media-title')?.textContent||'');item=catalog.find(x=>x.name===name||matches[x.id]?.name===name);}
      if(item)applyCard(card,item);
    });
    applyRows();
  }

  function ensureUi(){
    if(!menu){
      menu=document.createElement('div');menu.className='nougat-card-menu';menu.hidden=true;
      menu.innerHTML='<button data-card-action="play">Play / Resume</button><button data-card-action="info">Information</button><button data-card-action="fix">Fix Match</button><button data-card-action="clear">Clear Manual Match</button><button data-card-action="art">Refresh Artwork</button>';
      document.body.appendChild(menu);
      menu.addEventListener('click',event=>{const action=event.target.closest('[data-card-action]')?.dataset.cardAction;if(action)runAction(action);});
    }
    if(!dialog){
      dialog=document.createElement('dialog');dialog.className='nougat-match-dialog';
      dialog.innerHTML='<form method="dialog"><h2>FIX MATCH</h2><label>TITLE<input id="nougatMatchTitle"></label><label>YEAR<input id="nougatMatchYear" type="number" min="1800" max="2200"></label><label>TYPE<select id="nougatMatchType"><option value="Movie">Movie</option><option value="Episode">Television</option></select></label><label>TMDb ID<input id="nougatMatchTmdb" inputmode="numeric"></label><label>POSTER URL / PATH<input id="nougatMatchPoster"></label><div class="nougat-match-actions"><button value="cancel">Cancel</button><button type="button" id="nougatMatchSave">Save Match</button></div><p id="nougatMatchStatus"></p></form>';
      document.body.appendChild(dialog);
      dialog.querySelector('#nougatMatchSave').addEventListener('click',saveMatch);
    }
  }

  function showMenu(event,item){
    ensureUi();activeItem=item;menu.hidden=false;
    const x=Math.min(event.clientX,window.innerWidth-menu.offsetWidth-10),y=Math.min(event.clientY,window.innerHeight-menu.offsetHeight-10);
    menu.style.left=`${Math.max(8,x)}px`;menu.style.top=`${Math.max(8,y)}px`;
    const clear=menu.querySelector('[data-card-action="clear"]');clear.disabled=!readMatches()[item.id];
  }
  function hideMenu(){if(menu)menu.hidden=true;}

  function infoText(item){
    const m=readMatches()[item.id]||{};const title=m.name||item.name;const type=m.type||item.type;const year=Number(m.year)||item.year||0;
    return `${title}\n${type}${year?` • ${year}`:''}\n${item.filename||'No filename exposed'}\nMedia ID: ${item.id}${m.tmdbId?`\nTMDb: ${m.tmdbId}`:''}${readMatches()[item.id]?'\nManual match: ACTIVE':''}`;
  }

  async function postMatch(payload){
    if(!base())return false;
    try{const r=await fetch(`${base()}/nougat/v1/metadata/manual-match`,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(payload),cache:'no-store'});return r.ok;}catch(_){return false;}
  }

  function openFix(){
    ensureUi();const m=readMatches()[activeItem.id]||{};
    dialog.querySelector('#nougatMatchTitle').value=m.name||activeItem.name;
    dialog.querySelector('#nougatMatchYear').value=m.year||activeItem.year||'';
    dialog.querySelector('#nougatMatchType').value=m.type||activeItem.type;
    dialog.querySelector('#nougatMatchTmdb').value=m.tmdbId||'';
    dialog.querySelector('#nougatMatchPoster').value=m.poster||activeItem.poster||'';
    dialog.querySelector('#nougatMatchStatus').textContent='';dialog.showModal();
  }

  async function saveMatch(){
    if(!activeItem)return;
    const match={name:dialog.querySelector('#nougatMatchTitle').value.trim()||activeItem.name,year:Number(dialog.querySelector('#nougatMatchYear').value)||0,type:dialog.querySelector('#nougatMatchType').value,tmdbId:dialog.querySelector('#nougatMatchTmdb').value.trim(),poster:dialog.querySelector('#nougatMatchPoster').value.trim(),updated:Date.now()};
    const all=readMatches();all[activeItem.id]=match;writeMatches(all);applyAll();
    const hostSaved=await postMatch({action:'set',id:activeItem.id,...match});
    dialog.querySelector('#nougatMatchStatus').textContent=hostSaved?'Saved in browser and Nougat host.':'Saved in this browser. Nougat host manual-match endpoint is not available.';
    setTimeout(()=>dialog.close(),650);
  }

  async function clearMatch(){
    if(!activeItem)return;const all=readMatches();delete all[activeItem.id];writeMatches(all);applyAll();
    await postMatch({action:'clear',id:activeItem.id});
  }

  function refreshArtwork(){
    if(!activeItem)return;const selector=`[data-nougat-media-id="${CSS.escape(activeItem.id)}"] .media-art img`;
    document.querySelectorAll(selector).forEach(img=>{try{const u=new URL(img.src,location.href);u.searchParams.set('_nougat_refresh',Date.now());img.src=u.href;}catch(_){}});
  }

  function runAction(action){
    hideMenu();if(!activeItem)return;
    if(action==='play'){const card=[...document.querySelectorAll('.media-card')].find(x=>x.dataset.nougatMediaId===activeItem.id);card?.click();return;}
    if(action==='info'){alert(infoText(activeItem));return;}
    if(action==='fix'){openFix();return;}
    if(action==='clear'){clearMatch();return;}
    if(action==='art')refreshArtwork();
  }

  document.addEventListener('contextmenu',event=>{
    const card=event.target.closest('.media-card,.library-row');if(!card)return;
    const item=findItemForCard(card);if(!item)return;
    event.preventDefault();showMenu(event,item);
  });
  document.addEventListener('click',event=>{if(menu&&!event.target.closest('.nougat-card-menu'))hideMenu();});
  window.addEventListener('blur',hideMenu);

  ensureUi();
  observer=new MutationObserver(()=>requestAnimationFrame(applyAll));observer.observe(document.body,{childList:true,subtree:true});
  loadCatalog();
  document.getElementById('refreshButton')?.addEventListener('click',()=>setTimeout(loadCatalog,250));
})();