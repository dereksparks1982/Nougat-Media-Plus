(() => {
  'use strict';
  if(!window.NougatWebModules)return;
  const previous=window.NougatWebModules.activate.bind(window.NougatWebModules);
  const GUIDES_API='https://iptv-org.github.io/api/guides.json';
  const stationIds=['France24French.fr','France24Arabic.fr','France24Spanish.fr','AlJazeeraArabic.qa','AlJazeeraEnglish.qa','EBSKids.kr','CMCTV.hr','AlArabyTV.qa','AlQuranAlKareemTV.sa','Russia24.ru','Russia1.ru','RussiaK.ru','ChannelOne.ru','NTV.ru','Mir24.ru','RBKTV.ru','TVCentr.ru','DumaTV.ru','VmesteRF.ru','Mir.ru','STS.ru','Moskva24.ru','SanktPeterburg.ru','Soyuz.ru','TRTWorld.tr','TRTHaber.tr','TRT1.tr','NTV.tr','TRTArabi.tr','TRTAvaz.tr','TRTBelgesel.tr','TRTTurk.tr','TGRTHaber.tr','ArirangTV.kr','NHKWorldJapan.jp','CNAInternational.sg','DDNews.in'];
  let hostRef=null;
  let guidesPromise=null;
  let observer=null;
  const esc=v=>String(v??'').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));

  function base(){return hostRef?.baseUrl?hostRef.baseUrl():'';}
  function selectedIndex(){const active=document.querySelector('.world-tv-card.active-tool[data-world-index]');return active?Number(active.dataset.worldIndex)||0:0;}
  function selectedName(){return document.querySelector('.world-tv-card.active-tool .world-tv-name')?.textContent?.trim()||'World TV';}
  function time(unix){const n=Number(unix);return n?new Date(n*1000).toLocaleTimeString([],{hour:'numeric',minute:'2-digit'}):'';}
  function formatEntry(title,start,end){if(!title)return 'Unavailable';return `${time(start)}${end?` - ${time(end)}`:''}  ${title}`;}
  function renderGuide(data,source='Nougat host'){
    const out=document.getElementById('worldTvGuidePanel');if(!out)return;
    const currentTitle=data.current_title??data.CURRENT_TITLE??data.current?.title??'';
    const currentStart=Number(data.current_start??data.CURRENT_START??data.current?.start??0);
    const currentEnd=Number(data.current_end??data.CURRENT_END??data.current?.end??0);
    const nextTitle=data.next_title??data.NEXT_TITLE??data.next?.title??'';
    const nextStart=Number(data.next_start??data.NEXT_START??data.next?.start??0);
    const nextEnd=Number(data.next_end??data.NEXT_END??data.next?.end??0);
    const guideSource=data.source??data.SOURCE??source;
    out.innerHTML=`<div class="module-panel-grid"><div class="module-panel-card"><h3>NOW</h3><p>${esc(formatEntry(currentTitle,currentStart,currentEnd))}</p></div><div class="module-panel-card"><h3>NEXT</h3><p>${esc(formatEntry(nextTitle,nextStart,nextEnd))}</p></div></div><div class="status-line">GUIDE SOURCE • ${esc(guideSource||source)}</div>`;
  }
  function renderUnavailable(message){const out=document.getElementById('worldTvGuidePanel');if(out)out.innerHTML=`<div class="module-output world-tv-status">${esc(message)}</div>`;}
  async function hostGuide(id){
    const response=await fetch(`${base()}/nougat/v1/world-tv?action=guide&id=${encodeURIComponent(id)}`,{cache:'no-store'});
    let data={};try{data=await response.json();}catch(_){data={};}
    if(!response.ok)throw new Error(data.error||`HTTP ${response.status}`);
    const ok=data.ok!==false&&(data.current_title||data.CURRENT_TITLE||data.next_title||data.NEXT_TITLE||data.current||data.next);
    if(!ok)throw new Error(data.error||'No host guide data');
    return data;
  }
  async function guides(){
    if(!guidesPromise)guidesPromise=fetch(GUIDES_API,{cache:'force-cache'}).then(r=>{if(!r.ok)throw new Error(`guide directory HTTP ${r.status}`);return r.json();});
    return guidesPromise;
  }
  function xmltvEpoch(value){
    const text=String(value||'').trim();if(!text)return 0;
    const m=text.match(/^(\d{4})(\d{2})(\d{2})(\d{2})(\d{2})(\d{2})?\s*([+-]\d{4})?/);if(!m)return 0;
    const iso=`${m[1]}-${m[2]}-${m[3]}T${m[4]}:${m[5]}:${m[6]||'00'}${m[7]?`${m[7].slice(0,3)}:${m[7].slice(3)}`:'Z'}`;
    const ms=Date.parse(iso);return Number.isFinite(ms)?Math.floor(ms/1000):0;
  }
  async function browserGuide(id){
    const rows=await guides();const matches=(Array.isArray(rows)?rows:[]).filter(x=>x&&x.channel===id);
    if(!matches.length)throw new Error('No registered XMLTV guide source');
    const now=Math.floor(Date.now()/1000);
    for(const match of matches){
      const sources=Array.isArray(match.sources)?match.sources.slice(0,3):[];
      for(const source of sources){
        const url=String(source?.url||'');if(!/^https?:\/\//i.test(url))continue;
        try{
          const response=await fetch(url,{cache:'no-store'});if(!response.ok)continue;
          const xml=await response.text();const doc=new DOMParser().parseFromString(xml,'application/xml');if(doc.querySelector('parsererror'))continue;
          const wanted=new Set([id,String(match.site_id||'')]);let current=null,next=null;
          for(const p of doc.getElementsByTagName('programme')){
            if(!wanted.has(String(p.getAttribute('channel')||'')))continue;
            const start=xmltvEpoch(p.getAttribute('start')),end=xmltvEpoch(p.getAttribute('stop'));const title=p.getElementsByTagName('title')[0]?.textContent?.trim()||'';
            if(!start||!end||!title)continue;
            if(start<=now&&now<end)current={title,start,end};else if(start>now&&(!next||start<next.start))next={title,start,end};
          }
          if(current||next)return {current,next,source:source.host||new URL(url).hostname};
        }catch(_){}
      }
    }
    throw new Error('Registered XMLTV sources were not browser-readable');
  }
  async function refreshGuide(){
    const index=selectedIndex(),id=stationIds[index],name=selectedName();
    if(!id){renderUnavailable('No guide identity is registered for the selected World TV station.');return;}
    renderUnavailable(`Loading ${name} guide...`);
    try{renderGuide(await hostGuide(id),'Nougat host');return;}catch(_){}
    try{renderGuide(await browserGuide(id),'IPTV-org XMLTV');return;}catch(err){renderUnavailable(`${name} guide unavailable: ${err.message||err}. No schedule is being invented.`);}
  }
  function augment(){
    const workspace=document.querySelector('.world-tv-workspace');if(!workspace)return;
    const toolbar=workspace.querySelector('.module-toolbar');if(toolbar&&!document.getElementById('worldTvGuideRefresh')){
      const btn=document.createElement('button');btn.type='button';btn.className='sheet-button';btn.id='worldTvGuideRefresh';btn.textContent='Guide';btn.addEventListener('click',refreshGuide);toolbar.appendChild(btn);
    }
    if(!document.getElementById('worldTvGuidePanel')){
      const heading=workspace.querySelector('.module-heading');if(heading){const panel=document.createElement('div');panel.id='worldTvGuidePanel';panel.innerHTML='<div class="module-output world-tv-status">Select a station, then press Guide for current and next program data.</div>';heading.insertAdjacentElement('afterend',panel);}
    }
    workspace.querySelectorAll('[data-world-index]').forEach(card=>{if(card.dataset.guideBound)return;card.dataset.guideBound='1';card.addEventListener('click',()=>setTimeout(refreshGuide,0));});
  }
  function start(){stop();queueMicrotask(()=>{augment();observer=new MutationObserver(augment);const r=document.getElementById('moduleView');if(r)observer.observe(r,{childList:true,subtree:true});});}
  function stop(){if(observer){observer.disconnect();observer=null;}}

  window.NougatWebModules.activate=function(name,host){
    if(name==='worldtv'){hostRef=host;const result=previous(name,host);start();return result;}
    stop();return previous(name,host);
  };
})();