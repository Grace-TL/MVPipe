# MVPipe

This project implements MVPipe, which is a novel algorithm for Hierarchical Heavy Hitters (HHH)
detection. The project includes both the c++ and P4 implementation of MVPipe. 

---

### Files
- main\_mv.cpp: example about 1D HHH detection
- main\_mv2D.cpp: example about 2D HHH detection
- src/mvpipe.hpp, src/mvpipe.cpp: c++ implementation of MVPipe for 1D HHH
  detection
- src/mvpipe2D.hpp, src/mvpipe2D.cpp: c++ implementation of MVPipe for 2D HHH
  detection
- p4/mvpipe.p4: p4 implementation of MVPipe 
- traces/: ip trace folder
---

### Compile and run the examples
We show how to compile the C++ implementation of MVPipe on Ubuntu with g++ and make.

#### Requirements
- Ensure __g++__ and __make__ are installed.  Our experimental platform is
  equipped with Ubuntu 14.04, g++ 4.8.4 and make 3.81.

- Ensure the necessary library [PcapPlusPlus](https://pcapplusplus.github.io/docs/) is installed.

- Prepare the pcap files.
    - We provide two small test pcap files
      [here](https://drive.google.com/file/d/1BehIgLaL9uB4TnXY4boiHYbPU0_rqWzE/view?usp=sharing).
      You can download and put them in the "traces" folder for testing.
    - Specify the path of each pcap file in "iptraces.txt". 
    - Note that one pcap file is regarded as one epoch in our examples. 


#### Compile
- Compile examples with make

```
    # 1D-byte HHH detection
    $ make mvpipe_byte  
    
    #example of 1D-bit HHH detection
    $ make mvpipe 
    
    #example of 2D-byte HHH detection
    $ make mvpipe2D_byte 
    
    #example of 2D-bit HHH detection
    $ make mvpipe2D   
```

#### Run
- Run the examples, and the program will output some statistics about the
  detection results.

```
# perform 1D-byte HHH detection with the absolute threshold 1000 and 256 KiB
memory
$./mv_byte 100000 256
```
- Note that you can change the threshold and memory for testing 

