"""Send actual companion frames through the firmware's C stream/parser."""
from pathlib import Path
import subprocess
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'companion'))
from codex_tokens import encode_frame
frame=encode_frame(42,[[100,60,20],[1000,700,50],[10000000000,5000000000,99]],
                   [13,10080,5000,-1,-1,-1,1,3,0],2,[[5736,1],[9999,2],[-1,0]])
corrupt=frame.replace(b'100 ',b'101 ',1)
result=subprocess.run([sys.argv[1]],input=b'X'*600+b'\n'+corrupt+frame,capture_output=True,check=True)
assert result.stdout.strip()==b'42 120 1050 10000000099 5736 1 9999 2 -1 0',result.stdout
print('Python-to-firmware C wire integration: PASS')
analytics=dict(fields=[1,120,0,-1,1,1050,0,-1,2,10000000099,0,-1,2,4,7,10,818,916,1016,1050,1],
               heat='0'*180+'45',trend='000'*59+'3e8')
args=(43,[[100,60,20],[1000,700,50],[10000000000,5000000000,99]],
      [13,10080,5000,-1,-1,-1,1,3,0],2,[[5736,1],[9999,2],[-1,0]])
new_frame=encode_frame(*args,analytics)
invalid=[]
for index,value in ((0,8),(1,121),(12,183),(13,0),(16,1301),(20,2)):
    altered=dict(analytics,fields=analytics['fields'].copy());altered['fields'][index]=value
    invalid.append(encode_frame(*args,altered))
def checked(body):
    h=2166136261
    for b in body:h=((h^b)*16777619)&0xffffffff
    return body+f'*{h:08x}\n'.encode()
body=new_frame.split(b'*')[0]
invalid.append(checked(body.replace(('0'*180+'45').encode(),('0'*180+'65').encode())))
invalid.append(checked(body[:-3]+b'fff'))
invalid.append(checked(body[:-1]))
result=subprocess.run([sys.argv[1]],input=b'X'*1100+b'\n'+b''.join(invalid)+new_frame,capture_output=True,check=True)
assert result.stdout.splitlines()==[b'43 120 1050 10000000099 5736 1 9999 2 -1 0',b'A 1 120 1050 4 1000'],result.stdout
print('CT4 analytics, bounded framing and semantic rejection: PASS')
