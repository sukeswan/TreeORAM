void test_client(){ // test basic client position map and printing 
    Client c1; 

    c1.position_map.insert(pair<int, int>(1, 10));
    c1.position_map.insert(pair<int, int>(2, 20));
    c1.position_map.insert(pair<int, int>(3, 30));
    c1.position_map.insert(pair<int, int>(4, 40));

    c1.prinClient();
}

void test_block(){ // test creating a block and printing it 

    Block b1(6,7,"Blocky");
    b1.printBlock();

}

void test_bucket_basics(){ // create bucket, fill with blocks, and print
    Block b0(0,0, "Check0");
    Block b1(1,1, "Check1");
    Block b2(2,2, "Check2");
    Block b3(3,3, "Check3");
    int size = 4;
    Bucket bu(size); 

    bu.blocks[0] =  b0; 
    bu.blocks[1] =  b1; 
    bu.blocks[2] =  b2; 
    bu.blocks[3] =  b3; 

    bu.printBucket();
}

void test_node_basics(){
    Block b0(0,0, "Check0");
    Block b1(1,1, "Check1");
    Block b2(2,2, "Check2");
    Block b3(3,3, "Check3");
    int size = 4;
    Bucket bu(size); 

    bu.blocks[0] =  b0; 
    bu.blocks[1] =  b1; 
    bu.blocks[2] =  b2; 
    bu.blocks[3] =  b3; 

    Node n1; 
    n1.level = 0; 
    n1.bucket = bu; 
    n1.printNode(); 

}