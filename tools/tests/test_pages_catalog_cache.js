/* Offline Node.js contract test: node tools/tests/test_pages_catalog_cache.js */
'use strict';
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
const vm=require('node:vm');
const source=fs.readFileSync(path.join(__dirname,'../../modules-catalog-cache.js'),'utf8');
const records=new Map();
const events={};
let fail=false;
let accepted=true;
let requests=0;
const originals=async (url)=>{
  ++requests;
  if(fail)throw new TypeError('offline');
  const id=new URL(url).hostname[0];
  return {ok:true,clone(){return {json:async()=>({ok:true,items:[{id,title:id}]})};}};
};
class FakeResponse{
  constructor(content,init){this.body=JSON.parse(content);this.status=init.status;this.headers=init.headers;}
  async json(){return this.body;}
}
const window={fetch:originals,NougatLegalAccepted:true};
const localStorage={getItem:key=>records.get(key)||null,setItem:(key,v)=>records.set(key,v)};
vm.runInNewContext(source,{window,localStorage,location:{href:'https://viewer.example/'},URL,Response:FakeResponse,Date,JSON});
(async()=>{
  const a='https://alpha.example/nougat/v1/catalog';
  const b='https://beta.example/nougat/v1/catalog';
  await window.fetch(a);
  await new Promise(resolve=>setImmediate(resolve));
  fail=true;
  const offline=await window.fetch(a);
  assert.equal((await offline.json()).items[0].title,'a');
  assert.equal(offline.headers['X-Nougat-Catalog-Cache'],'offline');
  await assert.rejects(window.fetch(b),/offline/,'other host must not read alpha catalog');
  await assert.rejects(window.fetch(a,{signal:{aborted:true}}),/offline/);
  window.NougatLegalAccepted=false;
  await assert.rejects(window.fetch(a),/offline/,'terms gate cannot be bypassed with cache');
  assert.equal(requests,5);
  console.log('PASS: host isolation, offline-only fallback and legal/cancel boundaries');
})().catch(error=>{console.error(error);process.exitCode=1;});
