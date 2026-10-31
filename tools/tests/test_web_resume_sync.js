/* Offline regression checks. Run manually: node tools/tests/test_web_resume_sync.js
   No browser automation, media files, network requests or GitHub Actions. */
'use strict';
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
const vm=require('node:vm');
const source=fs.readFileSync(path.join(__dirname,'../../web-resume.js'),'utf8');
const data=new Map();
const events=new Map();
const dispatched=[];
const storage={
  getItem:key=>data.has(key)?data.get(key):null,
  setItem:(key,val)=>data.set(key,String(val)),
  removeItem:key=>data.delete(key)
};
const window={
  NougatWebHost:{getDefault:()=> 'https://first.example.net'},
  addEventListener:(event,fn)=>events.set(event,fn),
  dispatchEvent:evt=>dispatched.push(evt)
};
class CustomEvent{constructor(type,options){this.type=type;this.detail=options.detail;}}
const context={window,localStorage:storage,location:{origin:'https://viewer.example'},CustomEvent,console,URL};
data.set('nougat-web-resume-v2',JSON.stringify({
  film:{position_ms:60000,duration_ms:7200000,updated:1000,completed:false,title:'Film'}
}));
data.set('nougat-watch-position-v1:https://first.example.net',JSON.stringify({
  episode:{seconds:35,updated:2000}
}));
vm.runInNewContext(source,context);
const resume=window.NougatWebResume;
assert.equal(resume.read('https://first.example.net').film.position_ms,60000);
assert.equal(resume.read('https://first.example.net').episode.position_ms,35000);
assert.equal(resume.read('https://second.example.net').film,undefined,
  'global legacy resume must never contaminate another host');
assert.equal(resume.set('https://first.example.net','film',{
  position_ms:65000,duration_ms:7200000,updated:3000,title:'Film'}),true);
assert.equal(resume.read('https://first.example.net').film.position_ms,65000);
assert.equal(resume.set('https://second.example.net','film',{
  position_ms:12000,duration_ms:500000,updated:4000,title:'Other Film'}),true);
assert.equal(resume.read('https://first.example.net').film.position_ms,65000);
assert.equal(resume.read('https://second.example.net').film.position_ms,12000);
assert.equal(resume.set('https://first.example.net','__proto__',{}),false);
assert.equal(resume.set('https://first.example.net','invalid',{position_ms:Infinity}),false);
for(let i=0;i<225;i++)resume.set('https://first.example.net','item'+i,{
  position_ms:10000,duration_ms:100000,updated:10000+i
});
assert.equal(Object.keys(resume.read('https://first.example.net')).length,200);
events.get('storage')({key:resume.key('https://first.example.net')});
assert.equal(dispatched.at(-1).type,'nougat:resume-changed');
console.log('PASS: migration, host isolation, bounded history and shared-browser updates');
