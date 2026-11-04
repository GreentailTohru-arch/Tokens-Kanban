"""Pet status based on Codex turn lifecycle."""
class PetTracker:
    def state(self,index,now=None):
        return 2 if index.activity(now)==2 else 1
