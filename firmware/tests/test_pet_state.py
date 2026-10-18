import sys,unittest,tempfile,datetime as dt,json
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'companion'))
from pet_state import PetTracker,observe_tool
from codex_tokens import UsageIndex

class PetTest(unittest.TestCase):
    def test_session_tail_tool_start_and_completion(self):
        with tempfile.TemporaryDirectory() as root:
            path=Path(root)/'sessions/a.jsonl';path.parent.mkdir()
            def append(stamp,kind,payload):
                with path.open('a',encoding='utf-8') as f:f.write(json.dumps(dict(timestamp=dt.datetime.fromtimestamp(stamp,dt.timezone.utc).isoformat(),type=kind,payload=payload))+'\n')
            append(100,'event_msg',dict(type='task_started',turn_id='n'))
            idx=UsageIndex(Path(root));p=PetTracker();idx.refresh();self.assertEqual(p.state(idx,100),2)
            append(101,'response_item',dict(type='function_call',name='functions.exec_command',call_id='c',arguments='PRIVATE SENTINEL'))
            idx.refresh();self.assertEqual(p.state(idx,101),5);self.assertNotIn('PRIVATE SENTINEL',str(idx.files))
            append(102,'response_item',dict(type='function_call_output',call_id='c',output='PRIVATE SENTINEL'))
            idx.refresh();self.assertEqual(p.state(idx,102),2);self.assertNotIn('PRIVATE SENTINEL',str(idx.files))
            append(103,'event_msg',dict(type='task_complete',turn_id='n'))
            idx.refresh();self.assertEqual(p.state(idx,103),1)
    def test_lifecycle_and_no_history_completion(self):
        with tempfile.TemporaryDirectory() as root:
            idx=UsageIndex(Path(root));path=Path(root)/'sessions/a.jsonl'
            st=dict(readable=True,active=None,turns={'t':(1,95)},completions={'t':95},pet_tools={})
            idx.files[path]=st;p=PetTracker();self.assertEqual(p.state(idx,100),1)
            st.update(active='n',turns={'n':(2,101)})
            self.assertEqual(p.state(idx,101),2)
            stamp=dt.datetime.fromtimestamp(102,dt.timezone.utc).isoformat()
            observe_tool(st,{'timestamp':stamp},{'type':'function_call','name':'functions.exec_command','call_id':'c','arguments':'PRIVATE'})
            self.assertNotIn('PRIVATE',str(st));self.assertEqual(p.state(idx,103),5)
            observe_tool(st,{}, {'type':'function_call_output','call_id':'c'})
            self.assertEqual(p.state(idx,104),2)
            st.update(active=None,turns={'n':(1,105)},completions={'t':95,'n':105})
            self.assertEqual(p.state(idx,105),1);self.assertEqual(p.state(idx,111),1)
            st.update(active='n',turns={'n':(2,112)},pet_tools={'old':-20})
            self.assertEqual(p.state(idx,112),2)
if __name__=='__main__':unittest.main()
