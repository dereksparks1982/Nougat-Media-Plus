/* Run manually with Node.js (not GitHub Actions): node tools/tests/test_pages_web_host.js
   Uses mocks only. No external media, network request, credentials or real storage.
*/
'use strict';
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const code = fs.readFileSync(path.join(__dirname,'../../web-host.js'),'utf8');

function environment(defaultBase, protocol='https:') {
  const contents = new Map();
  const seen = [];
  const events = new Map();
  const storage = {
    getItem:key=>contents.has(key)?contents.get(key):null,
    setItem:(key,value)=>contents.set(key,String(value)),
    removeItem:key=>contents.delete(key)
  };
  const window = {
    NOUGAT_WEB_PLAYER:{enabled:true,baseUrl:defaultBase},
    addEventListener:(name,callback)=>events.set(name,callback),
    dispatchEvent:event=>seen.push(event)
  };
  const location = { protocol };
  class CustomEvent {
    constructor(type,options={}) {this.type=type;this.detail=options.detail;}
  }
  Function('window','localStorage','location','CustomEvent','URL',code)(
    window,storage,location,CustomEvent,URL);
  return {host:window.NougatWebHost,contents,seen,events};
}
{
  const {host,contents,seen,events}=environment('https://media.example.net');
  assert.equal(host.getBase(),'https://media.example.net');
  assert.equal(host.validate('https://private.example.net/'),'https://private.example.net');
  for (const invalid of [
    'http://media.example.net','javascript:alert(1)','file:///movie',
    'https://user:password@media.example.net','https://media.example.net/admin',
    'https://media.example.net/?token=abc','https://media.example.net/#fragment',
    'https://media.example.net@evil.test/private','https://media.example.net:443/path'
  ]) assert.equal(host.validate(invalid),'',invalid);
  assert.equal(host.setServer('https://remote.example.org'),true);
  assert.equal(contents.get(host.key),'https://remote.example.org');
  assert.equal(host.getBase(),'https://remote.example.org');
  assert.equal(seen.at(-1).type,'nougat:host-changed');
  assert.equal(host.reset(),true);
  assert.equal(host.getBase(),'https://media.example.net');
  contents.set(host.key,'https://otherserver.example.net');
  events.get('storage')({key:host.key});
  assert.equal(seen.at(-1).detail.baseUrl,'https://otherserver.example.net');
}
{
  const {host,contents}=environment('https://default.example.net');
  contents.set('nougat-watch-server-v1','https://oldwatch.example.org');
  assert.equal(host.getSaved(),'https://oldwatch.example.org');
  assert.equal(contents.get(host.key),'https://oldwatch.example.org');
}
{
  const {host}=environment('', 'http:');
  assert.equal(host.getBase(),'');
  assert.equal(host.setServer('http://192.168.1.2:8080'),false);
}
console.log('PASS: host validation, sharing, legacy migration, reset and storage updates');
