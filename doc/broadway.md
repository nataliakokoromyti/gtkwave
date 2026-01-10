Broadway Backend
================

GTKWave can run with the GTK Broadway backend and be accessed in a browser.
This is useful for remote use or environments without X11/XQuartz.

Quick start
-----------

1) Start the Broadway server:

    broadwayd :5

2) In another terminal, start GTKWave with the Broadway backend:

    GDK_BACKEND=broadway BROADWAY_DISPLAY=:5 ./build/src/gtkwave ./examples/des.fst

3) Open the Broadway URL in a browser:

    http://127.0.0.1:8085/

Notes
-----

- The Broadway server listens on port 8085 by default.
- `BROADWAY_DISPLAY` must match the display passed to `broadwayd`.
- If you are running in WSL or on a remote machine, use the host's browser
  and ensure the port is reachable.
