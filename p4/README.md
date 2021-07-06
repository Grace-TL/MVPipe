# P4 implementation of MVPipe 


---
### Files
- p4/mvpipe.p4: P4 source code of MVPipe 
---

### Compile and Run
The implementations should be run on the BMV2 v1model. We show how to compile the p4 files with
p4 compiler.

#### Requirements
- Install the p4 behavioral model (follow the steps [here](https://github.com/p4lang/behavioral-model)).
- Install the p4 compiler ([details](https://github.com/p4lang/p4c))

#### Compile
- Compile p4 files with p4 compiler

```
    $ p4c --target bmv2 --arch v1model --std p4-16 mvpipe.p4
```

#### Run

- Create virtual Ethernet devices with the script under your
  behavioral\_model directory

```
    $ sudo yourpath/behaviroal_model/tools/veth_setup.sh
```

- Run the behavioral model 

```
    $ sudo simple_switch --log-console -i 0@veth2 -i 1@veth4 mvpipe.json
```

- Send packets to the veth via python scapy package 

- The results can be pulled from the register arrays via the CLI of BMV2

- Note that our implementations only include the simple forwarding rules based
  on ingress ports. For example, packets entering from port 0 are emitted to
  port 1. You may add your own forwarding logic in the ingress pipeline.

