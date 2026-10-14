M12R50 - register both reviewed native primary lighting variants

R49 logs show the standard primary shader registering while the supported
single-spot variant does not. Once native RT is enabled, environment and opaque
callbacks advance but primary callbacks stop. Remove the extra standard-only
name filter; retain exact bytecode size/hash and root-layout validation for both
reviewed variants. Native shader execution and its extra u2 output are preserved.
Revert R49's unsuccessful CBV observer/locking optimization to the R48 path.

Close Control, extract this package, run PT-Enable.cmd, then restart.
Enable native RT: the current PT implementation needs its scene/environment
capture. Select your preferred HDR setting. This package does not change those
preferences. Keep FG off and the same scene/camera for comparisons.
Toggle PT and check lighting, reflections and Jesse's shading. Compare DLAA and
Quality with PT off and on. Send Collect-PT-Primary-M12R50.cmd's result ZIP.
No large ETL is needed. RT-Only.cmd plus restart restores capture-off operation.

This fixes an identified PT activation gate, not a verified performance fix.
The cause of the reported initial RT/HDR setting changes remains unconfirmed.
PT off in an enabled session still captures geometry. Shaders are unchanged.
