#include <string>
#include "ChaosInterface.hpp"
#include "CommandObserver.hpp"

// Test a round-trip (receive and send response)
class TestObserver : public Chaos::CommandObserver {
  Chaos::ChaosInterface interface = Chaos::ChaosInterface();
private:
  bool success = false;

  void newCommand(const std::string& command ) {
    std::cout << "Received command: " << command << std::endl;
    interface.sendMessage("I heard you");
    success = true;
  }

public:
  TestObserver() {
    interface.addObserver(this);
  }
  bool gotMessage() { return success; }
};

int main(int argc, char const *argv[])
{
  TestObserver observer = TestObserver();
  while (! observer.gotMessage()) {
    // wait
  }
  return 0;
}
