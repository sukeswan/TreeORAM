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