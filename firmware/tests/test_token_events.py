import json
from pathlib import Path
import sys
import tempfile
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'companion'))
from token_events import CompletionTracker
from codex_tokens import UsageIndex

class Connection:
    def __init__(self):self.commands=[];self.fail=False
    def write(self,data):self.commands.append(data.decode().strip())
    def readline(self,size):
        if self.fail:raise OSError('disconnect')
        return ('PC_EVT_ACK '+' '.join(self.commands[-1].split()[1:3])+'\n').encode()

class EventsTests(unittest.TestCase):
    def test_baseline_fork_retry_and_stale(self):
        e=CompletionTracker();c=Connection();e.observe({('old',99)},100);e.send(c);self.assertFalse(c.commands)
        e.observe({('old',99),('new',101)},102);c.fail=True
        with self.assertRaises(OSError):e.send(c)
        command=c.commands[-1];self.assertIsNotNone(e.pending)
        c.fail=False;e.send(c);self.assertEqual(c.commands[-1],command);self.assertIsNone(e.pending)
        e.observe({('new',101)},103);e.send(c);self.assertEqual(len(c.commands),2)
        e.observe({('stale',1)},104);self.assertIsNone(e.pending)
        e.observe({('later',105)},106);self.assertIsNotNone(e.pending);e.observe(set(),167);self.assertIsNone(e.pending)
    def test_explicit_completion_only_and_archives(self):
        def event(kind,turn):return json.dumps({'timestamp':'2026-10-16T01:00:00Z','type':'event_msg','payload':{'type':kind,'turn_id':turn}})+'\n'
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);(root/'sessions').mkdir();(root/'archived_sessions').mkdir()
            (root/'sessions/a.jsonl').write_text(event('task_started','a')+event('task_complete','a')+event('task_started','b')+event('turn_aborted','b'),encoding='utf-8')
            (root/'sessions/fork.jsonl').write_text(event('task_complete','a'),encoding='utf-8')
            (root/'archived_sessions/x.jsonl').write_text(event('task_complete','x'),encoding='utf-8')
            index=UsageIndex(root);index.refresh();self.assertEqual(index.completions(),{('a',1792112400.0)})

if __name__=='__main__':unittest.main()
