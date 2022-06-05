from chaosface.communicator import EngineObserver
from chaosface.communicator import ChaosCommunicator
import time

# This is currently only an integration test. Need to add unit tests.

class TestObserver(EngineObserver):
	def updateCommand(self, message) -> None:
		print("Message: " + str(message))


if __name__ == "__main__":
	subject = ChaosCommunicator()
	subject.start()
	
	testObserver = TestObserver()
	subject.attach(testObserver)
	
	while True:
		subject.sendMessage("game")
		time.sleep(10)
	
