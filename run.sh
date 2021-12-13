rm -f /mnt/pmem/log
rm -f ./core
make clean
make 
./test/test >output
# ./test/test