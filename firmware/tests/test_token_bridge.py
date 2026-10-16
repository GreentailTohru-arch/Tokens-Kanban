import datetime as dt
import json
from pathlib import Path
import sys
import tempfile
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'companion'))
from codex_tokens import UsageIndex,quota_values,encode_frame
from token_pricing import request_nano_usd
UTC=dt.timezone.utc
def event(timestamp,totals,last=None):
    keys=('input_tokens','cached_input_tokens','output_tokens')
    return json.dumps({'timestamp':timestamp,'type':'event_msg','payload':{'type':'token_count','info':{
        'total_token_usage':dict(zip(keys,totals)),'last_token_usage':dict(zip(keys,last or totals))}}})+'\n'
class BridgeTests(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory();self.root=Path(self.tmp.name);(self.root/'sessions').mkdir();self.path=self.root/'sessions/a.jsonl';self.index=UsageIndex(self.root)
    def tearDown(self):self.tmp.cleanup()
    def test_deltas_duplicates_forks_and_local_midnight(self):
        a=event('2026-10-15T15:59:00Z',(100,50,10));b=event('2026-10-15T16:01:00Z',(160,70,20),(60,20,10))
        self.path.write_text(a+b+b)
        (self.root/'sessions/fork.jsonl').write_text(a+b)
        self.index.refresh();now=dt.datetime(2026,10,16,8,tzinfo=dt.timezone(dt.timedelta(hours=8)))
        self.assertEqual(self.index.totals(now),[[60,20,10],[160,70,20],[160,70,20]])
    def test_append_partial_and_counter_reset(self):
        a=event('2026-10-16T01:00:00Z',(100,50,10))
        b=event('2026-10-16T02:00:00Z',(30,10,5))
        self.path.write_text(a+b[:20]);self.index.refresh()
        now=dt.datetime(2026,10,16,8,tzinfo=UTC);self.assertEqual(self.index.totals(now)[0],[100,50,10])
        with self.path.open('a') as f:f.write(b[20:])
        self.index.refresh();self.assertEqual(self.index.totals(now)[0],[130,60,15])
    def test_truncated_first_record_uses_last(self):
        self.path.write_text(event('2026-10-16T01:00:00Z',(9999,100,50),(10,5,2)))
        self.index.refresh();self.assertEqual(self.index.totals(dt.datetime(2026,10,16,8,tzinfo=UTC))[0],[10,5,2])
    def test_missing_and_corrupt(self):
        self.path.write_text('{broken}\n');self.index.refresh();self.assertEqual(self.index.totals()[0],[-1,-1,-1])
    def test_week_boundary(self):
        self.path.write_text(event('2026-10-11T12:00:00Z',(100,50,10))+event('2026-10-12T12:00:00Z',(130,60,15)))
        self.index.refresh();self.assertEqual(self.index.totals(dt.datetime(2026,10,16,8,tzinfo=UTC))[1],[30,10,5])
    def test_quota_mapping_nulls_and_multi_bucket(self):
        result={'rateLimits':{'primary':{'usedPercent':99,'windowDurationMins':300}},'rateLimitsByLimitId':{'codex':{'planType':'prolite','primary':{'usedPercent':13,'windowDurationMins':10080,'resetsAt':1100}}},'rateLimitResetCredits':{'availableCount':1}}
        self.assertEqual(quota_values(result,1000),[13,10080,100,-1,-1,-1,1,3])
        self.assertEqual(quota_values({},1000),[-1]*7+[0])
    def test_frame_checksum_and_large_counts(self):
        frame=encode_frame(1,[[10000000000,1,3]]*3,[-1]*7+[0,-1]);body,checksum=frame.strip().split(b'*');h=2166136261
        for b in body:h=((h^b)*16777619)&0xffffffff
        self.assertEqual(checksum,f'{h:08x}'.encode());self.assertLess(len(frame),512)
    def lifecycle(self,kind,turn='a',stamp='2026-10-16T01:00:00Z'):
        return json.dumps({'timestamp':stamp,'type':'event_msg','payload':{'type':kind,'turn_id':turn}})+'\n'
    def test_activity_lifecycle_and_stale(self):
        now=dt.datetime(2026,10,16,1,tzinfo=UTC).timestamp()
        self.assertEqual(self.index.activity(now),0)
        self.path.write_text(self.lifecycle('task_started'));self.index.refresh()
        self.assertEqual(self.index.activity(now+10),2)
        self.assertEqual(self.index.activity(now+900),0)
        with self.path.open('a') as f:f.write(self.lifecycle('task_complete',stamp='2026-10-16T01:01:00Z'))
        self.index.refresh();self.assertEqual(self.index.activity(now+61),1)
    def test_activity_multi_turn_fork_and_abort(self):
        now=dt.datetime(2026,10,16,1,3,tzinfo=UTC).timestamp()
        self.path.write_text(self.lifecycle('task_started'))
        fork=self.root/'sessions/fork.jsonl';fork.write_text(self.lifecycle('task_started')+self.lifecycle('task_complete',stamp='2026-10-16T01:01:00Z'))
        self.index.refresh();self.assertEqual(self.index.activity(now),1)
        with self.path.open('a') as f:f.write(self.lifecycle('task_started','b','2026-10-16T01:02:00Z'))
        self.index.refresh();self.assertEqual(self.index.activity(now),2)
        with self.path.open('a') as f:f.write(self.lifecycle('turn_aborted','b','2026-10-16T01:03:00Z'))
        self.index.refresh();self.assertEqual(self.index.activity(now),1)
    def test_archived_task_does_not_make_green(self):
        (self.root/'archived_sessions').mkdir()
        (self.root/'archived_sessions/task.jsonl').write_text(self.lifecycle('task_started'))
        self.index.refresh();self.assertEqual(self.index.activity(dt.datetime(2026,10,16,1,tzinfo=UTC).timestamp()),0)
    def test_quota_does_not_round_up_to_exhausted(self):
        value=quota_values({'rateLimits':{'primary':{'usedPercent':99.9,'windowDurationMins':300}}},0)
        self.assertEqual(value[0],99)
    def test_v2_activity_frame(self):
        self.assertTrue(encode_frame(1,[[1,0,1]]*3,[-1]*7+[0,-1],2).startswith(b'CT2 '))
        for bad in (-1,3,True):
            with self.assertRaises(ValueError):encode_frame(1,[[1,0,1]]*3,[-1]*7+[0,-1],bad)

class PricingTests(unittest.TestCase):
    setUp=BridgeTests.setUp
    tearDown=BridgeTests.tearDown
    lifecycle=BridgeTests.lifecycle
    def context(self,model):
        return json.dumps({'type':'turn_context','payload':{'model':model}})+'\n'
    def priced_event(self,stamp,total,last=None,writes=0):
        obj=json.loads(event(stamp,total,last))
        obj['payload']['info']['last_token_usage']['cache_write_input_tokens']=writes
        return json.dumps(obj)+'\n'
    def test_cache_subsets_and_long_context(self):
        usage=dict(input_tokens=100000,cached_input_tokens=60000,cache_write_input_tokens=10000,output_tokens=20000)
        self.assertEqual(request_nano_usd('gpt-6.1-sol',usage),291000000)
        self.assertEqual(request_nano_usd('gpt-6-astra',usage),1485000000)
        for size,expected in ((272000,544020000),(272001,1088034000)):
            self.assertEqual(request_nano_usd('gpt-6.1-sol',dict(input_tokens=size,cached_input_tokens=0,cache_write_input_tokens=0,output_tokens=2)),expected)
        self.assertIsNone(request_nano_usd('unknown',usage))
        usage['cache_write_input_tokens']=50000
        self.assertIsNone(request_nano_usd('gpt-6.1-sol',usage))
    def test_models_forks_periods_and_partial(self):
        a=self.context('gpt-6.1-sol')+self.priced_event('2026-10-15T15:59:00Z',(100000,60000,20000),writes=10000)
        b=self.context('gpt-6-astra')+self.priced_event('2026-10-15T16:01:00Z',(200000,120000,40000),(100000,60000,20000),10000)
        self.path.write_text(a+b)
        (self.root/'sessions/fork.jsonl').write_text(a+b)
        self.index.refresh();now=dt.datetime(2026,10,16,12,tzinfo=dt.timezone(dt.timedelta(hours=8)))
        self.assertEqual(self.index.costs(now),[[149,1],[178,1],[178,1]])
        with self.path.open('a') as f:
            f.write(self.context('new-model')+self.priced_event('2026-10-16T01:00:00Z',(200100,120000,40020),(100,0,20)))
        self.index.refresh()
        self.assertEqual(self.index.costs(now),[[149,2],[178,2],[178,2]])
        self.assertEqual(self.index.cost_summary(now)[2]['missingRecords'],1)
    def test_unknown_missing_cache_and_ambiguous_delta(self):
        now=dt.datetime(2026,10,16,8,tzinfo=UTC)
        self.assertEqual(self.index.costs(now),[[-1,0]]*3)
        self.path.write_text(self.context('gpt-6.1-sol')+event('2026-10-16T01:00:00Z',(100,50,10)))
        self.index.refresh();self.assertEqual(self.index.costs(now),[[-1,0]]*3)
        with self.path.open('a') as f:
            f.write(self.priced_event('2026-10-16T02:00:00Z',(500,100,30),(50,0,1)))
        self.index.refresh();self.assertEqual(self.index.costs(now),[[-1,0]]*3)
    def test_round_once_empty_day_and_reset(self):
        self.path.write_text(self.context('gpt-6.1-sol')+self.priced_event('2026-10-15T01:00:00Z',(0,0,400))+
                            self.priced_event('2026-10-15T02:00:00Z',(0,0,800),(0,0,400)))
        self.index.refresh();now=dt.datetime(2026,10,16,8,tzinfo=UTC)
        self.assertEqual(self.index.costs(now),[[0,1],[1,1],[1,1]])
        with self.path.open('a') as f:
            f.write(self.priced_event('2026-10-16T01:00:00Z',(0,0,1000),(0,0,200)))
        self.index.refresh();self.assertEqual(self.index.costs(now)[0],[0,1])
    def test_new_turn_cannot_inherit_previous_model(self):
        self.path.write_text(self.context('gpt-6.1-sol')+self.lifecycle('task_started')+
                            self.priced_event('2026-10-16T02:00:00Z',(10,0,1)))
        self.index.refresh();self.assertEqual(self.index.costs(dt.datetime(2026,10,16,8,tzinfo=UTC))[0],[-1,0])
    def test_ct3_estimate_validation(self):
        args=(1,[[100,0,5]]*3,[-1]*7+[0,-1],2)
        self.assertTrue(encode_frame(*args,[[2736,1],[1000,2],[-1,0]]).startswith(b'CT3 '))
        for costs in ([[0,0]]*3,[[-1,1]]*3,[[1,3]]*3,[[True,1]]*3,[[1,1]]):
            with self.assertRaises(ValueError):encode_frame(*args,costs)

if __name__=='__main__':unittest.main()
