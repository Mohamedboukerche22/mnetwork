#include "mnetwork.h"
#include<thread>
#include<string>
using namespace mnetwork;
using namespace std;

void multihosts(int port1,string port1file,int port2,string port2file){
    thread t1([=]() {hostFile(port1file.c_str(), port1);});
    thread t2([=]() {hostFile(port2file.c_str(), port2);});
    t1.join();
    t2.join();

}


int main()
{
    // hostFile("index.html",8080);  ------> host the file index.html
    // host("<h1>Welcome to mnetwork</h1>",8000); ------> host html script directly
    //multihosts(
    //  8080,"index.html",
    //  8000,"page2.html"
    // );  -- > port 1 , file one , port 2 , file 2 
     // this could host two websites in same time


    return 0;
}

