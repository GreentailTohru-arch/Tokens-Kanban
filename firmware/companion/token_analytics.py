"""Calendar analytics from the same deduplicated local token events."""
import datetime as dt
import math
from collections import Counter

MODEL_IDS={'gpt-6-astra':1,'gpt-6.1-sol':2,'gpt-6-sol':3,'gpt-5.6-sol':4,
           'gpt-5.6-luna':5,'gpt-6-luna':6,'gpt-5.6-terra':7}

def summarize(records,now):
    today=now.date();monday=today-dt.timedelta(days=today.weekday())
    start=monday-dt.timedelta(weeks=25)
    trend_start=today-dt.timedelta(days=59)
    unique={}
    for key,when,delta,cost,model in records:
        del cost
        if when<=now and (key not in unique or unique[key][2] is None):
            unique[key]=(when,delta,model)
    daily=Counter();models=[Counter() for _ in range(3)]
    for when,delta,model in unique.values():
        day=when.astimezone(now.tzinfo).date();total=delta[0]+delta[2]
        daily[day]+=total
        ident=MODEL_IDS.get(model,0) if isinstance(model,str) else 0
        for i,include in enumerate((day==today,monday<=day<=today,True)):
            if include and total>0: models[i][ident]+=total
    fields=[]
    for period in models:
        top=sorted(period.items(),key=lambda pair:(-pair[1],pair[0]))[:2]
        for ident,total in top+[(0,-1)]*(2-len(top)): fields.extend([ident,total])
    heat_values=[daily[start+dt.timedelta(days=i)] for i in range(182)]
    peak_heat=max(heat_values,default=0)
    heat=''.join('5' if start+dt.timedelta(days=i)>today else
                 str(max(1,min(4,math.ceil(math.log1p(n)/math.log1p(peak_heat)*4)))) if n else '0'
                 for i,n in enumerate(heat_values))
    trend_values=[daily[trend_start+dt.timedelta(days=i)] for i in range(60)]
    peak=max(trend_values,default=0)
    trend=''.join(f'{n*1000//peak if peak else 0:03x}' for n in trend_values)
    fields.append(sum(1 for i,n in enumerate(heat_values) if n>0 and start+dt.timedelta(days=i)<=today))
    fields.extend((start+dt.timedelta(weeks=w)).month for w in (0,12,25))
    for offset in (0,29,59):
        day=trend_start+dt.timedelta(days=offset);fields.append(day.month*100+day.day)
    fields.extend([peak,1 if unique else 0])
    return dict(fields=fields,heat=heat,trend=trend)
