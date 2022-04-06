rm DISK
./nachos -f
./nachos -cp test/small /path1/small
./nachos -cp test/medium /path1/medium
./nachos -cp test/big /path1/big
./nachos -cp test/small /path2/small
./nachos -cp test/medium /path2/medium
./nachos -cp test/big /path2/big
./nachos -cp test/small /path1/path3/path4/path5/small
./nachos -cp test/small /path1/path3/path6/small
hexdump -C DISK
./nachos -D
