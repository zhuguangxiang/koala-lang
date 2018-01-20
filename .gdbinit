# filename: .gdbinit
# gdb will read it when starting
# set auto-load safe-path / in ~/.gdbinit to prevent the warning of 'Auto-Load Safe-Path Declined'
handle SIGUSR1 nostop noprint
