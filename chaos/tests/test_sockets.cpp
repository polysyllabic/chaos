#include <string>
#include <iostream>
#include "ChaosInterface.hpp"
#include "CommandObserver.hpp"

// Test a round-trip (receive and send response)
class TestObserver : public Chaos::CommandObserver {
  Chaos::ChaosInterface interface;

private:
  bool success = false;
  std::string reply{"I heard you"};

  void newCommand(const std::string& command) {
    std::cout << "Received command: " << command << std::endl;
    std::cout << "About to send reply: " << reply << std::endl;
    success = interface.sendMessage(reply);
  }

public:
  TestObserver(const std::string& server_endpoint, const std::string& interface_endpoint) {
    interface.setObserver(this);
    std::cout << "server endpoint: " << server_endpoint << std::endl;
    std::cout << "interface endpoint: " << interface_endpoint << std::endl;
    interface.setupInterface(server_endpoint, interface_endpoint);
  }

  bool gotMessage() { return success; }
};

int main(int argc, char const *argv[])
{
  TestObserver observer("tcp://*:5555", "tcp://192.168.1.8:5556");
  
  while (true) {
  }

  return 0;
}
