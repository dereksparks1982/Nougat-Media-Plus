/* Run manually with Node: node tools/tests/test_pages_service_worker.js
   Simulates a private GitHub Pages installation. No real network or media. */
'use strict';
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
const vm=require('node:vm');
const code=fs.readFileSync(path.join(__dirname,'../../sw.js'),'utf8');
const scope='https://viewer.example/Nougat-Media-Plus/';
const listener=new Map();
const resources=new Map();
const cached=new Map();
let skipped=false,claimed=false,offline=false;
const cache={
  async put(url,value){cached.set(String(url),value);},
  async match(url){return cached.get(String(url))||undefined;}
};
const caches={
  async open(){return cache;},
  async keys(){return ['nougat-web-player-v42-old','other-app-cache'];},
  async delete(name){return name!=='other-app-cache';}
};
const response={ok:true,type:'basic',clone(){return this;}};
async function fetch(request) {
  const uri=String(typeof request==='string'?request:request.url);
  if(offline)throw Error('offline');
  if(uri.includes('/modules-system-tmdb.js'))throw Error('optional unavailable');
  return response;
}
const self={
  registration:{scope},
  location:{origin:'https://viewer.example'},
  addEventListener:(type,handler)=>listener.set(type,handler),
  skipWaiting:async()=>{skipped=true;},
  clients:{claim:async()=>{claimed=true;}}
};
const context={self,caches,fetch,URL,Response:{error:()=>({offline:true})},console,Promise,Set,Map};
vm.runInNewContext(code,context);
function event(type,arg={}) {
  let task;
  const e={...arg,waitUntil(p){task=p;},respondWith(p){task=p;}};
  listener.get(type)(e);
  return task;
}
(async()=>{
  await event('install');
  assert(skipped,'service worker should install when core files exist');
  assert(cached.has(scope+'watch/index.html'));
  assert(cached.has(scope+'web-resume.js'));
  await event('activate');
  assert(claimed);
  const request=(url,destination='document')=>({
    method:'GET',url,destination,cache:'default',mode:'navigate',
    headers:{get:()=>null}
  });
  assert.equal(event('fetch',{request:request('https://remote.example/nougat/v1/media?id=1')}),undefined);
  assert.equal(event('fetch',{request:request(scope+'nougat/v1/catalog')}),undefined);
  assert.equal(event('fetch',{request:request(scope+'watch/index.html?token=1')}),undefined);
  offline=true;
  assert.equal(await event('fetch',{request:request(scope+'watch/index.html')}),response);
  assert.equal(await event('fetch',{request:request(scope+'index.html')}),response);
  assert.equal(event('fetch',{request:request(scope+'private-video.mp4','video')}),undefined);
  console.log('PASS: scoped app shell, API bypass, no private caching, offline player');
})().catch(error=>{console.error(error);process.exitCode=1;});
