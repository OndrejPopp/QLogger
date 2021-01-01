#include "QLoggerWriter.h"

#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QDebug>

namespace
{
/**
 * @brief Converts the given level in a QString.
 * @param level The log level in LogLevel format.
 * @return The string with the name of the log level.
 */
QString levelToText(const QLogger::LogLevel &level)
{
   switch (level)
   {
      case QLogger::LogLevel::Trace:
         return "Trace";
      case QLogger::LogLevel::Debug:
         return "Debug";
      case QLogger::LogLevel::Info:
         return "Info";
      case QLogger::LogLevel::Warning:
         return "Warning";
      case QLogger::LogLevel::Error:
         return "Error";
      case QLogger::LogLevel::Fatal:
         return "Fatal";
   }

   return QString();
}
}

namespace QLogger
{

QLoggerWriter::QLoggerWriter(const QLoggerDestinationConfig &config)
   : mConfig(config)
{
   mConfig.fileFolderDestination
       = (mConfig.fileFolderDestination.isEmpty() ? QDir::currentPath() : mConfig.fileFolderDestination) + "/logs/";

   mConfig.fileDestination = mConfig.fileFolderDestination + mConfig.fileDestination;

   QDir dir(mConfig.fileFolderDestination);
   if (mConfig.fileDestination.isEmpty())
   {
      mConfig.fileDestination = dir.filePath(QString::fromLatin1("%1.log").arg(
          QDateTime::currentDateTime().date().toString(QString::fromLatin1("yyyy-MM-dd"))));
   }
   else if (!mConfig.fileDestination.contains(QLatin1Char('.')))
      mConfig.fileDestination.append(QString::fromLatin1(".log"));

   if (mConfig.mode == LogMode::Full || mConfig.mode == LogMode::OnlyFile)
      dir.mkpath(QStringLiteral("."));
}

void QLoggerWriter::setLogMode(LogMode mode)
{
   mConfig.mode = mode;

   if (mConfig.mode == LogMode::Full || mConfig.mode == LogMode::OnlyFile)
   {
      QDir dir(mConfig.fileFolderDestination);
      dir.mkpath(QStringLiteral("."));
   }

   if (mode != LogMode::Disabled && !isRunning())
      start();
}

QString QLoggerWriter::renameFileIfFull()
{
   QFile file(mConfig.fileDestination);

   // Rename file if it's full
   if (file.size() >= mMaxFileSize)
   {
      QString newName;

      const auto fileDestination = mConfig.fileDestination.left(mConfig.fileDestination.lastIndexOf('.'));
      const auto fileExtension = mConfig.fileDestination.mid(mConfig.fileDestination.lastIndexOf('.') + 1);

      if (mConfig.fileSuffixIfFull == LogFileDisplay::DateTime)
      {
         newName
             = QString("%1_%2.%3")
                   .arg(fileDestination, QDateTime::currentDateTime().toString("dd_MM_yy__hh_mm_ss"), fileExtension);
      }
      else
         newName = generateDuplicateFilename(fileDestination, fileExtension);

      const auto renamed = file.rename(mConfig.fileDestination, newName);

      return renamed ? newName : QString();
   }

   return QString();
}

QString QLoggerWriter::generateDuplicateFilename(const QString &fileDestination, const QString &fileExtension,
                                                 int fileSuffixNumber)
{
   QString path(fileDestination);
   if (fileSuffixNumber > 1)
      path = QString("%1(%2).%3").arg(fileDestination, QString::number(fileSuffixNumber), fileExtension);
   else
      path.append(QString(".%1").arg(fileExtension));

   // A name already exists, increment the number and check again
   if (QFileInfo::exists(path))
      return generateDuplicateFilename(fileDestination, fileExtension, fileSuffixNumber + 1);

   // No file exists at the given location, so no need to continue
   return path;
}

void QLoggerWriter::write(const EnqueuedMessage &message)
{
   // Write data to console
   if (mConfig.mode == LogMode::OnlyConsole || mConfig.mode == LogMode::Full)
      qInfo() << message.message;

   if (mConfig.mode == LogMode::OnlyConsole)
      return;

   // Write data to file
   QFile file(mConfig.fileDestination);

   const auto prevFilename = renameFileIfFull();

   if (file.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Append))
   {
      QTextStream out(&file);

      if (!prevFilename.isEmpty())
         out << QString("%1 - Previous log %2\n").arg(message.threadId, prevFilename);

      out << message.message;

      file.close();
   }
}

