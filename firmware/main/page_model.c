#include "page_model.h"
page_config_t page_config_default(void) {return (page_config_t){63,{PAGE_HOME,PAGE_ANALYTICS,PAGE_PET,PAGE_SYSTEM,PAGE_POSTCARD,PAGE_WALLPAPER}};}
bool page_config_valid(const page_config_t *c) {
    if(!c || !c->enabled || c->enabled>=64)return false;
    unsigned seen=0;
    for(unsigned i=0;i<PAGE_COUNT;++i) {
        if(c->order[i]>=PAGE_COUNT || (seen&(1u<<c->order[i])))return false;
        seen|=1u<<c->order[i];
    }
    return seen==63;
}
unsigned page_count(const page_config_t *c) {
    unsigned n=0;for(unsigned i=0;i<PAGE_COUNT;++i)n+=(c->enabled>>i)&1u;return n;
}
unsigned page_position(const page_config_t *c,unsigned page) {
    unsigned n=0;
    for(unsigned i=0;i<PAGE_COUNT;++i)if(c->enabled&(1u<<c->order[i])) {
        if(c->order[i]==page)return n;
        ++n;
    }
    return 0;
}
void page_nav_normalize(const page_config_t *c,token_nav_t *nav) {
    if(nav->page<PAGE_COUNT && (c->enabled&(1u<<nav->page)))return;
    for(unsigned i=0;i<PAGE_COUNT;++i)if(c->enabled&(1u<<c->order[i])) {nav->page=c->order[i];return;}
}
void page_nav_key(const page_config_t *c,token_nav_t *nav,unsigned key,bool double_click) {
    page_nav_normalize(c,nav);
    if(key==TOKEN_KEY_OK && double_click) {
        unsigned pos=0;while(pos<PAGE_COUNT && c->order[pos]!=nav->page)++pos;
        for(unsigned step=1;step<=PAGE_COUNT;++step) {
            unsigned next=c->order[(pos+step)%PAGE_COUNT];
            if(c->enabled&(1u<<next)) {nav->page=next;break;}
        }
    } else if(key==TOKEN_KEY_OK) {
        if(nav->page==PAGE_HOME)nav->show_cost=!nav->show_cost;
        if(nav->page==PAGE_POSTCARD)nav->contact=!nav->contact;
    } else if(!double_click && (nav->page==PAGE_HOME || nav->page==PAGE_ANALYTICS)) {
        if(key==TOKEN_KEY_UP)nav->period=(nav->period+2)%3;
        if(key==TOKEN_KEY_DOWN)nav->period=(nav->period+1)%3;
    }
}
