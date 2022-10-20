//
// Created by bytedance on 9.6.21.
//
//#include "net/KendyNet.h"
#include "tools/CommandLineTool.h"
#include "tools/argparse.hpp"
#include <string>
#include <iostream>
#include <cstring>
#include <sstream>


template <class Type>
Type stringToNum(const std::string& str)
{
    std::istringstream iss(str);
    Type num;
    iss >> num;
    return num;
}

int main(int argc,const char **argv) {



    ArgumentParser parser;

    parser.addArgument("-i", "--input_file", 1, true);
    parser.addArgument("-r", "--resource_dir", 1, true);
    parser.addArgument("-m", "--mode", 1, true);
    parser.addArgument("-h","--host",1,true );
    parser.addArgument("-p","--port",1,true);

    parser.parse(argc, argv);

    //string input_file = parser.retrieve<string>("input_file");
    string resource_dir = parser.retrieve<string>("resource_dir");
    if(resource_dir.empty()){
        resource_dir = "./resources";
    }
    string mode = parser.retrieve<string>("mode");
    if(mode.empty()){mode = "holdem";}
    if(mode != "holdem" && mode != "shortdeck")
        throw runtime_error(fmt::format("mode {} error, not in ['holdem','shortdeck']",mode));


    string host = parser.retrieve<string>("host");
    if(host.empty()) {
        host = "127.0.0.1";
    }

    int port = 8080;
    string portStr = parser.retrieve<string>("port");
    if(!portStr.empty()) {
        port = stringToNum<int>(portStr);
    }

    CommandLineTool clt = CommandLineTool(mode,resource_dir);
    cout << "startNet" << endl;
    clt.startNet(host,port);


    /*if(input_file.empty()) {
        CommandLineTool clt = CommandLineTool(mode,resource_dir);
        clt.startWorking();
    }else{
        cout << "EXEC FROM FILE" << endl;
        CommandLineTool clt = CommandLineTool(mode,resource_dir);
        clt.execFromFile(input_file);
    }*/
}