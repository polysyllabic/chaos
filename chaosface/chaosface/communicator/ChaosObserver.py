from abc import ABC, abstractmethod

class ChaosObserver(ABC):
	"""
	The ChaosObserver interface declares the update method used by subjects.
  We use it to listen to events from the chaos engine
	"""
	
	@abstractmethod
	def update(self, message ) -> None:
		"""
		Receive update from subject
		"""
		pass

