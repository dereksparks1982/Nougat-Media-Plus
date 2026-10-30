(() => {
  'use strict';
  const host = window.NougatWebHost;
  const dialog = document.getElementById('webHostDialog');
  const open = document.getElementById('webHostOpen');
  const address = document.getElementById('webHostAddress');
  const message = document.getElementById('webHostStatus');
  const form = document.getElementById('webHostForm');
  if (!host || !dialog || !open || !form) return;
  function say(text) { message.textContent = text; }
  function show() {
    if (window.NougatLegalAccepted !== true) return;
    address.value = host.getBase();
    say('Your media host must be online, reachable over HTTPS, and allow this site through CORS.');
    if (!dialog.open) dialog.showModal();
    address.focus();
  }
  open.addEventListener('click', show);
  document.getElementById('webHostClose').addEventListener('click', () => dialog.close());
  document.getElementById('webHostDefault').addEventListener('click', () => {
    if (!host.reset()) { say('Could not reset settings: local browser storage is unavailable.'); return; }
    address.value = host.getBase();
    say('Restored default server. Library is refreshing.');
  });
  form.addEventListener('submit', (event) => {
    event.preventDefault();
    if (!host.setServer(address.value)) {
      say('Enter a valid HTTPS server origin without a path, query, username or password.');
      return;
    }
    say('Server saved for the full player and Quick Watch.');
    dialog.close();
  });
  document.getElementById('webHostTest').addEventListener('click', async () => {
    const valid = host.validate(address.value);
    if (!valid) { say('Enter a valid HTTPS origin, such as https://media.example.org'); return; }
    const controller = new AbortController();
    const timer = setTimeout(() => controller.abort(), 8000);
    say('Testing connection...');
    try {
      const response = await fetch(valid + '/nougat/v1/health', {
        mode: 'cors', credentials: 'omit', redirect: 'error',
        cache: 'no-store', signal: controller.signal
      });
      if (!response.ok) throw new Error('HTTP ' + response.status);
      const data = await response.json();
      if (data.ok === false) throw new Error('Host did not report ready');
      say('Connected to media host. Choose Save & connect to use it.');
    } catch (error) {
      say('Server unavailable or browser access blocked: ' +
        (error?.name === 'AbortError' ? 'connection timed out' : String(error?.message || error)));
    } finally { clearTimeout(timer); }
  });
})();
