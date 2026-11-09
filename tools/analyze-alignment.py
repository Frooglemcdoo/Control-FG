"""Summarize CPU provenance; not a pixel/GPU-completion verdict."""
import collections,json,re,sys
from pathlib import Path
rows=[]
for line in Path(sys.argv[1]).read_text(errors='replace').splitlines():
 if re.search(r'(?:^| )ALIGN_BRIDGE ',line):
  rows.append(dict(re.findall(r'(\w+)=([^\s]+)',line)))
valid=[x for x in rows if int(x['matched_sequence']) and x['epoch']==x['matched_epoch']]
print(json.dumps({'bridge_samples':len(rows),'same_epoch_matched_samples':len(valid),'unmatched_or_prior_epoch':len(rows)-len(valid),'present_minus_producer_target':dict(collections.Counter(int(x['present'])-int(x['matched_target']) for x in valid)),'not_latest_source':sum(x['source_is_latest']=='0' for x in rows),'interpretation':'CPU resource provenance only; compare healthy and broken gameplay. No GPU pixel or completion proof.'},indent=2))
