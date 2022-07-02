from flexx import flx
import asyncio
import logging
logging.basicConfig(level=logging.DEBUG)

class ChaosRelay(flx.Component):
  vote_options = flx.IntProp(settable=True)
  voting_type = flx.EnumProp(['Proportional', 'Majority', 'DISABLED'], 'Proportional', settable=True)

relay = ChaosRelay()

class TestWidget(flx.Widget):

  def init(self):
    with flx.GroupWidget(flex=0, title='Modifier Voting'):
      with flx.HBox():
        with flx.VBox():
          flx.Label(text="Vote options:")
          flx.Label(text="Voting method:")
        with flx.VBox():
          self.vote_options = flx.LineEdit(text=str(3))
          self.voting_type = flx.ComboBox(options=['Proportional','Majority','DISABLED'], selected_key='Proportional')
        self.save_button = flx.Button(text='Save')

@flx.reaction('save_button.pointer_click')
def _save_button_clicked(self, *events):
  need_save = False
  if (relay.vote_options != int(self.vote_options.text)):
    relay.set_vote_options(int(self.vote_options.text))
    need_save = True
  if (relay.voting_type != self.voting_type.text):
    relay.set_voting_type(self.voting_type.text)
    need_save = True
  if need_save:
    pass

if __name__ == '__main__':
    flx.App(TestWidget).serve('Test')
    flx.create_server(host='0.0.0.0', port=80, loop=asyncio.new_event_loop())  
    flx.start()