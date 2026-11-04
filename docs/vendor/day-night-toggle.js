/* Adapted from Xiumuzaidiao/Day-night-toggle-button, version 4.0.
 * Upstream commit: 914c2e14567325f81b7b361f1bce19eea67e8b0f
 * Controlled presentation: host application handles state, persistence and keyboard input.
 * ISC License
 * 
 * Copyright(c)2024,Xiumuzaidiao
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 * 
 * 
 * 
 */
(() => {
 const stylesheet=new URL('day-night-toggle.css?v=1',document.currentScript.src).href;
 class ThemeButton extends HTMLElement {
  connectedCallback(){
   if(this.shadowRoot)return;
   const shadow=this.attachShadow({mode:'open'}),style=document.createElement('link'),container=document.createElement('div');
   container.className='container';
   style.rel='stylesheet';style.href=stylesheet;
   container.innerHTML="<div class=\"components\"><div class=\"main-button\"><div class=\"moon\"></div><div class=\"moon\"></div><div class=\"moon\"></div></div><div class=\"daytime-background\"></div><div class=\"daytime-background\"></div><div class=\"daytime-background\"></div><div class=\"cloud\"><div class=\"cloud-son\"></div><div class=\"cloud-son\"></div><div class=\"cloud-son\"></div><div class=\"cloud-son\"></div><div class=\"cloud-son\"></div><div class=\"cloud-son\"></div></div><div class=\"cloud-light\"><div class=\"cloud-son\"></div><div class=\"cloud-son\"></div><div class=\"cloud-son\"></div><div class=\"cloud-son\"></div><div class=\"cloud-son\"></div><div class=\"cloud-son\"></div></div><div class=\"stars\"><div class=\"star big\"><div class=\"star-son\"></div><div class=\"star-son\"></div><div class=\"star-son\"></div><div class=\"star-son\"></div></div><div class=\"star big\"><div class=\"star-son\"></div><div class=\"star-son\"></div><div class=\"star-son\"></div><div class=\"star-son\"></div></div><div class=\"star medium\"><div class=\"star-son\"></div><div class=\"star-son\"></div><div class=\"star-son\"></div><div class=\"star-son\"></div></div><div class=\"star medium\"><div class=\"star-son\"></div><div class=\"star-son\"></div><div class=\"star-son\"></div><div class=\"star-son\"></div></div><div class=\"star small\"><div class=\"star-son\"></div><div class=\"star-son\"></div><div class=\"star-son\"></div><div class=\"star-son\"></div></div><div class=\"star small\"><div class=\"star-son\"></div><div class=\"star-son\"></div><div class=\"star-son\"></div><div class=\"star-son\"></div></div></div></div>";
   shadow.append(style,container);
  }
 }
 customElements.define("theme-button",ThemeButton);
})();
