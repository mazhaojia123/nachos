rm DISK
./nachos -f
./nachos -cp test/small /small
./nachos -cp test/medium /medium
./nachos -cp test/big /big
./nachos -cp test/small /small
./nachos -cp test/medium /medium
./nachos -cp test/big /big
hexdump -C DISK
./nachos -D
