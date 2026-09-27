(() => {
  'use strict';
  if (!window.NougatWebModules) return;

  const previous = window.NougatWebModules.activate.bind(window.NougatWebModules);
  const root = () => document.getElementById('moduleView');
  const esc = (v) => String(v ?? '').replace(/[&<>"']/g, (c) => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
  const bind = (id, fn) => { const el = document.getElementById(id); if (el) el.addEventListener('click', fn); };

  const IPTV_STREAMS = 'https://iptv-org.github.io/api/streams.json';
  const IPTV_LOGOS = 'https://iptv-org.github.io/api/logos.json';
  const ALOULA_CHANNELS = 'https://aloula.faulio.com/api/v1/channels';
  const ALOULA_PLAYER = (id) => `https://aloula.faulio.com/api/v1.1/channels/${encodeURIComponent(id)}/player`;

  let streamsPromise = null;
  let logosPromise = null;
  let selectedIndex = 0;
  let query = '';
  let region = 'ALL';

  const stations = [
    {id:'France24French.fr',name:'France 24 Français',country:'France',language:'French',url:'https://live.france24.com/hls/live/2037179/F24_FR_HI_HLS/master_5000.m3u8',home:'https://www.france24.com/fr/direct/'},
    {id:'France24Arabic.fr',name:'France 24 Arabic',country:'France',language:'Arabic',url:'https://live.france24.com/hls/live/2037222/F24_AR_HI_HLS/master_5000.m3u8',home:'https://www.france24.com/ar/live/'},
    {id:'France24Spanish.fr',name:'France 24 Español',country:'France',language:'Spanish',url:'https://live.france24.com/hls/live/2037220/F24_ES_HI_HLS/master_2300.m3u8',home:'https://www.france24.com/es/en-vivo/'},
    {id:'AlJazeeraArabic.qa',name:'Al Jazeera Arabic',country:'Qatar',language:'Arabic',url:'https://live-hls-apps-aja-fa.getaj.net/AJA/index.m3u8',home:'https://www.aljazeera.net/live/'},
    {id:'AlJazeeraEnglish.qa',name:'Al Jazeera English',country:'Qatar',language:'English',url:'https://live-hls-apps-aje-fa.getaj.net/AJE/index.m3u8',home:'https://www.aljazeera.com/live/'},
    {id:'EBSKids.kr',name:'EBS Kids',country:'South Korea',language:'Korean',url:'https://ebsonair.ebs.co.kr/ebsufamilypc/familypc1m/chunklist_w1898633944.m3u8',home:'https://www.ebs.co.kr/'},
    {id:'CMCTV.hr',name:'CMC TV',country:'Croatia',language:'Croatian',url:'https://stream.cmctv.hr:49998/cmc/live.m3u8',home:'https://cmctv.hr/'},
    {id:'AlArabyTV.qa',name:'Al Araby TV',country:'Qatar / United Kingdom',language:'Arabic',url:'https://alaraby.cdn.octivid.com/alaraby/smil:alaraby.stream.smil/chunklist.m3u8',home:'https://www.alaraby.com/live'},
    {id:'AlQuranAlKareemTV.sa',name:'Al Quran Al Kareem TV',country:'Saudi Arabia',language:'Arabic',url:'',home:'https://www.aloula.sa/',resolver:'aloula-quran'},

    {id:'Russia24.ru',name:'Russia 24',country:'Russia',language:'Russian',url:'https://stream.smotrim.ru/hls2/russia24nl_smotrim/playlist_5.m3u8',home:'https://smotrim.ru/channel/3'},
    {id:'Russia1.ru',name:'Russia 1',country:'Russia',language:'Russian',url:'https://stream.smotrim.ru/hls2/russia_hd/playlist_6.m3u8',home:'https://smotrim.ru/channel/1'},
    {id:'RussiaK.ru',name:'Russia-K',country:'Russia',language:'Russian',url:'https://stream.smotrim.ru/hls2/russia_k/playlist_5.m3u8',home:'https://smotrim.ru/channel/2'},
    {id:'ChannelOne.ru',name:'Channel One Russia',country:'Russia',language:'Russian',url:'',home:'https://www.1tv.ru/live'},
    {id:'NTV.ru',name:'NTV Russia',country:'Russia',language:'Russian',url:'https://cdn.ntv.ru/vitrina/index.m3u8',home:'https://www.ntv.ru/air/ntv/'},
    {id:'Mir24.ru',name:'MIR 24',country:'Russia',language:'Russian',url:'',home:'https://mir24.tv/live'},
    {id:'RBKTV.ru',name:'RBC TV',country:'Russia',language:'Russian',url:'',home:'https://tv.rbc.ru/'},
    {id:'TVCentr.ru',name:'TV Center',country:'Russia',language:'Russian',url:'https://tvc-hls.cdnvideo.ru/tvc-res/smil:vd9221.smil/playlist.m3u8',home:'https://www.tvc.ru/channel/onair'},
    {id:'DumaTV.ru',name:'Duma TV',country:'Russia',language:'Russian',url:'https://dumatv.iptv2022.com/playlist.m3u8',home:'https://dumatv.ru/'},
    {id:'VmesteRF.ru',name:'Vmeste RF',country:'Russia',language:'Russian',url:'',home:'https://vmeste-rf.tv/'},
    {id:'Mir.ru',name:'MIR',country:'Russia',language:'Russian',url:'',home:'https://mir24.tv/'},
    {id:'STS.ru',name:'STS',country:'Russia',language:'Russian',url:'',home:'https://ctc.ru/online/'},
    {id:'Moskva24.ru',name:'Moskva 24',country:'Russia',language:'Russian',url:'https://stream.smotrim.ru/hls2/givc9/playlist_4.m3u8',home:'https://www.m24.ru/live'},
    {id:'SanktPeterburg.ru',name:'Sankt-Peterburg',country:'Russia',language:'Russian',url:'https://stream.smotrim.ru/hls2/ext_spbtv/playlist_5.m3u8',home:'https://topspb.tv/online/'},
    {id:'Soyuz.ru',name:'Soyuz',country:'Russia',language:'Russian',url:'https://hls-tvsoyuz.cdnvideo.ru/tvsoyuz/soyuz/playlist.m3u8',home:'https://tv-soyuz.ru/'},

    {id:'TRTWorld.tr',name:'TRT World',country:'Türkiye',language:'English',url:'https://tv-trtworld.medya.trt.com.tr/master.m3u8',home:'https://www.trtworld.com/live'},
    {id:'TRTHaber.tr',name:'TRT Haber',country:'Türkiye',language:'Turkish',url:'https://tv-trthaber.medya.trt.com.tr/master.m3u8',home:'https://www.trthaber.com/canli-yayin-izle/trt-haber/'},
    {id:'TRT1.tr',name:'TRT 1',country:'Türkiye',language:'Turkish',url:'https://tv-trt1.medya.trt.com.tr/master.m3u8',home:'https://www.trt1.com.tr/canli-izle'},
    {id:'NTV.tr',name:'NTV Türkiye',country:'Türkiye',language:'Turkish',url:'',home:'https://www.ntv.com.tr/canli-yayin/ntv'},
    {id:'TRTArabi.tr',name:'TRT Arabi',country:'Türkiye',language:'Arabic',url:'https://tv-trtarabi.medya.trt.com.tr/master.m3u8',home:'https://www.trtarabi.com/live'},
    {id:'TRTAvaz.tr',name:'TRT Avaz',country:'Türkiye',language:'Turkish / Turkic',url:'https://tv-trtavaz.medya.trt.com.tr/master.m3u8',home:'https://www.trtavaz.com.tr/'},
    {id:'TRTBelgesel.tr',name:'TRT Belgesel',country:'Türkiye',language:'Turkish',url:'https://tv-trtbelgesel.medya.trt.com.tr/master.m3u8',home:'https://www.trtbelgesel.com.tr/'},
    {id:'TRTTurk.tr',name:'TRT Türk',country:'Türkiye',language:'Turkish',url:'https://tv-trtturk.medya.trt.com.tr/master.m3u8',home:'https://www.trtturk.com.tr/'},
    {id:'TGRTHaber.tr',name:'TGRT Haber',country:'Türkiye',language:'Turkish',url:'https://canli.tgrthaber.com/tgrt.m3u8',home:'https://www.tgrthaber.com/canli-yayin'},

    {id:'ArirangTV.kr',name:'Arirang TV',country:'South Korea',language:'English / Korean',url:'',home:'https://www.arirang.com/'},
    {id:'NHKWorldJapan.jp',name:'NHK World Japan',country:'Japan',language:'English',url:'',home:'https://www3.nhk.or.jp/nhkworld/en/live/'},
    {id:'CNAInternational.sg',name:'CNA',country:'Singapore',language:'English',url:'',home:'https://www.channelnewsasia.com/watch'},
    {id:'DDNews.in',name:'DD News',country:'India',language:'Hindi / English',url:'',home:'https://ddnews.gov.in/'}
  ];

  function ensureStyle() {
    if (document.getElementById('nougatWorldTvStyle')) return;
    const style = document.createElement('style');
    style.id = 'nougatWorldTvStyle';
    style.textContent = `
      .world-tv-workspace{--world-tv:#c97426;--world-tv-soft:#f0b36c}
      .world-tv-workspace .module-heading{color:var(--world-tv-soft)}
      .world-tv-workspace .sheet-button.active-tool{border-color:var(--world-tv);background:#2c1808;color:#ffd7a4}
      .world-tv-workspace .module-field{border-color:#8a4b18}
      .world-tv-grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(190px,1fr));gap:10px;margin:12px 4px 24px}
      .world-tv-card{display:grid;grid-template-rows:112px auto;border:1px solid #7c481d;background:#171008;overflow:hidden;clip-path:polygon(8px 0,calc(100% - 8px) 0,100% 8px,100% calc(100% - 8px),calc(100% - 8px) 100%,8px 100%,0 calc(100% - 8px),0 8px)}
      .world-tv-art{display:grid;place-items:center;background:#080604;border-bottom:1px solid #6b3c17;color:#b98a5a;overflow:hidden;font-weight:800;letter-spacing:.08em}
      .world-tv-art img{width:100%;height:100%;object-fit:contain;background:#050403}
      .world-tv-body{display:grid;gap:6px;padding:10px}
      .world-tv-name{font-size:12px;font-weight:800;color:#f4e0c8}
      .world-tv-meta{font-size:10px;color:#b99b79;line-height:1.4}
      .world-tv-actions{display:flex;gap:5px;flex-wrap:wrap}
      .world-tv-actions .sheet-button{flex:1 1 auto;min-width:74px;border-color:#7c481d}
      .world-tv-status{border-left-color:var(--world-tv)}
      @media(max-width:680px){.world-tv-grid{grid-template-columns:repeat(2,minmax(0,1fr))}.world-tv-art{height:92px}}
    `;
    document.head.appendChild(style);
  }

  function getStreams() {
    if (!streamsPromise) streamsPromise = fetch(IPTV_STREAMS, {cache:'force-cache'}).then(r => {
      if (!r.ok) throw new Error(`IPTV directory HTTP ${r.status}`);
      return r.json();
    }).catch(() => []);
    return streamsPromise;
  }

  function getLogos() {
    if (!logosPromise) logosPromise = fetch(IPTV_LOGOS, {cache:'force-cache'}).then(r => {
      if (!r.ok) throw new Error(`logo directory HTTP ${r.status}`);
      return r.json();
    }).catch(() => []);
    return logosPromise;
  }

  async function resolveDirectoryStream(station) {
    const rows = await getStreams();
    const matches = (Array.isArray(rows) ? rows : []).filter((row) => {
      const id = String(row.channel || row.channel_id || '');
      const url = String(row.url || '');
      return id === station.id && /^https?:\/\//i.test(url) && !/youtube\.com|youtu\.be/i.test(url);
    });
    if (!matches.length) return '';
    const score = (row) => {
      const quality = String(row.quality || '').match(/\d+/);
      const height = quality ? Number(quality[0]) : 0;
      const url = String(row.url || '');
      return Math.min(height || 720, 1080) + (/\.m3u8(?:$|\?)/i.test(url) ? 50 : 0) + (/^https:/i.test(url) ? 25 : 0);
    };
    matches.sort((a,b) => score(b) - score(a));
    return String(matches[0].url || '');
  }

  function aloulaItems(payload) {
    if (Array.isArray(payload)) return payload;
    if (!payload || typeof payload !== 'object') return [];
    for (const key of ['data','channels','items']) if (Array.isArray(payload[key])) return payload[key];
    return [];
  }

  async function resolveAloulaQuran() {
    const channelsResponse = await fetch(ALOULA_CHANNELS, {headers:{Accept:'application/json'}});
    if (!channelsResponse.ok) throw new Error(`Aloula HTTP ${channelsResponse.status}`);
    const channels = aloulaItems(await channelsResponse.json());
    const ranked = channels.filter(c => c && c.has_live).map(c => {
      const title = String(c.title || '');
      const slug = String(c.url || '');
      const joined = `${title} ${slug}`.toLowerCase().replace(/[^a-z0-9\u0600-\u06ff]+/g,'');
      let score = 0;
      if (title.includes('القرآن') && title.includes('الكريم')) score = 200;
      if (/alkuranalkarim|alquranalkarim|alquranalkareem|quranalkarim|quranalkareem|qurantv/.test(joined)) score = Math.max(score,180);
      if (/quran|kuran|koran/.test(joined)) score = Math.max(score,120);
      return {score,c};
    }).filter(x => x.score > 0).sort((a,b) => b.score - a.score);
    if (!ranked.length) throw new Error('official Aloula Quran channel not found');
    const channel = ranked[0].c;
    if (channel.streams && typeof channel.streams.hls === 'string') return channel.streams.hls;
    if (!Number.isInteger(channel.id)) throw new Error('official Aloula channel ID invalid');
    const playerResponse = await fetch(ALOULA_PLAYER(channel.id), {headers:{Accept:'application/json'}});
    if (!playerResponse.ok) throw new Error(`Aloula player HTTP ${playerResponse.status}`);
    const player = await playerResponse.json();
    const candidates = [player, player && player.data, player && player.player].filter(x => x && typeof x === 'object');
    for (const item of candidates) if (item.streams && typeof item.streams.hls === 'string') return item.streams.hls;
    throw new Error('official Aloula live HLS unavailable');
  }

  async function resolveStation(station) {
    if (station.url) return station.url;
    if (station.resolver === 'aloula-quran') {
      try { return await resolveAloulaQuran(); } catch (_) {}
    }
    return resolveDirectoryStream(station);
  }

  function filteredStations() {
    const q = query.trim().toLowerCase();
    return stations.map((station, index) => ({station,index})).filter(({station}) => {
      if (region !== 'ALL' && station.country !== region) return false;
      if (!q) return true;
      return `${station.name} ${station.country} ${station.language}`.toLowerCase().includes(q);
    });
  }

  function regionOptions() {
    const values = [...new Set(stations.map(s => s.country))].sort((a,b) => a.localeCompare(b));
    return ['ALL', ...values];
  }

  async function hydrateArtwork() {
    const logos = await getLogos();
    if (!Array.isArray(logos) || !logos.length) return;
    const byChannel = new Map();
    for (const logo of logos) {
      const channel = String(logo.channel || logo.channel_id || '');
      const url = String(logo.url || '');
      if (channel && /^https?:\/\//i.test(url) && !byChannel.has(channel)) byChannel.set(channel, url);
    }
    document.querySelectorAll('[data-world-art]').forEach((box) => {
      const id = box.dataset.worldArt;
      const url = byChannel.get(id);
      if (!url || box.querySelector('img')) return;
      const img = document.createElement('img');
      img.src = url;
      img.alt = '';
      img.loading = 'lazy';
      img.addEventListener('error', () => img.remove(), {once:true});
      box.textContent = '';
      box.appendChild(img);
    });
  }

  function selectCard(index) {
    selectedIndex = index;
    document.querySelectorAll('[data-world-index]').forEach(card => card.classList.toggle('active-tool', Number(card.dataset.worldIndex) === index));
  }

  async function watchStation(host, index) {
    const station = stations[index];
    if (!station) return;
    selectCard(index);
    const status = document.getElementById('worldTvStatus');
    if (status) status.textContent = `Resolving ${station.name}...`;
    let url = '';
    try { url = await resolveStation(station); } catch (error) {
      if (status) status.textContent = `${station.name}: ${error.message || error}`;
    }
    if (url && host && typeof host.playUrl === 'function') {
      if (status) status.textContent = `${station.name} • direct non-YouTube source sent to Nougat Player`;
      host.playUrl(url, station.name);
      return;
    }
    if (status) status.textContent = `${station.name} has no browser-playable direct source right now. Opening the official broadcaster page.`;
    if (station.home) window.open(station.home, '_blank', 'noopener');
  }

  function renderCards(host) {
    const grid = document.getElementById('worldTvGrid');
    if (!grid) return;
    const list = filteredStations();
    grid.innerHTML = list.length ? list.map(({station,index}) => `
      <article class="world-tv-card${index === selectedIndex ? ' active-tool' : ''}" data-world-index="${index}">
        <div class="world-tv-art" data-world-art="${esc(station.id)}">${esc(station.name.split(/\s+/).slice(0,2).map(x=>x[0]||'').join('').toUpperCase())}</div>
        <div class="world-tv-body">
          <div class="world-tv-name">${esc(station.name)}</div>
          <div class="world-tv-meta">${esc(station.country)} • ${esc(station.language)}</div>
          <div class="world-tv-actions">
            <button type="button" class="sheet-button" data-world-watch="${index}">Watch Live</button>
            <button type="button" class="sheet-button" data-world-official="${index}">Official Site</button>
          </div>
        </div>
      </article>`).join('') : '<div class="module-empty">No World TV channels match this filter.</div>';

    grid.querySelectorAll('[data-world-index]').forEach(card => card.addEventListener('click', (event) => {
      if (event.target.closest('button')) return;
      selectCard(Number(card.dataset.worldIndex));
    }));
    grid.querySelectorAll('[data-world-watch]').forEach(btn => btn.addEventListener('click', () => watchStation(host, Number(btn.dataset.worldWatch))));
    grid.querySelectorAll('[data-world-official]').forEach(btn => btn.addEventListener('click', () => {
      const station = stations[Number(btn.dataset.worldOfficial)];
      if (station && station.home) window.open(station.home, '_blank', 'noopener');
    }));
    hydrateArtwork().catch(() => {});
  }

  function renderWorldTv(host) {
    ensureStyle();
    const r = root();
    if (!r) return;
    const regions = regionOptions();
    r.innerHTML = `<div class="module-workspace world-tv-workspace">
      <div class="module-toolbar">
        <button type="button" class="sheet-button" id="worldTvPrevious">Previous</button>
        <button type="button" class="sheet-button" id="worldTvNext">Next</button>
        <button type="button" class="sheet-button" id="worldTvWatchSelected">Watch Live</button>
        <button type="button" class="sheet-button" id="worldTvOfficialSelected">Official Site</button>
      </div>
      <h2 class="module-heading">WORLD TV</h2>
      <div class="module-field"><span>SEARCH</span><input id="worldTvSearch" type="search" value="${esc(query)}" autocomplete="off"></div>
      <div class="module-field"><span>REGION</span><select id="worldTvRegion">${regions.map(value => `<option ${value===region?'selected':''}>${esc(value)}</option>`).join('')}</select></div>
      <div class="module-output world-tv-status" id="worldTvStatus">${stations.length} standalone World TV channels loaded. Direct non-YouTube sources are preferred; official broadcaster pages are the fallback.</div>
      <div class="world-tv-grid" id="worldTvGrid"></div>
    </div>`;

    const search = document.getElementById('worldTvSearch');
    const regionSelect = document.getElementById('worldTvRegion');
    search.addEventListener('input', () => { query = search.value; renderCards(host); });
    regionSelect.addEventListener('change', () => { region = regionSelect.value; renderCards(host); });
    bind('worldTvPrevious', () => { selectedIndex = (selectedIndex - 1 + stations.length) % stations.length; renderCards(host); });
    bind('worldTvNext', () => { selectedIndex = (selectedIndex + 1) % stations.length; renderCards(host); });
    bind('worldTvWatchSelected', () => watchStation(host, selectedIndex));
    bind('worldTvOfficialSelected', () => { const station = stations[selectedIndex]; if (station && station.home) window.open(station.home, '_blank', 'noopener'); });
    renderCards(host);
  }

  window.NougatWebModules.activate = function(name, host) {
    if (name === 'worldtv') return renderWorldTv(host);
    return previous(name, host);
  };
})();
