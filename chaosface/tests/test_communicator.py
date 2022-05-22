from chaosface.communicator import Observer
from chaosface.communicator import ChaosCommunicator
import time

# This is currently only an integration test. Need to add unit tests.

class TestObserver(Observer):
	def updateCommand(self, message ) -> None:
		print("Message: " + str(message))


if __name__ == "__main__":
	subject = ChaosCommunicator()
	subject.start()
	
	testObserver = TestObserver()
	subject.attach(testObserver)
	
	while True:
		subject.sendMessage("Hello Chaos!")
		time.sleep(1)
	
