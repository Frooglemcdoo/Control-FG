"""Generate isolated validation shaders, never used by the release workflow."""
import character_surface
import character_diffuse
from character_coverage_guard import guarded_diffuse

# guarded_diffuse retains its original imported filtered_diffuse function.
# Only this process substitutes the candidate in the generated validation set.
character_diffuse.filtered_diffuse = lambda data, info: guarded_diffuse(data, info)[0]
character_surface.main()
print('Prepared coverage-guard candidates on the current HQ and diagnostic variants')
