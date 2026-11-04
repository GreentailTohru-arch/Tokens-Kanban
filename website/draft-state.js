export class DraftState {
  constructor(){this.sources=new Map();this.saved=new Map();}
  add(key,label,snapshot){this.sources.set(key,{label,snapshot});this.mark(key);}
  mark(key){const source=this.sources.get(key);if(source)this.saved.set(key,source.snapshot());}
  changed(){return [...this.sources].filter(([key,s])=>s.snapshot()!==this.saved.get(key)).map(([key,s])=>({key,label:s.label}));}
}
