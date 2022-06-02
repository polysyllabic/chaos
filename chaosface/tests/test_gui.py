# Test the gui on its own
import threading
import logging

import chaosface.gui.flexxgui as gui

if __name__ == "__main__":
  logging.basicConfig(filename="test.log", level=logging.DEBUG)
  logging.info("Starting GUI")
  gui.startFlexx()
  flexxThread = threading.Thread(target=gui.startFlexx)
  flexxThread.start()

  logging.info("Stopping")
  gui.stopFlexx()
  flexxThread.join()
