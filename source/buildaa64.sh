export PATH=${PATH}:$(pwd)/../../buildroot-2022.02.10/output/host/bin/
#export

# Building SDL1.2 Application
make -f Makefile.aa64 clean
make -f Makefile.aa64 -j32
