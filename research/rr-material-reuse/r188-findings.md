# R188 feedback

User supplied Lighting-R188-20261027-180001-22264-0.log (volume) and
Lighting-R188-20261027-180210-9552-0.log (cube), plus
image(20261027-230439).png and image(20261027-230443).png.

Both modes report gain 8, direct_created=1, library_created=0, failures=0.
SAVE LOG snapshots have RR off; runs contain RR-on testing. User explicitly
confirmed the noisy painting image is option 2 with RR ON, resolving the
screenshot's absent overlay. Do not invalidate that report from SAVE LOG's
later toggle state. User says option 3 was too dark to tell but might have
looked good. Record cube quality as inconclusive, not passed.

The painting shows strong mottled/granular detail in the isolated volume
signal with RR. This does not prove volume lighting is the only source, nor
that the diagnostic 8x brightness has the same RR response as full lighting.
The next experiment restores full normal-brightness lighting at SPEC4 and
tests secondary-roughness angular broadening of the volume specular lookup.
No repeated SPEC8/16 test or exact camera matching is required.
