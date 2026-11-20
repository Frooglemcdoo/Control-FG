from pathlib import Path
s = (Path(__file__).resolve().parents[1] / "src/rr_native_frame.h").read_text()
hook = s[s.index("static void RRNativeHookFilter("):]
assert hook.index("ResetHistoryBeforeFilter(rrNativeFrame,rrNativeHistory)") < hook.index("if(control_reblur::Active())")
assert hook.index("ResetHistoryBeforeFilter(rrNativeFrame,rrNativeHistory)") < hook.index("rrNativeFilterOriginal(")
print("PASS: history reset precedes REBLUR admission and every native filter route")
