# Tests commands to manipulate the text of a button
command=^BAT-3,1,-Local
result=Setup ...-Local
compare=true
exec
# ----
command=^BAT-3,2,-Local
result=Setup ...-Local
compare=true
exec
# ----
command=?TXT-3,1
result=Setup ...-Local
compare=true
exec
# ----
command=?TXT-3,2
result=Setup ...-Local
compare=true
exec
# ----
command=^BAT-3,0,-Local
result=Setup ...-Local-Local
compare=true
exec
# ----
command=?TXT-3,1
result=Setup ...-Local-Local
compare=true
exec
# ----
command=^BAU-2,1,cebbceadcebeceb7
result=Copyrightλέξη
compare=true
exec
# ----
command=^TXT-3,0,Setup ...
result=Setup ...
compare=true
exec
# ----
command=^TXT-2,0,Copyright
result=Copyright
compare=true
exec
# ----
command=^UNI-2,0,cebbceadcebeceb7
result=λέξη
compare=true
exec
