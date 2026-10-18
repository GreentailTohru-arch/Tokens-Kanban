"""Pet status from lifecycle/tool metadata, never prompts or microphone access."""
import time

COMPUTER_TOOLS=('exec_command','write_stdin','mcp__cua_repl','mcp__edge_devtools')

def observe_tool(state,record,payload):
    kind=payload.get('type');call=payload.get('call_id')
    if not isinstance(call,str):return
    if kind in ('function_call','custom_tool_call'):
        name=payload.get('name','')
        if isinstance(name,str) and any(key in name for key in COMPUTER_TOOLS):
            import datetime as dt
            stamp=dt.datetime.fromisoformat(record['timestamp'].replace('Z','+00:00'))
            if stamp.tzinfo is not None:state.setdefault('pet_tools',{})[call]=stamp.timestamp()
    elif kind in ('function_call_output','custom_tool_call_output'):
        state.setdefault('pet_tools',{}).pop(call,None)

class PetTracker:
    def state(self,index,now=None):
        now=time.time() if now is None else now
        if index.activity(now)==2:
            for path,state in index.files.items():
                if state['readable'] and state.get('active') and 'archived_sessions' not in path.relative_to(index.home).parts:
                    if any(0<=now-stamp<120 for stamp in state.get('pet_tools',{}).values()):return 5
            return 2
        return 1
