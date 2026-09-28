(() => {
  'use strict';

  const KEY='nougat-web-catalog-cache-v1';
  const originalFetch=window.fetch.bind(window);
  let forceNextCatalog=false;

  function isCatalogRequest(input,init={}){
    try{
      const url=new URL(typeof input==='string'?input:input.url,location.href);
      const method=String(init.method||(typeof input==='object'&&input.method)||'GET').toUpperCase();
      return method==='GET'&&url.pathname.endsWith('/nougat/v1/catalog');
    }catch(_){return false;}
  }

  function readCache(){
    try{
      const value=JSON.parse(localStorage.getItem(KEY)||'null');
      if(!value||!value.payload||!Array.isArray(value.payload.items))return null;
      return value;
    }catch(_){return null;}
  }

  function writeCache(payload){
    try{localStorage.setItem(KEY,JSON.stringify({savedAt:Date.now(),payload}));}catch(_){}
  }

  function cachedResponse(value){
    return new Response(JSON.stringify(value.payload),{status:200,statusText:'OK',headers:{'Content-Type':'application/json','X-Nougat-Catalog-Cache':'hit'}});
  }

  async function refreshInBackground(input,init){
    try{
      const response=await originalFetch(input,init);
      if(!response.ok)return;
      const clone=response.clone();
      const data=await clone.json();
      if(data&&data.ok&&Array.isArray(data.items)){
        const before=readCache();
        writeCache(data);
        const changed=JSON.stringify(before?.payload?.items||[])!==JSON.stringify(data.items);
        window.dispatchEvent(new CustomEvent('nougat:catalog-cache-updated',{detail:{changed,count:data.items.length}}));
      }
    }catch(_){}
  }

  window.fetch=async function(input,init={}){
    if(!isCatalogRequest(input,init))return originalFetch(input,init);
    const cached=readCache();
    if(cached&&!forceNextCatalog){
      refreshInBackground(input,init);
      return cachedResponse(cached);
    }
    forceNextCatalog=false;
    try{
      const response=await originalFetch(input,init);
      if(response.ok){
        try{const data=await response.clone().json();if(data&&data.ok&&Array.isArray(data.items))writeCache(data);}catch(_){}
      }
      return response;
    }catch(err){
      const fallback=readCache();
      if(fallback)return cachedResponse(fallback);
      throw err;
    }
  };

  const refreshButton=document.getElementById('refreshButton');
  if(refreshButton)refreshButton.addEventListener('click',()=>{forceNextCatalog=true;},{capture:true});

  window.addEventListener('nougat:catalog-cache-updated',event=>{
    if(!event.detail?.changed)return;
    const status=document.getElementById('catalogStatus');
    if(status&&!status.textContent.includes('REFRESH AVAILABLE'))status.dataset.backgroundRefresh='ready';
  });
})();