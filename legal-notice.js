(() => {
  'use strict';
  const noticeVersion = '2026-10-30-v2';
  const key = 'nougat-media-plus-legal-acceptance';
  const gate = document.getElementById('nougatLegalGate');
  const checkbox = document.getElementById('nougatLegalConsent');
  const agree = document.getElementById('nougatLegalAgree');
  const reject = document.getElementById('nougatLegalReject');
  const warning = document.getElementById('nougatLegalRejectWarning');
  const storageError = document.getElementById('nougatLegalStorageError');
  if (!gate || !checkbox || !agree || !reject || !warning) {
    // Fail closed: without the notice controls, the viewer must not be usable.
    document.body.replaceChildren();
    document.body.textContent = 'Nougat Media Plus cannot start: legal notice controls are unavailable.';
    return;
  }

  let accepted = false;
  try { accepted = localStorage.getItem(key) === noticeVersion; } catch (_) { /* no valid receipt */ }
  // GitHub Pages uses a separate HTTPS bridge. LAN-served HTTP pages
  // register a server-side acknowledgement before loading any media data.
  const requiresLanSession = location.protocol === 'http:';
  const registerLanSession = async () => {
    if (!requiresLanSession) return;
    const response = await fetch('/nougat/v1/terms/accept', {
      method: 'POST', credentials: 'same-origin', cache: 'no-store'
    });
    if (!response.ok) throw new Error('LAN terms acceptance failed');
    const status = await fetch('/nougat/v1/terms/status', {
      credentials: 'same-origin', cache: 'no-store'
    });
    if (!status.ok || !(await status.json()).accepted)
      throw new Error('The LAN server did not retain the acknowledgement cookie');
  };

  // Keep every other part of the UI inert while acceptance is outstanding.
  const background = Array.from(document.body.children).filter(node => node !== gate);
  const previousOverflow = document.documentElement.style.overflow;
  for (const node of background) node.inert = true;
  document.documentElement.style.overflow = 'hidden';
  gate.hidden = false;
  checkbox.checked = false;
  agree.disabled = true;
  let refusedOnce = false;

  const closeViewer = () => {
    // Browsers usually prevent scripts from closing a user-opened tab.
    // Remove the UI and navigate away, leaving no app functionality accessible.
    document.body.replaceChildren();
    document.body.textContent = 'Nougat Media Plus has closed. Acceptance of the Terms of Service is required to use this application.';
    document.body.style.background = '#071713';
    document.body.style.color = '#f1f6ef';
    document.body.style.padding = '32px';
    try { window.close(); } catch (_) { /* no permission to close tab */ }
    try { window.location.replace('about:blank'); } catch (_) { /* UI remains removed */ }
  };
  const refuse = () => {
    if (refusedOnce) { closeViewer(); return; }
    refusedOnce = true;
    checkbox.checked = false;
    agree.disabled = true;
    warning.hidden = false;
    reject.textContent = 'Refuse & Exit';
    reject.focus();
  };

  const unlock = () => {
    window.NougatLegalAccepted = true;
    gate.hidden = true;
    for (const node of background) node.inert = false;
    document.documentElement.style.overflow = previousOverflow;
    window.dispatchEvent(new Event('nougat:legal-accepted'));
  };
  const showAcceptanceError = () => {
    if (storageError) {
      storageError.textContent = 'Could not confirm or save acknowledgement. Check your browser settings and LAN server, then retry.';
      storageError.hidden = false;
    }
  };
  checkbox.addEventListener('change', () => { agree.disabled = !checkbox.checked; });
  reject.addEventListener('click', refuse);
  agree.addEventListener('click', async () => {
    if (!checkbox.checked || agree.disabled) return;
    agree.disabled = true;
    try {
      await registerLanSession();
      localStorage.setItem(key, noticeVersion);
      if (localStorage.getItem(key) !== noticeVersion) throw new Error('Storage unavailable');
    } catch (_) {
      showAcceptanceError();
      agree.disabled = !checkbox.checked;
      return;
    }
    unlock();
  });
  gate.addEventListener('keydown', (event) => {
    if (event.key === 'Escape') {
      event.preventDefault();
      refuse();
      return;
    }
    if (event.key !== 'Tab') return;
    const focusable = Array.from(gate.querySelectorAll('a[href], input:not(:disabled), button:not(:disabled)'));
    if (!focusable.length) return;
    if (event.shiftKey && document.activeElement === focusable[0]) {
      event.preventDefault(); focusable[focusable.length - 1].focus();
    } else if (!event.shiftKey && document.activeElement === focusable[focusable.length - 1]) {
      event.preventDefault(); focusable[0].focus();
    }
  });
  if (accepted) {
    // A service restart invalidates process-local sessions. Re-register a
    // saved acknowledgement without asking the user to tick the box again.
    registerLanSession().then(unlock).catch(() => {
      showAcceptanceError();
      checkbox.focus();
    });
  } else {
    checkbox.focus();
  }
})();
