(() => {
  'use strict';
  const descriptions = Object.freeze({
    discover: 'Local recommendations use the Nougat library. External catalog providers depend on their separate integrations.',
    livetv: 'Channel scans, tuners, recording and guide data require the Nougat host and supported tuner equipment.',
    worldtv: 'External television streams depend on broadcaster availability, region and browser codec support.',
    radio: 'Internet radio may play directly. Receiver, recording and hardware tests require the Linux host and compatible equipment.',
    search: 'Local library search works in the browser. Crawler requests are limited by the source site’s HTTPS and CORS policy; native downloads require the host.',
    stream: 'Direct browser streaming depends on source URLs and codecs. Video extraction and native downloads require the host.',
    studio: 'Browser Studio includes browser-supported tools. Advanced FFmpeg export and device-level editing require the native host.',
    games: 'Game catalogs can be browsed here. Native emulators, ROM access and controller backends require the installed Linux application.',
    network: 'The browser exposes connection details; native interface, firewall and satellite controls require hardware/backend support.',
    system: 'Browser diagnostics work locally. Tuners, antivirus and native system operations require supported host services.'
  });
  const root = document.getElementById('moduleView');
  if (!root) return;
  let pending = false;
  function update() {
    pending = false;
    const view = document.querySelector('.rail-button.active')?.dataset.view;
    if (!view || !descriptions[view]) return;
    const workspace = root.querySelector('.module-workspace');
    if (!workspace || workspace.querySelector('.web-capability-note')) return;
    const details = document.createElement('details');
    details.className = 'web-capability-note';
    const label = document.createElement('summary');
    label.textContent = 'Browser capabilities & host requirements';
    const description = document.createElement('p');
    description.textContent = descriptions[view];
    const control = document.createElement('button');
    control.type = 'button';
    control.className = 'sheet-button';
    control.textContent = 'Configure media server';
    control.addEventListener('click', () =>
      document.getElementById('webHostOpen')?.click());
    details.append(label,description,control);
    workspace.appendChild(details);
  }
  function schedule() {
    if (pending || window.NougatLegalAccepted !== true) return;
    pending = true;
    requestAnimationFrame(update);
  }
  const observer = new MutationObserver(schedule);
  observer.observe(root, { childList:true, subtree:true });
  document.querySelectorAll('.rail-button').forEach(button =>
    button.addEventListener('click',schedule));
  window.addEventListener('nougat:host-changed',schedule);
  if (window.NougatLegalAccepted === true) schedule();
  else window.addEventListener('nougat:legal-accepted',schedule,{once:true});
})();
