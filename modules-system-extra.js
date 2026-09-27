(() => {
  'use strict';
  if(!window.NougatWebModules)return;
  const previous=window.NougatWebModules.activate.bind(window.NougatWebModules);
  const root=()=>document.getElementById('moduleView');
  const esc=v=>String(v??'').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
  const button=(label,id)=>`<button type="button" class="sheet-button" id="${id}">${esc(label)}</button>`;
  let hostRef=null;
  let observer=null;

  function output(){return document.getElementById('securityOutput');}
  async function request(params){const base=hostRef?.baseUrl?hostRef.baseUrl():'';const response=await fetch(`${base}/nougat/v1/security?${params}`,{cache:'no-store'});let data={};try{data=await response.json();}catch(_){data={};}if(!response.ok)throw new Error(data.error||`HTTP ${response.status}`);return data;}
  function showScopes(){const out=output();if(!out)return;out.innerHTML=`<strong>SYSTEM SCAN</strong>\nChoose the same standalone scan scope. No scan is reported complete unless the host returns a result.\n<div class="module-toolbar system-scan-scopes">${button('Full System','securityScopeFull')}${button('Critical System Areas','securityScopeCritical')}${button('Startup Locations','securityScopeStartup')}${button('Downloads','securityScopeDownloads')}${button('Removable Drives','securityScopeRemovable')}${button('New/Changed Files (7 days)','securityScopeChanged')}</div>`;[['Full','full'],['Critical','critical'],['Startup','startup'],['Downloads','downloads'],['Removable','removable'],['Changed','changed-7-days']].forEach(([id,scope])=>document.getElementById(`securityScope${id}`)?.addEventListener('click',()=>runScope(scope)));}
  async function runScope(scope){const out=output();if(!out)return;out.textContent=`Requesting ${scope} system scan from Nougat host...`;try{const data=await request(new URLSearchParams({action:'system',scope}));out.textContent=data.status||JSON.stringify(data,null,2);}catch(err){out.textContent=`Host security bridge unavailable: ${err.message||err}\n\nThe scan did not run. No clean/safe verdict is being claimed.`;}}
  function showKey(){const out=output();if(!out)return;out.innerHTML=`<strong>THREAT INTEL KEY</strong>\nThe standalone stores this privately outside Git. The web player does not persist it.\n<div class="module-field"><span>AUTH KEY</span><input id="securityIntelKey" type="password" autocomplete="off" spellcheck="false"></div><div class="module-toolbar">${button('Send to Nougat Host','securityIntelSend')}${button('Clear Field','securityIntelClear')}</div>`;document.getElementById('securityIntelClear')?.addEventListener('click',()=>{document.getElementById('securityIntelKey').value='';});document.getElementById('securityIntelSend')?.addEventListener('click',async()=>{const key=document.getElementById('securityIntelKey').value.trim();if(!key){out.textContent='Enter the owner-supplied threat-intel key first.';return;}out.textContent='Sending key to the Nougat host over the configured HTTPS bridge. It is not stored in browser storage.';try{const base=hostRef?.baseUrl?hostRef.baseUrl():'';const response=await fetch(`${base}/nougat/v1/security/auth-key`,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({key}),cache:'no-store'});let data={};try{data=await response.json();}catch(_){data={};}if(!response.ok)throw new Error(data.error||`HTTP ${response.status}`);out.textContent=data.status||'Threat-intel key accepted by the host.';}catch(err){out.textContent=`Host key endpoint unavailable: ${err.message||err}\n\nThe key was not saved in browser storage.`;}});}
  function augment(){
    const system=document.getElementById('securitySystem');if(!system)return;
    system.textContent='System Scan ▾';
    if(!document.getElementById('securityIntelKeyButton')){const toolbar=system.closest('.module-toolbar');if(toolbar)toolbar.insertAdjacentHTML('beforeend',button('Threat Intel Key','securityIntelKeyButton'));document.getElementById('securityIntelKeyButton')?.addEventListener('click',showKey);}
  }
  function capture(event){if(event.target.closest?.('#securitySystem')){event.preventDefault();event.stopImmediatePropagation();showScopes();}}
  function startWatch(){if(observer)observer.disconnect();observer=new MutationObserver(augment);observer.observe(root(),{childList:true,subtree:true});root().addEventListener('click',capture,true);augment();}
  function stopWatch(){if(observer){observer.disconnect();observer=null;}root()?.removeEventListener('click',capture,true);}
  window.NougatWebModules.activate=function(name,host){if(name!=='system'){stopWatch();return previous(name,host);}hostRef=host;const result=previous(name,host);queueMicrotask(startWatch);return result;};
})();