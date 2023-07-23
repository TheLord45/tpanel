# Tests button manipulation functions
# Opacity
command=^BOP-2,1,128
result=128
compare=true
exec
# ----
command=^BOP-2,1,255
result=255
compare=true
exec
# Border style
command=^BOR-2,26
result=Neon Active -S
compare=true
exec
# ----
command=^BOR-2,AMX Elite -M
result=AMX Elite -M
compare=true
exec
# Button size and position
command=^BSP-2,right,bottom
result=640,430
compare=true
exec
# Enable / Diasable
command=^ENA-3,0
result=0
compare=true
exec
# ----
command=^ENA-3,1
result=1
compare=true
exec
# Fonts
command=?FON-3,0
saveresult=font
compare=true
result=23
exec
# ----
command=^FON-3,0,26
compare=true
compresult=font
result=26
reverse=true
exec