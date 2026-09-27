(() => {
  'use strict';
  if(!window.NougatWebModules)return;
  const previous=window.NougatWebModules.activate.bind(window.NougatWebModules);
  const root=()=>document.getElementById('moduleView');
  const esc=v=>String(v??'').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
  const button=(label,id)=>`<button type="button" class="sheet-button" id="${id}">${esc(label)}</button>`;
  let hostRef=null;
  let observer=null;

  async function api(action,extra={}){const base=hostRef?.baseUrl?hostRef.baseUrl():'';const params=new URLSearchParams({action,...extra});const response=await fetch(`${base}/nougat/v1/p2p?${params}`,{cache:'no-store'});let data={};try{data=await response.json();}catch(_){data={};}if(!response.ok)throw new Error(data.error||`HTTP ${response.status}`);return data;}
  function augment(){const out=document.getElementById('p2pOutput');if(!out||document.getElementById('p2pNetworkAdvanced'))return;const bar=document.createElement('div');bar.className='module-toolbar';bar.innerHTML=button('Network / Advanced','p2pNetworkAdvanced');out.before(bar);document.getElementById('p2pNetworkAdvanced').addEventListener('click',renderAdvanced);}
  function renderAdvanced(){
    const out=document.getElementById('p2pOutput');if(!out)return;
    out.innerHTML=`<strong>P2P NETWORK / ADVANCED</strong>\nPeer and node administration stays associated with P2P, matching standalone Nougat.\n<div class="module-field"><span>PEER ADDRESS</span><input id="p2pPeerAddress" type="text" placeholder="host:port"></div><div class="module-field"><span>NODE PORT</span><input id="p2pNodePort" type="number" min="1" max="65535" value="7799"></div><div class="module-toolbar">${button('Node ID','p2pNodeId')}${button('Peers','p2pPeers')}${button('Add Peer','p2pAddPeer')}${button('Remove Peer','p2pRemovePeer')}${button('Start Node','p2pStartNode')}${button('Stop Node','p2pStopNode')}</div><div class="module-result-list" id="p2pNetworkResult"></div>`;
    const result=document.getElementById('p2pNetworkResult');
    const run=async(action,extra={})=>{result.innerHTML='<div class="module-empty">Working...</div>';try{const data=await api(action,extra);if(Array.isArray(data.peers)){result.innerHTML=data.peers.length?data.peers.map(p=>`<div class="module-result-row"><span>${esc(p.address||p)}</span><span>${esc(p.status||'')}</span><span>${esc(p.node||'')}</span></div>`).join(''):'<div class="module-empty">No peers returned.</div>';}else result.innerHTML=`<div class="module-output">${esc(data.status||data.node_id||data.nodeId||JSON.stringify(data,null,2))}</div>`;}catch(err){result.innerHTML=`<div class="module-output">P2P host bridge unavailable: ${esc(err.message||err)}\n\nNo node or peer change was claimed.</div>`;}};
    document.getElementById('p2pNodeId').addEventListener('click',()=>run('node-id'));document.getElementById('p2pPeers').addEventListener('click',()=>run('peers'));document.getElementById('p2pAddPeer').addEventListener('click',()=>{const peer=document.getElementById('p2pPeerAddress').value.trim();if(peer)run('add-peer',{peer});});document.getElementById('p2pRemovePeer').addEventListener('click',()=>{const peer=document.getElementById('p2pPeerAddress').value.trim();if(peer)run('remove-peer',{peer});});document.getElementById('p2pStartNode').addEventListener('click',()=>run('start-node',{port:String(Math.max(1,Math.min(65535,Number(document.getElementById('p2pNodePort').value)||7799)))}));document.getElementById('p2pStopNode').addEventListener('click',()=>run('stop-node'));
  }
  function watch(){if(observer)observer.disconnect();observer=new MutationObserver(augment);observer.observe(root(),{childList:true,subtree:true});augment();}
  function stop(){if(observer){observer.disconnect();observer=null;}}
  window.NougatWebModules.activate=function(name,host){if(name!=='search'){stop();return previous(name,host);}hostRef=host;const result=previous(name,host);queueMicrotask(watch);return result;};
})();