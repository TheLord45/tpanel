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
