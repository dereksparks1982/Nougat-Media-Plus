(() => {
  'use strict';
  const noticeVersion = '2026-10-30-v1';
  const key = 'nougat-media-plus-legal-acceptance';
  const gate = document.getElementById('nougatLegalGate');
  const checkbox = document.getElementById('nougatLegalConsent');
  const button = document.getElementById('nougatLegalAgree');
  const message = document.getElementById('nougatLegalStorageError');
  if (!gate || !checkbox || !button) return;

  let accepted = false;
  try { accepted = localStorage.getItem(key) === noticeVersion; } catch (_) { /* no receipt */ }
  if (accepted) { gate.hidden = true; return; }

  // Inert background leaves accepted UI geometry unchanged.
  const background = Array.from(document.body.children).filter(node => node !== gate);
  const previousOverflow = document.documentElement.style.overflow;
  for (const node of background) node.inert = true;
  document.documentElement.style.overflow = 'hidden';
  gate.hidden = false;
  checkbox.checked = false;
  button.disabled = true;
  checkbox.addEventListener('change', () => { button.disabled = !checkbox.checked; });
  button.addEventListener('click', () => {
    if (!checkbox.checked) return;
    try {
      localStorage.setItem(key, noticeVersion);
      if (localStorage.getItem(key) !== noticeVersion) throw new Error('Storage unavailable');
    } catch (_) {
      if (message) message.hidden = false;
      return;
    }
    gate.hidden = true;
    for (const node of background) node.inert = false;
    document.documentElement.style.overflow = previousOverflow;
  });
  gate.addEventListener('keydown', (event) => {
    if (event.key !== 'Tab') return;
    const focusable = Array.from(gate.querySelectorAll('a[href], input:not(:disabled), button:not(:disabled)'));
    if (!focusable.length) return;
    if (event.shiftKey && document.activeElement === focusable[0]) {
      event.preventDefault(); focusable[focusable.length - 1].focus();
    } else if (!event.shiftKey && document.activeElement === focusable[focusable.length - 1]) {
      event.preventDefault(); focusable[0].focus();
    }
  });
  checkbox.focus();
})();
