# Material animation and rebuild dependency audit

This is an offline audit following R224, not a new runtime build or an FPS claim.

## Verified field identity

The native update producer is renderer RVA 0xC51D0. At 0xC68BE it takes the address of the material record at rbp-0x80. Parameter lookups use named identifiers initialized earlier in this same function. Their output addresses establish:

| Record offset | Native parameter | Evidence |
| --- | --- | --- |
| 0x30 | g_fEmissionIntensity | Output rbp-0x50 at 0xC628A |
| 0xC0–0xCF | g_vWrinkleWeights0 | Output rbp+0x40 at 0xC6551 |
| 0xD0–0xDF | g_vWrinkleWeights1 | Output rbp+0x50 at 0xC6577 |
| 0xE0–0xEF | g_vWrinkleWeights2 | Output rbp+0x60 at 0xC659D |
| 0xF0–0xFF | g_vWrinkleWeights3 | Output rbp+0x70 at 0xC65C3 |
| 0x100–0x10F | g_vWrinkleWeights4 | Output rbp+0x80 at 0xC65E9 |
| 0x110–0x11F | g_vWrinkleWeights5 | Output rbp+0x90 at 0xC6612 |

`tools/audit-native-material-fields.py` reproduces the mappings from the locally supplied renderer, including the parameter-string references and stack-relative output destinations. The generated JSON records all recovered fields. No field meaning is inferred solely from a numeric value.

## R224 evidence

In reports 7080–10680 of PID31372, 713 sampled changes cover 38 entry IDs on one manager. Entry4332 accounts for 454 samples; every changed offset lies in the wrinkle-weight range. The other37 entries account for259 samples, each changing only emission intensity at0x30 between float32 1.0 and approximately0.01. There were no material-key changes, zero-byte output differences, dropped events or invalid diagnostic inputs. IDs are run-local; periodic sampling cannot establish unbiased relative frequency. The logs do not identify the character or object owning entry4332, nor whether the emission alternation is intentional or redundant.

These are meaningful renderer inputs. Excluding them from hashing or freezing their values would not be a behavior-preserving CPU optimization.

## Why a simple per-entry GPU upload is insufficient

The native rebuild copies entry hashes and entry indices to the sort array at0xC7D11–0xC7D49. It calls a radix-style sort at0xC7D67 (routine0xC9C40 builds four byte histograms). At0xC7D81–0xC7DC7 it groups equal hashes and writes resulting output IDs into native handles. At0xC7DC9 it sets the output-record count, then uploads records and resolves35 texture descriptors for each output record.

Changing an animation field therefore changes the sorting/deduplication key. It can move a record, split a shared record or merge records. R224's output count actually alternates between2085 and2086 while the entry count stays4904. Updating only the previous GPU slot without handling that mapping would be incorrect.

## Texture binding and lifetime evidence

The regular texture-binding helper at0xC9910 already skips its replacement path when the texture pointer is unchanged (branch0xC992B→0xC99BA). The previously inspected helper0xC9A10 is a different path; its unconditional dirty write must not be generalized to every texture update. On replacement,0xC9910 removes the old listener, invokes old-object virtual+0x48, stores the new pointer, invokes new-object virtual+0x40, and registers the dirty callback0xC7BB0. These virtual calls suggest release/retain behavior, but their names and complete streaming synchronization contract remain unverified.

The measured texture-resolution loop is0xC8122–0xC81AD. A candidate optimization should preserve animated material data, native sort/dedup mapping and texture-change callbacks while reducing repeated resolution for unchanged bindings. Persistent descriptor reuse is not yet proven safe against streaming/resource retirement. No cache, suppression or speculative parallel job is installed by this audit.

## Decision

Keep R224 available for existing diagnosis; no further broad capture is needed to identify the fields. Continue the implementation audit at descriptor resolution and its invalidation/lifetime boundary. Do not present an untested cache as a fix or ask for another identical field-diff run.
