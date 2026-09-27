(() => {
  'use strict';
  if (!window.NougatWebModules) return;

  const previous = window.NougatWebModules.activate.bind(window.NougatWebModules);
  const esc = (v) => String(v ?? '').replace(/[&<>"']/g, (c) => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
  const root = () => document.getElementById('moduleView');
  const button = (label,id,active=false,extra='') => `<button type="button" class="sheet-button${active?' active-tool':''}" id="${id}" ${extra}>${esc(label)}</button>`;
  const bind = (id,fn) => { const el=document.getElementById(id); if(el) el.addEventListener('click',fn); };
  const shell = (html) => { const r=root(); if(!r)return null; r.innerHTML=`<div class="module-workspace">${html}</div>`; return r.querySelector('.module-workspace'); };

  let hostRef = null;
  let panel = 'Diagnostics';
  let lastReport = null;
  const logs = [];
  const panels = ['Diagnostics','Server','Virus Scan','Metadata','Logs'];

  function log(message){
    logs.unshift(`${new Date().toLocaleString()}  ${message}`);
    if(logs.length>120)logs.length=120;
  }
  function download(name,text,type='text/plain'){
    const blob=new Blob([text],{type});const url=URL.createObjectURL(blob);const a=document.createElement('a');a.href=url;a.download=name;a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);
  }
  async function getJson(path){
    const base=hostRef&&hostRef.baseUrl?hostRef.baseUrl():'';
    const response=await fetch(`${base}${path}`,{cache:'no-store'});
    let data={};try{data=await response.json();}catch(_){data={};}
    if(!response.ok)throw new Error(data.error||`HTTP ${response.status}`);
    return data;
  }
  async function storageEstimate(){
    if(!navigator.storage||!navigator.storage.estimate)return null;
    try{return await navigator.storage.estimate();}catch(_){return null;}
  }
  async function collectReport(){
    const report={timestamp:new Date().toISOString(),browser:{userAgent:navigator.userAgent,platform:navigator.platform||'',online:navigator.onLine,secureContext:window.isSecureContext,cpuThreads:navigator.hardwareConcurrency||null,memoryGB:navigator.deviceMemory||null,language:navigator.language||'',screen:`${screen.width}x${screen.height}`},server:{ok:false,error:'',health:null,catalogCount:null},storage:null,serviceWorker:{supported:'serviceWorker' in navigator,controlled:!!navigator.serviceWorker?.controller}};
    try{report.server.health=await getJson('/nougat/v1/health');report.server.ok=true;}catch(err){report.server.error=String(err.message||err);}
    try{const c=await getJson('/nougat/v1/catalog');report.server.catalogCount=Number(c.count??(Array.isArray(c.items)?c.items.length:0));}catch(err){if(!report.server.error)report.server.error=String(err.message||err);}
    const s=await storageEstimate();if(s)report.storage={usage:s.usage||0,quota:s.quota||0};
    lastReport=report;log(`Diagnostics completed: bridge ${report.server.ok?'online':'offline'}.`);return report;
  }
  function bytes(value){
    const n=Number(value)||0;if(n>=1073741824)return `${(n/1073741824).toFixed(2)} GB`;if(n>=1048576)return `${(n/1048576).toFixed(1)} MB`;if(n>=1024)return `${(n/1024).toFixed(1)} KB`;return `${n} B`;
  }
  function reportTable(r){
    return `<table class="diagnostic-table"><tbody>
      <tr><th>Nougat bridge</th><td>${r.server.ok?'ONLINE':'OFFLINE'}</td></tr>
      <tr><th>Service</th><td>${esc(r.server.health?.service||r.server.health?.product||'Unavailable')}</td></tr>
      <tr><th>Version</th><td>${esc(r.server.health?.version||'Unknown')}</td></tr>
      <tr><th>Catalog</th><td>${Number.isFinite(r.server.catalogCount)?`${r.server.catalogCount} item(s)`:'Unavailable'}</td></tr>
      <tr><th>Browser online</th><td>${r.browser.online?'YES':'NO'}</td></tr>
      <tr><th>Secure context</th><td>${r.browser.secureContext?'YES':'NO'}</td></tr>
      <tr><th>CPU threads</th><td>${esc(r.browser.cpuThreads??'Not reported')}</td></tr>
      <tr><th>Memory</th><td>${r.browser.memoryGB?`${esc(r.browser.memoryGB)} GB`:'Not reported'}</td></tr>
      <tr><th>Web storage</th><td>${r.storage?`${bytes(r.storage.usage)} / ${bytes(r.storage.quota)}`:'Unavailable'}</td></tr>
      <tr><th>Service worker</th><td>${r.serviceWorker.supported?(r.serviceWorker.controlled?'ACTIVE':'SUPPORTED'):'UNAVAILABLE'}</td></tr>
      ${r.server.error?`<tr><th>Bridge error</th><td>${esc(r.server.error)}</td></tr>`:''}
    </tbody></table>`;
  }

  async function renderDiagnostics(){
    const body=document.getElementById('systemBody');if(!body)return;
    body.innerHTML='<div class="module-output">Running Nougat web diagnostics...</div>';
    const report=await collectReport();
    body.innerHTML=`<h2 class="module-heading">SYSTEM • DIAGNOSTICS</h2>${reportTable(report)}<div class="module-toolbar">${button('Run Diagnostics','sysRun')}${button('Retry','sysRetry')}${button('Copy','sysCopy')}${button('Export TXT','sysTxt')}${button('Export JSON','sysJson')}${button('Support Bundle','sysBundle')}</div><div class="module-output" id="systemResult">Diagnostic Green is used only when a check returned actual healthy evidence. Unknown stays unknown.</div>`;
    bind('sysRun',renderDiagnostics);bind('sysRetry',renderDiagnostics);
    bind('sysCopy',async()=>{const text=JSON.stringify(lastReport,null,2);try{await navigator.clipboard.writeText(text);document.getElementById('systemResult').textContent='Diagnostic report copied.';}catch(err){document.getElementById('systemResult').textContent=`Copy failed: ${err.message||err}`;}});
    bind('sysTxt',()=>download('nougat-web-diagnostics.txt',diagnosticText(lastReport)));
    bind('sysJson',()=>download('nougat-web-diagnostics.json',JSON.stringify(lastReport,null,2),'application/json'));
    bind('sysBundle',()=>download('nougat-web-support-bundle.json',JSON.stringify({diagnostics:lastReport,logs},null,2),'application/json'));
  }
  function diagnosticText(r){
    if(!r)return 'No report yet.';
    return [`Nougat Web Diagnostics`, `Timestamp: ${r.timestamp}`, `Bridge: ${r.server.ok?'online':'offline'}`, `Service: ${r.server.health?.service||r.server.health?.product||'Unavailable'}`, `Version: ${r.server.health?.version||'Unknown'}`, `Catalog: ${Number.isFinite(r.server.catalogCount)?r.server.catalogCount:'Unavailable'}`, `Browser online: ${r.browser.online?'yes':'no'}`, `Secure context: ${r.browser.secureContext?'yes':'no'}`, `Platform: ${r.browser.platform||'Unknown'}`, `CPU threads: ${r.browser.cpuThreads??'Unknown'}`, `Memory GB: ${r.browser.memoryGB??'Unknown'}`, `Storage: ${r.storage?`${bytes(r.storage.usage)} / ${bytes(r.storage.quota)}`:'Unavailable'}`, r.server.error?`Error: ${r.server.error}`:''].filter(Boolean).join('\n');
  }

  async function renderServer(){
    const body=document.getElementById('systemBody');if(!body)return;
    body.innerHTML='<div class="module-output">Checking media host...</div>';
    const report=await collectReport();
    body.innerHTML=`<h2 class="module-heading">SYSTEM • SERVER</h2><div class="module-panel-grid"><div class="module-panel-card"><h3>MEDIA HOST</h3><p>${report.server.ok?'ONLINE':'OFFLINE'}</p><p>${esc(report.server.health?.service||'Nougat Web Player')}</p></div><div class="module-panel-card"><h3>CATALOG</h3><p>${Number.isFinite(report.server.catalogCount)?`${report.server.catalogCount} indexed item(s)`:'Unavailable'}</p></div></div><div class="module-toolbar">${button('Start Server','sysStart',false,'disabled title="The web client cannot start a host that is offline."')}${button('Stop Server','sysStop',false,'disabled title="Remote Stop is intentionally unavailable because it would sever this web session."')}${button('Refresh Server','sysRefresh')}</div><div class="module-output" id="serverStatus">${report.server.ok?'The Nougat media host is reachable.':'The host must be started on the Nougat machine before this browser can connect.'}</div>`;
    bind('sysRefresh',async()=>{document.getElementById('serverStatus').textContent='Refreshing server and catalog...';if(hostRef?.refresh)await hostRef.refresh();await renderServer();});
  }

  function renderMetadata(){
    const body=document.getElementById('systemBody');if(!body)return;
    const items=hostRef?.getItems?hostRef.getItems():[];
    const movies=items.filter(x=>x.type==='Movie');const tv=items.filter(x=>x.type==='Episode');const withYear=items.filter(x=>Number(x.year)>0);const withPoster=items.filter(x=>x.poster);
    body.innerHTML=`<h2 class="module-heading">SYSTEM • METADATA</h2><table class="diagnostic-table"><tbody><tr><th>Total media</th><td>${items.length}</td></tr><tr><th>Movies</th><td>${movies.length}</td></tr><tr><th>TV episodes</th><td>${tv.length}</td></tr><tr><th>Production year present</th><td>${withYear.length}</td></tr><tr><th>Poster URL present</th><td>${withPoster.length}</td></tr></tbody></table><div class="module-toolbar">${button('Refresh Metadata','metadataRefresh')}${button('Open Library','metadataLibrary')}</div><div class="module-output">This reports metadata actually exposed by the Nougat web catalog. It does not invent TMDb matches that the bridge did not return.</div>`;
    bind('metadataRefresh',async()=>{if(hostRef?.refresh)await hostRef.refresh();renderMetadata();});bind('metadataLibrary',()=>hostRef?.setView&&hostRef.setView('library'));
  }

  async function sha256(file){
    const hash=await crypto.subtle.digest('SHA-256',await file.arrayBuffer());return [...new Uint8Array(hash)].map(b=>b.toString(16).padStart(2,'0')).join('');
  }
  function renderSecurity(){
    const body=document.getElementById('systemBody');if(!body)return;
    body.innerHTML=`<h2 class="module-heading">SYSTEM • VIRUS SCAN</h2><div class="module-toolbar">${button('Scan File','securityFile')}${button('Scan Folder','securityFolder')}${button('Scan Movies','securityMovies')}${button('Scan TV','securityTv')}${button('Quick Scan','securityQuick')}${button('System Scan','securitySystem')}${button('Scan Again','securityAgain')}${button('History','securityHistory')}</div><input id="securityFilePicker" type="file" hidden><input id="securityFolderPicker" type="file" webkitdirectory multiple hidden><div class="module-output" id="securityOutput">WARN ME FIRST policy active. The browser can inspect selected files and calculate hashes. It does not claim a malware verdict unless the Nougat host security bridge actually returns one.</div>`;
    const output=document.getElementById('securityOutput');
    bind('securityFile',()=>document.getElementById('securityFilePicker').click());bind('securityFolder',()=>document.getElementById('securityFolderPicker').click());
    document.getElementById('securityFilePicker').addEventListener('change',async(e)=>{const file=e.target.files?.[0];if(!file)return;output.textContent='Hashing selected file...';try{const hash=await sha256(file);const entry={when:new Date().toISOString(),name:file.name,size:file.size,type:file.type||'unknown',sha256:hash,verdict:'NO HOST MALWARE VERDICT'};let history=[];try{history=JSON.parse(localStorage.getItem('nougat-web-security-history')||'[]')||[];}catch(_){}history.unshift(entry);localStorage.setItem('nougat-web-security-history',JSON.stringify(history.slice(0,100)));log(`Security file inspection: ${file.name}`);output.textContent=`File: ${file.name}\nSize: ${bytes(file.size)}\nType: ${file.type||'unknown'}\nSHA-256: ${hash}\nVerdict: NO HOST MALWARE VERDICT\n\nThe file was not uploaded by this web inspection.`;}catch(err){output.textContent=`Inspection failed: ${err.message||err}`;}});
    document.getElementById('securityFolderPicker').addEventListener('change',(e)=>{const files=[...(e.target.files||[])];const total=files.reduce((n,f)=>n+f.size,0);log(`Security folder inventory: ${files.length} file(s).`);output.textContent=`Folder selection: ${files.length} file(s)\nTotal size: ${bytes(total)}\n\nBrowser folder inventory complete. No malware verdict was invented.`;});
    const bridgeAction=async(action)=>{output.textContent=`Requesting ${action} from Nougat host security bridge...`;try{const data=await getJson(`/nougat/v1/security?action=${encodeURIComponent(action)}`);output.textContent=data.status||JSON.stringify(data,null,2);log(`Host security action ${action} completed.`);}catch(err){output.textContent=`Host security bridge unavailable: ${err.message||err}\n\nThe standalone security engines remain on the Nougat host; this browser will not pretend the scan ran.`;}};
    bind('securityMovies',()=>bridgeAction('movies'));bind('securityTv',()=>bridgeAction('tv'));bind('securityQuick',()=>bridgeAction('quick'));bind('securitySystem',()=>bridgeAction('system'));bind('securityAgain',()=>bridgeAction('repeat'));
    bind('securityHistory',()=>{let history=[];try{history=JSON.parse(localStorage.getItem('nougat-web-security-history')||'[]')||[];}catch(_){}output.textContent=history.length?history.map(x=>`${x.when}  ${x.name}  ${bytes(x.size)}\n${x.sha256}\n${x.verdict}`).join('\n\n'):'No browser security inspection history yet.';});
  }

  function renderLogs(){
    const body=document.getElementById('systemBody');if(!body)return;
    body.innerHTML=`<h2 class="module-heading">SYSTEM • LOGS</h2><div class="module-toolbar">${button('Copy','logsCopy')}${button('Export TXT','logsExport')}${button('Clear Web Logs','logsClear')}${button('Clear Web Cache','logsClearCache')}</div><div class="module-output" id="logsOutput">${esc(logs.length?logs.join('\n'):'No Nougat web session log entries yet.')}</div>`;
    bind('logsCopy',async()=>{try{await navigator.clipboard.writeText(logs.join('\n'));}catch(_){}});bind('logsExport',()=>download('nougat-web-session.log',logs.join('\n')));bind('logsClear',()=>{logs.length=0;renderLogs();});bind('logsClearCache',async()=>{const out=document.getElementById('logsOutput');try{const keys=await caches.keys();await Promise.all(keys.map(k=>caches.delete(k)));out.textContent=`Cleared ${keys.length} Nougat web cache(s). Reloading...`;setTimeout(()=>location.reload(),400);}catch(err){out.textContent=`Cache clear failed: ${err.message||err}`;}});
  }

  async function renderPanel(){
    if(panel==='Server')return renderServer();
    if(panel==='Virus Scan')return renderSecurity();
    if(panel==='Metadata')return renderMetadata();
    if(panel==='Logs')return renderLogs();
    return renderDiagnostics();
  }
  function renderSystem(host){
    hostRef=host;
    shell(`<div class="module-toolbar search-tabs">${panels.map((p,i)=>button(p,`systemTab${i}`,panel===p)).join('')}</div><div id="systemBody"></div>`);
    panels.forEach((p,i)=>bind(`systemTab${i}`,()=>{panel=p;renderSystem(hostRef);}));
    renderPanel();
  }

  window.NougatWebModules.activate=function(name,host){
    if(name==='system')return renderSystem(host);
    return previous(name,host);
  };
})();