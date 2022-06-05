from abc import ABC, abstractclassmethod, abstractmethod

# Declares an interface for observers that want to be notified when data changes
class DataObserver(ABC):
  @abstractmethod
  def update(self, attribute, new_value) -> None:
    pass