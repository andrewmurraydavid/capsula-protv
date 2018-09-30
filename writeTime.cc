// basic file operations
#include <iostream>
#include <fstream>
using namespace std;

int main(int argc, char* argv[]) {

  if (argc == 2) {
    std::ofstream myfile("/home/pi/ct/timeFile.txt");
    if (myfile.is_open())
    {
        myfile << argv[1] << endl;
        myfile.flush();
        myfile.close();
        cout << "Wrote to file: " << argv[1] << endl;
    }
    else
    {
        std::cerr << "didn't write" << std::endl;
    }
    return 0;
  }
  return 1;
}
