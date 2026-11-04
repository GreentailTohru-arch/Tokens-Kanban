// Short-lived, accessible feedback. All messages remain plain text.
export class NoticeCenter {
  constructor(node){this.node=node;this.nodes=[];this.timer=null;this.exitTimer=null;this.drag=null;this.remaining=0;this.hovered=new Set();}
  clear(){clearTimeout(this.timer);clearTimeout(this.exitTimer);this.timer=this.exitTimer=null;this.drag=null;this.hovered.clear();}
  pause(){if(this.timer){clearTimeout(this.timer);this.timer=null;this.remaining=Math.max(0,this.deadline-performance.now());}}
  resume(){if(!this.node.hidden&&this.node.dataset.phase!=='exit'&&!this.timer&&!this.drag&&!this.hovered.size&&!this.nodes.some(n=>n.contains(document.activeElement))){this.deadline=performance.now()+this.remaining;this.timer=setTimeout(()=>this.dismiss(),this.remaining);}}
  decorate(node,text,kind){
    node.replaceChildren();node.hidden=false;node.title=text;node.dataset.kind=kind;node.dataset.phase='enter';node.removeAttribute('data-dragging');node.style.removeProperty('--notice-x');node.style.removeProperty('--notice-y');node.setAttribute('role',kind==='error'?'alert':'status');
    const label=document.createElement('span');label.className='notice-text';label.textContent=text;
    const close=document.createElement('button');close.type='button';close.className='notice-close';close.setAttribute('aria-label','关闭通知');close.title='关闭通知';close.innerHTML='<svg viewBox="0 0 16 16" aria-hidden="true"><path d="m4 4 8 8M12 4l-8 8"/></svg>';close.onclick=()=>this.dismiss();node.append(label,close);
    node.onpointerenter=e=>{if(e.pointerType==='mouse'){this.hovered.add(node);this.pause();}};node.onpointerleave=()=>{this.hovered.delete(node);this.resume();};close.onfocus=()=>this.pause();close.onblur=()=>queueMicrotask(()=>this.resume());
    node.onkeydown=e=>{if(e.key==='Escape'){e.stopPropagation();this.dismiss();}};
    node.onpointerdown=e=>{if(e.button!==0||e.target.closest('button'))return;this.pause();this.drag={id:e.pointerId,x:e.clientX,y:e.clientY,dx:0,dy:0};node.dataset.dragging='true';node.setPointerCapture(e.pointerId);};
    node.onpointermove=e=>{if(!this.drag||this.drag.id!==e.pointerId)return;this.drag.dx=e.clientX-this.drag.x;this.drag.dy=Math.min(0,e.clientY-this.drag.y);node.style.setProperty('--notice-x',this.drag.dx+'px');node.style.setProperty('--notice-y',this.drag.dy+'px');};
    const release=e=>{if(!this.drag||this.drag.id!==e.pointerId)return;const d=this.drag;this.drag=null;node.removeAttribute('data-dragging');if(node.hasPointerCapture(e.pointerId))node.releasePointerCapture(e.pointerId);if(e.type!=='pointercancel'&&(Math.abs(d.dx)>=48||d.dy<=-36))this.dismiss();else{node.style.removeProperty('--notice-x');node.style.removeProperty('--notice-y');this.resume();}};
    node.onpointerup=release;node.onpointercancel=release;node.onlostpointercapture=e=>{if(this.drag?.id===e.pointerId)release(e);};
  }
  show(text,kind='success'){
    this.clear();document.querySelectorAll('.dialog-notice').forEach(n=>n.remove());this.nodes=[this.node];this.focusReturn=document.activeElement;this.remaining=kind==='error'?4200:2200;
    if(!text){this.node.hidden=true;return;}
    this.decorate(this.node,text,kind);
    const modal=document.querySelector('dialog[open]');if(modal){const n=document.createElement('div');n.className='toast dialog-notice';this.decorate(n,text,kind);const heading=modal.querySelector('.crop-heading,h2');if(heading)heading.after(n);else modal.prepend(n);this.nodes.push(n);}
    this.resume();
  }
  dismiss(){
    this.clear();if(this.node.hidden)return;
    const focused=this.nodes.some(n=>n.contains(document.activeElement));
    for(const n of this.nodes){n.dataset.phase='exit';n.removeAttribute('data-dragging');}
    if(focused){const target=this.focusReturn?.isConnected&&!this.focusReturn.disabled?this.focusReturn:document.getElementById('menuToggle');target?.focus({preventScroll:true});}
    this.exitTimer=setTimeout(()=>{for(const n of this.nodes){n.hidden=true;if(n!==this.node)n.remove();}this.exitTimer=null;},matchMedia('(prefers-reduced-motion:reduce)').matches?0:140);
  }
}
