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

void test_basics (){ // test basics of Node, Tree, Client, and Server
    Client c1; 
    c1.prinClient(); 

    c1.tree->nodes[0]->bucket->blocks[0]->data = "Testing 123456"; 

    Server s1; 
    s1.update(c1.tree); 

    s1.printServer(); 
}

void test_Path() { // check getPath function
    int* path = getPath(11); 

    for (int i =0; i <PATHSIZE; i++) {
        cout << path[i] << endl; 
    }
}