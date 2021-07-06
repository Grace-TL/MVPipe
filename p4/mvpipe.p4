/* -*- P4_16 -*- */

#include <core.p4>
#include <v1model.p4>

const bit<16> TYPE_IPV4 = 0x800;

// Headers and Metadata definitions

typedef bit<9>  egressSpec_t;
typedef bit<48> macAddr_t;
typedef bit<32> ip4Addr_t;

#define HASH_BASE 10w0
#define BUCKET0 2048
#define BUCKET1 2048
#define BUCKET2 2048
#define BUCKET3 256
#define BUCKET4 1 



header ethernet_t {
    macAddr_t dstAddr;
    macAddr_t srcAddr;
    bit<16>   etherType;
}

header ipv4_t {
    bit<4>    version;
    bit<4>    ihl;
    bit<8>    diffserv;
    bit<16>   totalLen;
    bit<16>   identification;
    bit<3>    flags;
    bit<13>   fragOffset;
    bit<8>    ttl;
    bit<8>    protocol;
    bit<16>   hdrChecksum;
    ip4Addr_t srcAddr;
    ip4Addr_t dstAddr;
}

struct mvpipe_metadata_t {
    bit<32> hash0;
    bit<32> hash1;
    bit<32> hash2;
    bit<32> hash3;
    bit<32> flag0;
    bit<32> flag1;
    bit<32> flag2;
    bit<32> flag3;
    bit<32> pushkey0;
    bit<32> pushkey1;
    bit<32> pushkey2;
    bit<32> pushkey3;
    int<32> pushval0;
    int<32> pushval1;
    int<32> pushval2;
    int<32> pushval3;
    int<32> tmpval;
    bit<32> tmpkey;
}

struct headers {
    ethernet_t   ethernet;
    ipv4_t       ipv4;
}

//define register arrays
//key field
register<bit<32> >(BUCKET0)  mvpipe_key_0;
register<bit<32> >(BUCKET1)  mvpipe_key_1;
register<bit<32> >(BUCKET2)  mvpipe_key_2;
register<bit<32> >(BUCKET3)  mvpipe_key_3;

//total sum field
register<int<32> >(BUCKET0)  mvpipe_value_0;
register<int<32> >(BUCKET1)  mvpipe_value_1;
register<int<32> >(BUCKET2)  mvpipe_value_2;
register<int<32> >(BUCKET3)  mvpipe_value_3;
register<int<32> >(BUCKET4)  mvpipe_value_4;

//total indicator field
register<int<32> >(BUCKET0)  mvpipe_ind_0;
register<int<32> >(BUCKET1)  mvpipe_ind_1;
register<int<32> >(BUCKET2)  mvpipe_ind_2;
register<int<32> >(BUCKET3)  mvpipe_ind_3;

//total cumulative field
register<int<32> >(BUCKET0)  mvpipe_cum_0;
register<int<32> >(BUCKET1)  mvpipe_cum_1;
register<int<32> >(BUCKET2)  mvpipe_cum_2;
register<int<32> >(BUCKET3)  mvpipe_cum_3;


// Standard IPv4 parser, dummy ingress processing

parser MyParser(packet_in packet,
        out headers hdr,
        inout mvpipe_metadata_t meta,
        inout standard_metadata_t standard_metadata) {

    state start {
        transition parse_ethernet;
    }

    state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
TYPE_IPV4: parse_ipv4;
            default: accept;
        }
    }

    state parse_ipv4 {
        packet.extract(hdr.ipv4);
        transition accept;
    }
}

control MyVerifyChecksum(inout headers hdr, inout mvpipe_metadata_t meta) {
    apply {  }
}


control MyIngress(inout headers hdr,
        inout mvpipe_metadata_t meta,
        inout standard_metadata_t standard_metadata) {
    action drop() {
        mark_to_drop(standard_metadata);	
    }    

    //forward all packets to the specified port
    action set_egr(egressSpec_t port) {
        standard_metadata.egress_spec = port;
    }

    table forward {
        key = {
            standard_metadata.ingress_port: exact;
        }
        actions = {
            set_egr;
            drop;
        }	
        size = 1;	
        default_action = drop();
    }

    apply {
        forward.apply();	
    }

}
// =========== Start implementation of MVPipe ============

