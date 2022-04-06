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
./nachos -ap test/small /path1/small
./nachos -ap test/medium /path1/medium
./nachos -ap test/big /path1/big
./nachos -ap test/small /path2/small
./nachos -ap test/medium /path2/medium
./nachos -ap test/big /path2/big
./nachos -ap test/small /path1/path3/path4/path5/small
./nachos -ap test/small /path1/path3/path6/small
./nachos -hap test/small /path1/small
./nachos -hap test/medium /path1/medium
./nachos -hap test/big /path1/big
./nachos -hap test/small /path2/small
./nachos -hap test/medium /path2/medium
./nachos -hap test/big /path2/big
./nachos -hap test/small /path1/path3/path4/path5/small
./nachos -hap test/small /path1/path3/path6/small
./nachos -nap /path1/medium /path1/small
./nachos -nap /path1/big /path1/medium
./nachos -nap /path2/small /path1/big
./nachos -nap /path2/medium /path2/small
./nachos -nap /path2/big /path2/medium
./nachos -nap /path1/path3/path4/path5/small /path1/small
./nachos -nap /path1/path3/path6/small /path1/small
hexdump -C DISK
./nachos -D
