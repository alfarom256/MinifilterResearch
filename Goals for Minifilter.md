### Goals for PoC Minifilter
1. Allow writes to a file but prevent reading the data back correctly unless the process is our target process
2. Use a driver to read a filter from the user and remove the callback for that IRP_MJ function
3. Use a vulnerable driver with r/w to enumerate the list of filters and remove callbacks 