(() => {
  'use strict';
  if (!window.NougatWebModules) return;
  const previous=window.NougatWebModules.activate.bind(window.NougatWebModules);
  const root=()=>document.getElementById('moduleView');
  const esc=(v)=>String(v??'').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
  const button=(label,id,active=false,extra='')=>`<button type="button" class="sheet-button${active?' active-tool':''}" id="${id}" ${extra}>${esc(label)}</button>`;
  const bind=(id,fn)=>{const el=document.getElementById(id);if(el)el.addEventListener('click',fn);};
  const shell=(html)=>{const r=root();if(!r)return null;r.innerHTML=`<div class="module-workspace">${html}</div>`;return r.querySelector('.module-workspace');};
  const platforms=['YouTube','Vimeo','Rumble','RuTube','VK','OK'];
  let platform='YouTube';
  let hostRef=null;

  function base(){return hostRef?.baseUrl?hostRef.baseUrl():'';}
  function absoluteNougatUrl(value){
    const url=String(value||'').trim();
    if(!url)return '';
    if(/^https?:\/\//i.test(url))return url;
    if(url.startsWith('/'))return `${base()}${url}`;
    return url;
  }
  async function resolveHost(url){
    const response=await fetch(`${base()}/nougat/v1/stream?action=resolve&platform=${encodeURIComponent(platform)}&url=${encodeURIComponent(url)}`,{cache:'no-store'});
    let data={};try{data=await response.json();}catch(_){data={};}
    if(!response.ok)throw new Error(data.error||`HTTP ${response.status}`);
    return data;
  }
  function platformMatch(url){
    let host='';try{host=new URL(url).hostname.toLowerCase();}catch(_){return false;}
    const rules={YouTube:['youtube.com','youtu.be'],Vimeo:['vimeo.com'],Rumble:['rumble.com'],RuTube:['rutube.ru'],VK:['vk.com','vkvideo.ru'],OK:['ok.ru']};
    return (rules[platform]||[]).some(domain=>host===domain||host.endsWith('.'+domain));
  }
  function stopPlayer(){const p=document.getElementById('player');if(!p)return;p.pause();p.removeAttribute('src');p.load();}

  function renderStream(host){
    hostRef=host;
    shell(`<div class="module-toolbar stream-platforms">${platforms.map((p,i)=>button(p,`streamCompletePlatform${i}`,p===platform)).join('')}</div><h2 class="module-heading">STREAM • ${esc(platform.toUpperCase())}</h2><div class="module-field"><span>URL</span><input id="streamCompleteUrl" type="url" autocomplete="off" spellcheck="false" placeholder="Paste a ${esc(platform)} URL"></div><div class="module-toolbar">${button('Watch','streamCompleteWatch')}${button('Stop','streamCompleteStop')}${button('Webpage','streamCompleteWeb')}${button('Clear','streamCompleteClear')}</div><div class="module-output" id="streamCompleteOutput">Ready. Watch uses Nougat's standalone yt-dlp engine and browser compatibility bridge.</div>`);
    platforms.forEach((p,i)=>bind(`streamCompletePlatform${i}`,()=>{platform=p;renderStream(hostRef);}));
    bind('streamCompleteClear',()=>{document.getElementById('streamCompleteUrl').value='';document.getElementById('streamCompleteOutput').textContent='Ready.';});
    bind('streamCompleteStop',()=>{stopPlayer();document.getElementById('streamCompleteOutput').textContent='Stream stopped.';});
    bind('streamCompleteWeb',()=>{const url=document.getElementById('streamCompleteUrl').value.trim();if(url)window.open(url,'_blank','noopener');});
    bind('streamCompleteWatch',async()=>{
      const input=document.getElementById('streamCompleteUrl'),out=document.getElementById('streamCompleteOutput'),url=input.value.trim();
      if(!url){out.textContent='Enter a stream URL.';return;}
      let parsed;try{parsed=new URL(url);}catch(_){out.textContent='That is not a valid URL.';return;}
      if(!/^https?:$/.test(parsed.protocol)){out.textContent='Only HTTP/HTTPS stream URLs are accepted.';return;}
      out.textContent=platformMatch(url)?`Starting ${platform} through the Nougat Stream engine...`:`URL does not look like ${platform}; asking the Nougat Stream engine anyway...`;
      try{
        const data=await resolveHost(url);
        const resolved=absoluteNougatUrl(data.url||data.streamUrl||data.stream||'');
        if(resolved){
          hostRef?.playUrl&&hostRef.playUrl(resolved,data.title||`${platform} Stream`);
          out.textContent=data.status||'Nougat Stream bridge started.';
          return;
        }
        out.textContent=data.status||'Nougat host returned no playable stream URL.';
        return;
      }catch(err){
        if(/\.(m3u8|mp4|webm|ogg|ogv)(?:$|[?#])/i.test(url)){
          hostRef?.playUrl&&hostRef.playUrl(url,`${platform} Stream`);
          out.textContent='Nougat Stream engine unavailable; direct media URL sent to the browser player.';
          return;
        }
        out.textContent=`Nougat Stream engine unavailable: ${err.message||err}\n\nUse Webpage to open the source normally.`;
      }
    });
    document.getElementById('streamCompleteUrl').addEventListener('keydown',e=>{if(e.key==='Enter')document.getElementById('streamCompleteWatch').click();});
  }

  window.NougatWebModules.activate=function(name,host){if(name==='stream')return renderStream(host);return previous(name,host);};
})();