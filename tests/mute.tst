# Tests whether the setting of system mute works or not.
command=^MUT-1
result=1
compare=true
exec
# ---
command=^MUT-0
result=0
compare=true
exec
