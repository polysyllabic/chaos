
from abc import ABC, abstractmethod
import ChaosObserver

class ChaosSubject(ABC):
	"""
	The Subject interface declares a set of methods for managing subscribers.
	"""

	@abstractmethod
	def attach(self, observer: ChaosObserver) -> None:
		"""
		Attach an observer to the subject.
		"""
		pass

	@abstractmethod
	def detach(self, observer: ChaosObserver) -> None:
		"""
		Detach an observer from the subject.
		"""
		pass

	@abstractmethod
	def notify(self, message) -> None:
		"""
		Notify all observers about an event.
		"""
		pass
