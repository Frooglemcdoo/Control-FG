# Known issues — v2.1.0

## RR and indirect diffuse lighting

Blinds can exhibit crawling/noisy streaks when RR and Ray Traced Indirect Diffuse Lighting are both enabled, on RTX 40- and 50-series GPUs. E/F selection does not resolve it. Disable Ray Traced Indirect Diffuse Lighting or RR as a workaround. Investigation is ongoing.

## RTX 40-series overlay pacing

Periodic frame-time spikes while the overlay is visible have been reported on an RTX 4070. A confirmed fix is not included. Close the overlay during gameplay if affected.

## Windows security report

A Windows security block was reported on another machine. The exact detection name and affected file have not yet been collected, so the cause and false-positive status are unconfirmed. Download reputation warnings and Defender Antivirus detections are different diagnoses.

For a report, include the detection name, affected filename, file SHA-256, and Windows Security Protection History screenshot. Do not disable antivirus protection. Suspected false positives can be submitted to Microsoft at https://www.microsoft.com/en-us/wdsi/filesubmission.

## HDR transitions

If an HDR change causes ghosting or corruption, restart Control with the desired HDR setting and collect compact logs if it persists. Removing the overlay footer is a UI change, not a new rendering fix.
