#include <QAction>
#include <QApplication>
#include <QFile>
#include <QFileDialog>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QPlainTextEdit>
#include <QTextStream>

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  QMainWindow window;
  QPlainTextEdit *editor = new QPlainTextEdit();
  editor->setReadOnly(true);

  window.setCentralWidget(editor);
  QMenuBar *menuBar = window.menuBar();

  QMenu *fileMenu = menuBar->addMenu("File");

  QAction *openAction = fileMenu->addAction("Open");

  QAction *quitAction = fileMenu->addAction("Quit");

  QObject::connect(openAction, &QAction::triggered, [&]() {
    QString fileName = QFileDialog::getOpenFileName(
        &window, "Open File", "", "JSON/XML Files (*.json *.xml)");

    if (fileName.isEmpty())
      return;

    QFile file(fileName);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
      return;

    QTextStream in(&file);

    editor->setPlainText(in.readAll());

    file.close();
  });

  QObject::connect(quitAction, &QAction::triggered, &app, &QApplication::quit);

  window.resize(800, 600);
  window.show();

  return app.exec();
}