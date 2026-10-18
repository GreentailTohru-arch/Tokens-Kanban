// Completion feedback is separate from transfer progress: only confirmed saves call this.
let bar,animation,hideTimer;
export function cancelSaveCompletion(){clearTimeout(hideTimer);animation?.cancel();animation=null;if(bar)bar.hidden=true;}
export function completeSave(){
 cancelSaveCompletion();
 if(!bar){bar=document.createElement('div');bar.className='save-complete';bar.setAttribute('role','progressbar');bar.setAttribute('aria-label','保存完成');bar.setAttribute('aria-valuemin','0');bar.setAttribute('aria-valuemax','100');bar.setAttribute('aria-valuenow','100');document.querySelector('.topbar').append(bar);}
 bar.hidden=false;
 const reduced=matchMedia('(prefers-reduced-motion:reduce)').matches;
 animation=bar.animate(reduced?[{opacity:1},{opacity:1}]:[{transform:'scaleX(0)'},{transform:'scaleX(1)'}],{duration:650,easing:'ease-out',fill:'forwards'});
 const current=animation;
 current.finished.then(()=>{if(animation===current)hideTimer=setTimeout(()=>{if(animation===current){bar.hidden=true;animation=null;}},180);}).catch(()=>{});
}
