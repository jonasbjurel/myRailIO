import sys
from PyQt5.QtWidgets import QApplication
from ui import UI_mainWindow
from topDecoderLogic import topDecoder


if __name__ == "__main__":
    app = QApplication(sys.argv)
    win = UI_mainWindow()  #WE NEED TO pass win
    top = topDecoder(win, demo=True) #WE NEED TO pass top
    win.show()
    sys.exit(app.exec())
