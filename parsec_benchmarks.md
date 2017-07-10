    *blackscholes* - Option pricing with Black-Scholes Partial Differential Equation (PDE)
    bodytrack - Body tracking of a person
    canneal - Simulated cache-aware annealing to optimize routing cost of a chip design
    dedup - Next-generation compression with data deduplication
    There is an issue with Perl's documentation files. You need to issue this command to remove part of the documentation so it installs just fine
    sed -i '/=item 2/d' ../../parsec-3.0/pkgs/libs/ssl/src/doc/ssl/*.pod (also other things such as =over need to be removed from the documentation files)
    
    facesim - Simulates the motions of a human face  [
    you've to create a soft link to Face_Data directory from the directory you're running this script to make this work]
    + only runs for a specific nubmer of threads: like 1, 2, 3, 4, 6, 8, 16, 32
    
    
    ferret - Content similarity search server [generates more threads than it should with the -n command. for -n = 1 it generates 6 threads, for -n = 2, it generates
    10, for n = 3 12 and for n = 4 18, and for n = 5 it generates 22 threads]

    fluidanimate - Fluid dynamics for animation purposes with Smoothed Particle Hydrodynamics (SPH) method
     number of threads must be a power fo2 
    
    raytrace - Real-time raytracing [it takes 50seconds to actually spawn the real threads!! cray cray]]]
    
    streamcluster - Online clustering of an input stream
    swaptions - Pricing of a portfolio of swaptions
    
    
    vips - Image processing [in order to pass the number of threads to this benchmarks you actually have to change a variable defined in the 
    .runconf files found inside the parsec directory. 
    actually you have to change this ... "export IM_CONCURRENCY=20"
    https://lists.cs.princeton.edu/pipermail/parsec-users/2010-May/000762.html/]
    
    
    freqmine - Frequent itemset mining  [X   -> doesn't support pthreads]
    x264 - H.264 video encoding [throws *** Error in `/localhome/kantonia/slate/benchmarks/parsec-3.0/bin/../pkgs/apps/x264/inst/amd64-linux.gcc-pthreads/bin/x264': double free or corruption (!prev): 0x0000000000d37020 ***
when it starts running


Have a look here: http://lists.cslab.ece.ntua.gr/pipermail/advcomparch/2017-April/001467.html for problems with ferret & facesim.
