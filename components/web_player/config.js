(() => {
  'use strict';

  // Same HTTPS Nougat media bridge proven by the Hanafi web viewer.
  // GitHub Pages serves only the front end; movies and TV remain on the
  // Nougat host and are delivered through /nougat/v1/* with HTTP Range support.
  window.NOUGAT_WEB_PLAYER = Object.freeze({
    enabled: true,
    baseUrl: 'https://97-201-65-144.sslip.io'
  });
})();
