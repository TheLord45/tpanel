# Tests the push and release functions.
# The test simulates a mouse click.
command=PUSH-330,30
exec
# ---
command=RELEASE-330,30
exec
# ---
command=PUSH-330,30
wait=1500
exec
# ---
command=RELEASE-330,30
wait=500
exec
