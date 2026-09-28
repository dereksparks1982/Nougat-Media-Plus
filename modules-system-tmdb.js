(() => {
  'use strict';
  if(!window.NougatWebModules)return;
  const previous=window.NougatWebModules.activate.bind(window.NougatWebModules);
  let hostRef=null;
  let observer=null;

  function base(){return hostRef?.baseUrl?hostRef.baseUrl():'';}
  async function action(name){
    const response=await fetch(`${base()}/nougat/v1/discover?action=${encodeURIComponent(`tmdb-${name}`)}`,{cache:'no-store'});
    let data={};try{data=await response.json();}catch(_){data={};}
    if(!response.ok)throw new Error(data.error||`HTTP ${response.status}`);
    return data;
  }
  async function run(name){
    const out=document.getElementById('systemTmdbStatus');if(!out)return;
    out.textContent=`TMDB • ${name.toUpperCase()}...`;
    try{const data=await action(name);out.textContent=data.status||data.message||`TMDb ${name} completed on the Nougat host.`;}
    catch(err){out.textContent=`TMDb host control unavailable: ${err.message||err}. No local credential change was claimed.`;}
  }
  function augment(){
    const body=document.getElementById('systemBody');if(!body)return;
    const heading=body.querySelector('.module-heading');
    if(!heading||!heading.textContent.toUpperCase().includes('METADATA')||document.getElementById('systemTmdbControls'))return;
    const block=document.createElement('div');block.id='systemTmdbControls';
    block.innerHTML=`<h3 class="module-heading">TMDB</h3><div class="module-toolbar"><button type="button" class="sheet-button" id="systemTmdbTest">TMDb Test</button><button type="button" class="sheet-button" id="systemTmdbReplace">TMDb Replace</button><button type="button" class="sheet-button" id="systemTmdbClear">TMDb Clear</button></div><div class="module-output" id="systemTmdbStatus">TMDb credential and metadata actions run only on the configured Nougat host. The browser does not store the host token.</div>`;
    body.appendChild(block);
    document.getElementById('systemTmdbTest')?.addEventListener('click',()=>run('test'));
    document.getElementById('systemTmdbReplace')?.addEventListener('click',()=>run('replace'));
    document.getElementById('systemTmdbClear')?.addEventListener('click',()=>run('clear'));
  }
  function start(){stop();queueMicrotask(()=>{augment();observer=new MutationObserver(augment);const body=document.getElementById('systemBody');if(body)observer.observe(body,{childList:true,subtree:true});});}
  function stop(){if(observer){observer.disconnect();observer=null;}}

  window.NougatWebModules.activate=function(name,host){
    if(name==='system'){hostRef=host;const result=previous(name,host);start();return result;}
    stop();return previous(name,host);
  };
})();