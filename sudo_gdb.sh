# allow debug in sudoer mode
# precondition 
    #  `sudo vim /etc/sudoers` 
    #  add "user_name" ALL=(ALL) NOPASSWD:/usr/bin/gdb
    #  `sudo chmod 0440 /etc/sudoers`
# how to use :
    #  use this script as debugger instead of /usr/bin/gdb
sudo /usr/bin/gdb "$@"