#ifndef APPLICATION_CONTROLLER_H
#define APPLICATION_CONTROLLER_H

#include "gui/models/models.h"
#include <QObject>
#include <filesystem>

class ApplicationController : public QObject {
  Q_OBJECT

signals:
  void loadStarted();
  void loadCompleted();
  void loadFailed(const QString &errorMessage);

private:
  ReportModel report_;

public:
  ApplicationController(QObject *parent = nullptr);
  void loadReport(const std::filesystem::path &filePath);
};

#endif