#include "theme_model.h"
#include <string.h>
theme_config_t theme_default(void){return (theme_config_t){0,0x68C4CB,0x70BFFF,0x2F3A49,0xF1F7F4,0xADB8C3};}
bool theme_valid(const theme_config_t *c){return c && c->preset<4 && c->accent<=0xFFFFFF && c->chart<=0xFFFFFF && c->background<=0xFFFFFF && c->text<=0xFFFFFF && c->muted<=0xFFFFFF;}
static uint32_t mix(uint32_t a,uint32_t b,unsigned percent){uint32_t result=0;for(unsigned shift=0;shift<24;shift+=8)result|=((((a>>shift)&255)*(100-percent)+((b>>shift)&255)*percent+50)/100)<<shift;return result;}
uint32_t theme_color(const theme_config_t *c,uint32_t original){
    theme_config_t d=theme_default();
    if(!c || (c->accent==d.accent && c->chart==d.chart && c->background==d.background && c->text==d.text && c->muted==d.muted))return original;
    switch(original){
    case 0x2F3A49:return c->background;
    case 0xF1F7F4:return c->text;
    case 0xADB8C3:return c->muted;
    case 0x68C4CB:return c->accent;
    case 0x70BFFF:return c->chart;
    case 0x374453:return mix(c->background,c->text,5);
    case 0x516471:return mix(c->background,c->accent,27);
    case 0xBDECD8:return mix(c->accent,c->text,55);
    case 0x475B69:case 0x40555E:return mix(c->background,c->text,16);
    case 0x52616E:case 0x455361:return mix(c->background,c->text,22);
    case 0x344352:return mix(c->background,c->chart,8);
    case 0x3B5267:return mix(c->background,c->chart,25);
    case 0x456985:return mix(c->background,c->chart,45);
    case 0x528DB5:return mix(c->background,c->chart,70);
    case 0x9EDBFA:return c->chart;
    default:return original; /* Status lamps, diagnostics and photographs retain their meaning. */
    }
}

/* Existing postcard flash data remains canonical RGB565. Recolor only the known
 * background/text regions; photograph/QR pixels (including quiet zone) are immutable. */
static uint16_t to565(uint32_t c){return ((c>>8)&0xf800)|((c>>5)&0x7e0)|((c>>3)&31);}
uint16_t theme_postcard_pixel(const theme_config_t *c,uint16_t p,unsigned x,unsigned y,bool back){
    theme_config_t d=theme_default();if(!c||(c->background==d.background&&c->text==d.text&&c->muted==d.muted))return p;
    if(back){if(x>=26&&x<214&&y>=8&&y<196)return p;}
    else if(x>=33&&x<207&&y>=6&&y<180){
        int dx=x<49?49-(int)x:x>190?(int)x-190:0;
        int dy=y<22?22-(int)y:y>163?(int)y-163:0;
        if(dx*dx+dy*dy<=256)return p;
    }
    if(p==to565(d.background))return to565(c->background);
    /* Text anti-aliasing is a blend with the canonical background. Infer coverage
     * from green (6 bits), only in the established text bands. */
    bool text=back?(y>=222&&y<254):(y>=196&&y<264);
    if(!text)return p;
    bool secondary=!back&&y>=232;uint32_t original=secondary?d.muted:d.text,target=secondary?c->muted:c->text;
    int low=(to565(d.background)>>5)&63,high=(to565(original)>>5)&63,v=(p>>5)&63;
    int coverage=(v-low)*100/(high-low);if(coverage<0)coverage=0;if(coverage>100)coverage=100;
    return to565(mix(c->background,target,(unsigned)coverage));
}
