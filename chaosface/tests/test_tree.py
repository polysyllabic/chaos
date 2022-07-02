from flexx import flx
import asyncio
import logging
logging.basicConfig(level=logging.DEBUG)

class TestTree(flx.Widget):

    def init(self):
      self.mod_items = []

      with flx.VBox():
        flx.Label(style="text-align:left", text='Available Modifiers')
        with flx.TreeWidget(flex=1, max_selected=1, minsize=(300,300)) as self.tree:
          for i in range(4):
            self.mod_items.append(flx.TreeItem(text=f'Modifier {i+1}', checked=False))
        with flx.HBox():
          with flx.VBox(flex=1):
            flx.Label(text='Description:')
            self.mod_description = flx.LineEdit()
          with flx.VBox(flex=1):
            flx.Label(text='Groups:')
            self.mod_groups = flx.LineEdit()
          flx.Widget(flex=1)

    @flx.reaction('tree.children**.checked')
    def on_mod_checked(self, *events):
        for ev in events:
          self.mod_description.set_text('Check of ' + ev.source.text + ' is ' + ev.new_value)

    @flx.reaction('tree.children**.selected')
    def on_mod_selected(self, *events):
        for ev in events:
          self.mod_groups.set_text('Selected: ' + ev.source.text)


#    @flx.reaction('!new_game_data')
#    def _rebuild_mod_tree(self, *events):
#      # Remove old tree items
#      for item in self.mod_items:
#        item.set_parent(None)
#        item.dispose()
#      # Rebuild list
#      self.mod_items = []
#      for mod, data in config.relay.modifier_data.items():
#        self.mod_items.append(flx.TreeItem(text=mod, checked=data['active']))

if __name__ == '__main__':
    flx.App(TestTree).serve('TreeTest')
    flx.create_server(host='0.0.0.0', port=80, loop=asyncio.new_event_loop())  
    flx.start()