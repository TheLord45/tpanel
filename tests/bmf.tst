# Tests the command ^BMF
# set frame
command=^BMF-1,1,%BCircle 25
result=Circle 25
compare=true
exec
# ---
command=^BMF-1,1,%B4
result=Picture Frame
compare=true
exec
# Set Font
command=^BMF-1,1,%F3
result=3
compare=true
exec
# ----
command=^BMF-1,1,%FArial
result=12
compare=true
exec
# Position/Size
command=^BMF-1,1,%R30,100,180,200
result=30,100,150,100
compare=true
exec
# Text
command=^BMF-1,1,%TShow me
result=Show me
compare=true
exec
# ----
command=^BMF-3,1,%J2
result=2,0,0
compare=true
exec
# ----
command=^BMF-3,1,%JT0,2,2
result=0,2,2
compare=true;
exec
# ----
command=^BMF-3,1,%JT5
result=5,0,0
compare=true;
exec
# Colors
command=^BMF-3,1,%CFCyan
result=Cyan
compare=true
exec
# ----
command=^BMF-3,1,%CBYellow
result=Yellow
compare=true;
exec
# ----
command=^BMF-3,1,%CTPurple
result=Purple
compare=true
exec
# Show/Hide
command=^BMF-3,1,%SW0
result=FALSE
compare=true
exec
# ----
command=^BMF-3,1,%SW1
wait=500
result=TRUE
compare=true
exec
# Enable/Disable
command=^BMF-3,1,%EN0
result=FALSE
compare=true
exec
# ----
command=^BMF-3,1,%EN1
result=TRUE
compare=true
exec
# Sound
command=^BMF-1,1,%OTMomentary
result=4
compare=true
exec
# ----
command=^BMF-1,1,%SOdocked.mp3
result=docked.mp3
compare=true
exec
# ----
command=CLICK-35,110
exec
# ----
command=@PPF-_WhatItIs
screenwait=true
exec
