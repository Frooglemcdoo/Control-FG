from pathlib import Path
import sys, json, re
root=Path(sys.argv[1] if len(sys.argv)>1 else '.').resolve()
required=['README.md','INSTALL.md','TROUBLESHOOTING.md','BUILDING.md','CHANGELOG.md','ROADMAP.md','NEXUS_MODS.md','NEXUS_MODS_DESCRIPTION.txt','GITHUB_RELEASE.md','RELEASE_CHECKLIST.md','.gitignore','Build.cmd','Fetch-Streamline.ps1','Verify-Streamline.ps1','Verify-Build.ps1','Make-DropIn.ps1','Manage-Probe.ps1','target-manifest.json','src/probe.cpp','src/streamline_bridge.h','src/hdr10_bridge.h','src/fg_overlay.h','legal/STREAMLINE-LICENSE.txt','legal/THIRD-PARTY-NOTICES.md']
checks=[]
def ck(name, value):
    checks.append((name,bool(value)))
    if not value: raise SystemExit('FAIL: '+name)
ck('required-files',all((root/x).is_file() for x in required))
manage=(root/'Manage-Probe.ps1').read_text(encoding='utf-8-sig')
build=(root/'Build.cmd').read_text(encoding='utf-8-sig')
drop=(root/'Make-DropIn.ps1').read_text(encoding='utf-8-sig')
overlay=(root/'src/fg_overlay.h').read_text(encoding='utf-8-sig')
bridge=(root/'src/streamline_bridge.h').read_text(encoding='utf-8-sig')
ck('collector-current-version',"PROBE v0\\.8\\.36" in manage and "PROBE v0\\.8\\.29" not in manage)
ck('fresh-build-stages-sdk','Fetch-Streamline.ps1' in build and 'Verify-Streamline.ps1' in build)
ck('release-zip','Control-FG-v'+"'+$Version+'"+'.zip' in drop or "Control-FG-v'+$Version+'.zip" in drop)
ck('release-docs-packed',all(x in drop for x in ['INSTALL.md','TROUBLESHOOTING.md','RELEASE_NOTES.md','STREAMLINE-LICENSE.txt','THIRD-PARTY-NOTICES.md']))
ck('hidden-overlay','fgOverlayVisible{0}' in overlay and 'SW_HIDE' in overlay)
ck('rtx40-policy','GeForce RTX 40' in bridge and 'allowed=off,2x' in bridge)
ck('dynamic-baseline','eDynamic' in bridge and 'SL_SIMULATION_MARKER_START' in bridge)
ck('no-presentmon-runtime','PresentMon' not in overlay and 'PresentMon' not in bridge)
ck('no-built-binaries',not any(p.suffix.lower() in {'.dll','.exe','.obj','.lib','.pdb'} for p in root.rglob('*') if p.is_file()))
print(f'PASS: {len(checks)} public-release checks')
