(() => {
  'use strict';
  if(!window.NougatWebModules)return;
  const previous=window.NougatWebModules.activate.bind(window.NougatWebModules);
  let timer=0;

  function stop(){if(timer){clearInterval(timer);timer=0;}}
  function start(){
    stop();
    timer=setInterval(()=>{
      const guide=document.getElementById('liveGuide');
      const view=document.querySelector('[data-view-panel="module"].active');
      const liveButton=document.querySelector('.rail-button[data-view="livetv"].active');
      if(!guide||!view||!liveButton)return;
      guide.click();
    },60000);
  }

  window.NougatWebModules.activate=function(name,host){
    if(name==='livetv'){
      const result=previous(name,host);
      queueMicrotask(start);
      return result;
    }
    stop();
    return previous(name,host);
  };
})();