void QLoggerWriter::enqueue(const QDateTime &date, const QString &threadId, const QString &module, LogLevel level,
                            const QString &function, const QString &fileName, int line, const QString &message)
{
   QMutexLocker locker(&mutex);

   if (mConfig.mode == LogMode::Disabled)
      return;

   QString fileLine;
   if (mConfig.messageOptions.testFlag(LogMessageDisplay::File)
       && mConfig.messageOptions.testFlag(LogMessageDisplay::Line) && !fileName.isEmpty() && line > 0
       && mConfig.level <= LogLevel::Debug)
   {
      fileLine = QString("{%1:%2}").arg(fileName, QString::number(line));
   }
   else if (mConfig.messageOptions.testFlag(LogMessageDisplay::File)
            && mConfig.messageOptions.testFlag(LogMessageDisplay::Function) && !fileName.isEmpty()
            && !function.isEmpty() && mConfig.level <= LogLevel::Debug)
   {
      fileLine = QString("{%1}{%2}").arg(fileName, function);
   }

   QString text;
   if (mConfig.messageOptions.testFlag(LogMessageDisplay::Default))
   {
      text
          = QString("[%1][%2][%3][%4]%5 %6")
                .arg(levelToText(level), module, date.toString("dd-MM-yyyy hh:mm:ss.zzz"), threadId, fileLine, message);
   }
   else
   {
      if (mConfig.messageOptions.testFlag(LogMessageDisplay::LogLevel))
         text.append(QString("[%1]").arg(levelToText(level)));

      if (mConfig.messageOptions.testFlag(LogMessageDisplay::ModuleName))
         text.append(QString("[%1]").arg(module));

      if (mConfig.messageOptions.testFlag(LogMessageDisplay::DateTime))
         text.append(QString("[%1]").arg(date.toString("dd-MM-yyyy hh:mm:ss.zzz")));

      if (mConfig.messageOptions.testFlag(LogMessageDisplay::ThreadId))
         text.append(QString("[%1]").arg(threadId));

      if (!fileLine.isEmpty())
      {
         if (fileLine.startsWith(QChar::Space))
            fileLine = fileLine.right(1);

         text.append(fileLine);
      }
      if (mConfig.messageOptions.testFlag(LogMessageDisplay::Message))
      {
         if (text.isEmpty() || text.endsWith(QChar::Space))
            text.append(QString("%1").arg(message));
         else
            text.append(QString(" %1").arg(message));
      }
   }

   text.append(QString::fromLatin1("\n"));

   messages.append({ threadId, text });

   if (!mIsStop)
      mQueueNotEmpty.wakeAll();
}

void QLoggerWriter::run()
{
   if (!mQuit)
   {
      QMutexLocker locker(&mutex);
      mQueueNotEmpty.wait(&mutex);
   }

   while (!mQuit)
   {
      decltype(messages) copy;

      {
         QMutexLocker locker(&mutex);
         std::swap(copy, messages);
      }

      for (const auto &msg : qAsConst(copy))
         write(msg);

      copy.clear();

      if (!mQuit)
      {
         QMutexLocker locker(&mutex);
         mQueueNotEmpty.wait(&mutex);
      }
   }
}

void QLoggerWriter::closeDestination()
{
   QMutexLocker locker(&mutex);
   mQuit = true;
   mQueueNotEmpty.wakeAll();
}

}
