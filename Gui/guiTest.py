import sys
from PyQt5.QtWidgets import QApplication
from ui import UI_mainWindow
from topDecoderLogic import topDecoder


if __name__ == "__main__":
    app = QApplication(sys.argv)
    win = UI_mainWindow()
    win.setTop(topDecoder(win, demo=True))
    win.show()
    sys.exit(app.exec())
