from flexx import flx
from chaosface.model.ModelObserver import ModelObserver

from config import model
from views import ActiveView
from model import ModelObserver

class ActiveMods(ModelObserver):
  def init(self):
    self.chaosActiveView = ActiveView(self)
    model.attach(self)

  def modelUpdate(self, message) -> None:
    if message == 'active_modifiers':
      # TODO: Need to refresh the view with a different number of modifiers
      pass

    if message == 'new_modifier':
      self.chaosActiveView.updateMods(model.activeMods)

    if message == 'modifier_times':
      for i in range(model.get_value('active_modifiers')):
        self.chaosActiveView.updateTime(model.activeModTimes[i])

  #@relay.reaction('updateModTimes')
  #def _updateModTimes(self, *events):
  #  for ev in events:
  #    self.chaosActiveView.updateTime(ev.value)
      
  #@relay.reaction('updateActiveMods')
  #def _updateActiveMods(self, *events):
  #  for ev in events:
  #    self.chaosActiveView.updateMods(ev.value)