control MyEgress(inout headers hdr,
        inout mvpipe_metadata_t meta,
        inout standard_metadata_t standard_metadata) {

    action compute_reg0_index () {
        hash(meta.hash0, HashAlgorithm.crc16, HASH_BASE,
                {hdr.ipv4.srcAddr, 7w11}, 12w2048);
    }

    table hash_index0 {
        actions = {
            compute_reg0_index;	
        }
        default_action = compute_reg0_index();
    }

    action compute_reg1_index () {
        hash(meta.hash1, HashAlgorithm.crc16, HASH_BASE,
                {meta.pushkey0, 5w3}, 12w2048);
    }

    table hash_index1 {
        actions = {
            compute_reg1_index;	
        }
        default_action = compute_reg1_index();
    }

    action compute_reg2_index () {
        hash(meta.hash2, HashAlgorithm.crc16, HASH_BASE,
                {meta.pushkey0, 1w1}, 12w2048);
    }

    table hash_index2 {
        actions = {
            compute_reg2_index;	
        }
        default_action = compute_reg2_index();
    }


    apply {
        //1. calculate the index at array 0
        hash_index0.apply();
        // 1.1 update the total sum
        mvpipe_value_0.read(meta.tmpval, meta.hash0);
        mvpipe_value_0.write(meta.hash0, meta.tmpval+1);
        mvpipe_key_0.read(meta.tmpkey, meta.hash0);	
        if (meta.tmpkey == hdr.ipv4.srcAddr) {
            mvpipe_ind_0.read(meta.tmpval, meta.hash0);
            mvpipe_ind_0.write(meta.hash0, meta.tmpval+1);
            mvpipe_cum_0.read(meta.tmpval, meta.hash0);
            mvpipe_cum_0.write(meta.hash0, meta.tmpval+1); 
        } else {
            mvpipe_ind_0.read(meta.tmpval, meta.hash0);
            if (meta.tmpval == 0) {
                //kick out original candidate HHH
                mvpipe_ind_0.write(meta.hash0, 1);
                mvpipe_key_0.read(meta.pushkey0, meta.hash0);
                mvpipe_cum_0.read(meta.pushval0, meta.hash0);
                mvpipe_key_0.write(meta.hash0, hdr.ipv4.srcAddr); 	
                mvpipe_cum_0.write(meta.hash0, 1);
                meta.flag0=1;
            } else {
                mvpipe_ind_0.write(meta.hash0, meta.tmpval-1);     
            }    
        } 

        if (meta.flag0 == 1) {
            //update array 2
            meta.pushkey0 = meta.pushkey0 & 32w0xFFFFFF00;	
            hash_index1.apply();	
            mvpipe_value_1.read(meta.tmpval, meta.hash1);
            mvpipe_value_1.write(meta.hash1, meta.tmpval+meta.pushval0);
            mvpipe_key_1.read(meta.tmpkey, meta.hash1);	
            if (meta.tmpkey == meta.pushkey0) {
                mvpipe_ind_1.read(meta.tmpval, meta.hash1);
                mvpipe_ind_1.write(meta.hash1, meta.tmpval+meta.pushval0);
                mvpipe_cum_1.read(meta.tmpval, meta.hash1);
                mvpipe_cum_1.write(meta.hash1, meta.tmpval+meta.pushval0); 
            } else {
                mvpipe_ind_1.read(meta.tmpval, meta.hash1);
                if (meta.tmpval < meta.pushval0) {
                    //kick out original candidate HHH
                    mvpipe_ind_1.write(meta.hash1, meta.pushval0-meta.tmpval);
                    mvpipe_key_1.read(meta.pushkey1, meta.hash1);
                    mvpipe_cum_1.read(meta.pushval1, meta.hash1);
                    mvpipe_key_1.write(meta.hash1, meta.pushkey0); 	
                    mvpipe_cum_1.write(meta.hash1, meta.pushval0);
                    meta.flag1=1;
                } else {
                    mvpipe_ind_1.write(meta.hash1, meta.tmpval-meta.pushval0);     
                }   
            }
        }

        if (meta.flag1 == 1) {
            //update array 3 
            meta.pushkey1 = meta.pushkey1 & 32w0xFFFF0000;	
            hash_index2.apply();	
            mvpipe_value_2.read(meta.tmpval, meta.hash2);
            mvpipe_value_2.write(meta.hash2, meta.tmpval+meta.pushval1);

            mvpipe_key_2.read(meta.tmpkey, meta.hash2);	
            if (meta.tmpkey == meta.pushkey1) {
                mvpipe_ind_2.read(meta.tmpval, meta.hash2);
                mvpipe_ind_2.write(meta.hash2, meta.tmpval+meta.pushval1);
                mvpipe_cum_2.read(meta.tmpval, meta.hash2);
                mvpipe_cum_2.write(meta.hash2, meta.tmpval+meta.pushval1); 
            } else {
                mvpipe_ind_2.read(meta.tmpval, meta.hash2);
                if (meta.tmpval < meta.pushval1) {
                    //kick out original candidate HHH
                    mvpipe_ind_2.write(meta.hash2, meta.pushval1-meta.tmpval);
                    mvpipe_key_2.read(meta.pushkey2, meta.hash2);
                    mvpipe_cum_2.read(meta.pushval2, meta.hash2);
                    mvpipe_key_2.write(meta.hash2, meta.pushkey1); 	
                    mvpipe_cum_2.write(meta.hash2, meta.pushval1);
                    meta.flag2=1;
                } else {
                    mvpipe_ind_2.write(meta.hash2, meta.tmpval - meta.pushval1);     
                }
            }
        }

        if (meta.flag2 == 1) {
            //update array 3 
            meta.pushkey2 = meta.pushkey2 & 32w0xFF000000;	
            meta.hash3 = meta.pushkey2;
            mvpipe_value_3.read(meta.tmpval, meta.hash3);
            mvpipe_value_3.write(meta.hash3, meta.tmpval+meta.pushval2);
            mvpipe_ind_3.read(meta.tmpval, meta.hash3);

            mvpipe_key_3.read(meta.tmpkey, meta.hash3);	
            if (meta.tmpkey == meta.pushkey2) {
                mvpipe_ind_3.read(meta.tmpval, meta.hash3);
                mvpipe_ind_3.write(meta.hash3, meta.tmpval+meta.pushval2);
                mvpipe_cum_3.read(meta.tmpval, meta.hash3);
                mvpipe_cum_3.write(meta.hash3, meta.tmpval+meta.pushval2); 
            } else {
                if (meta.tmpval < meta.pushval2) {
                    //kick out original candidate HHH
                    mvpipe_ind_3.write(meta.hash3, meta.pushval2-meta.tmpval);
                    mvpipe_cum_3.read(meta.pushval3, meta.hash3);
                    mvpipe_key_3.write(meta.hash3, meta.pushkey2); 	
                    mvpipe_cum_3.write(meta.hash3, meta.pushval2);
                    meta.flag3=1;
                } else {
                    mvpipe_ind_3.write(meta.hash3, meta.tmpval-meta.pushval2);     
                }    
            }
        }

        if (meta.flag3 == 1) {
            mvpipe_value_4.read(meta.tmpval, 0);
            mvpipe_value_4.write(0, meta.tmpval+meta.pushval3);
        }
    }
}


control MyComputeChecksum(inout headers hdr, inout mvpipe_metadata_t meta) {
    apply { }
}


// Minimal deparser etc.
control MyDeparser(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.ipv4);
    }
}

    V1Switch(
            MyParser(),
            MyVerifyChecksum(),
            MyIngress(),
            MyEgress(),
            MyComputeChecksum(),
            MyDeparser()
            ) main;


