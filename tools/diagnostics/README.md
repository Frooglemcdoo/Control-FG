# Developer diagnostics

Use the compact collector in the source root for normal support. These tools are retained for development and are not included in the deployment ZIP. Run them from an extracted source tree.

| Tool | Purpose |
| --- | --- |
| `Collect-ControlFG-Logs.ps1` | Detailed source-tree collection; also used by `Manage-Probe.ps1 -Action Collect`. |
| `Collect-LiveGuides.ps1` | Helper loaded by the detailed collector. |
| `Collect-RR-Inputs.cmd` / `.ps1` | Collect completed C1 RR input captures from diagnostic runs. |
| `Collect-Alignment-Logs.cmd` / `.ps1` | Historical R12 alignment capture; requires that diagnostic build. |
| `Collect-Epic-Compatibility.cmd` | Read-only Epic binary compatibility report. Requires Python 3. |
| `Collect-GOG-Compatibility.cmd` | Read-only GOG binary compatibility report. Requires Python 3. |

The wrappers locate their dependencies relative to this source tree. The detailed, RR-input and alignment collectors write outputs at the project root by default. Storefront probes write to the project root's `compatibility-output` folder.
