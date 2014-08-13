#include <iostream>
#include <fstream>

#include "json.hh"

using namespace std;

int main(int argc, char **argv) {
    
    ifstream file;
    file.open(argv[1]);
    json::Reader reader(file);
    
    json::Writer writer(cout);
    
    json::Value *value;
    while (value = reader.getValue()) {
        writer.putValue(value);
        delete value;
    }

}
