#include <string>
#include "ChaosInterface.hpp"
#include "CommandObserver.hpp"

class TestObserver : public Chaos::CommandObserver {
  Chaos::ChaosInterface interface = Chaos::ChaosInterface();
private:
  bool success = false;

  void newCommand(const std::string& command ) {
    std::cout << "Received command: " << command << std::endl;
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
