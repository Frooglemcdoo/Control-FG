# R155 — capture handoff and delay

CAPTURE LIGHT now closes the diagnostic panel using the shared ClosePanel routine, restores the cursor state and returns focus to the game. This resumes gameplay paused by losing focus to the overlay. Previously the button only changed focus and left the panel visible.

Capture waits for five seconds of consecutive evaluated gameplay before recording; resets, frame gaps and failed evaluations restart the timer. This is nonblocking and applies to both capture positions. Reopen F10 to check Saved pairs, then use PACKAGE LIGHT after both captures. An explicitly opened game pause menu is not forcibly dismissed by synthetic Escape inputs.

Retains R154 native SR/DLAA capture with NATIVE RR OFF, usual DLSS Quality/DLAA and unchanged RT settings. Normal rendering, exposure, accepted shadow shaders and packaging below 200 MB are unchanged. This is a capture usability update, not a claimed shadow fix. Windows tests do not replace in-game validation.
