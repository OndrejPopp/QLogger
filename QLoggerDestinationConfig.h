#pragma once

/****************************************************************************************
 ** QLogger is a library to register and print logs into a file.
 **
 ** LinkedIn: www.linkedin.com/in/cescmm/
 ** Web: www.francescmm.com
 **
 ** This library is free software; you can redistribute it and/or
 ** modify it under the terms of the GNU Lesser General Public
 ** License as published by the Free Software Foundation; either
 ** version 2 of the License, or (at your option) any later version.
 **
 ** This library is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 ** Lesser General Public License for more details.
 **
 ** You should have received a copy of the GNU Lesser General Public
 ** License along with this library; if not, write to the Free Software
 ** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 ***************************************************************************************/

#include <QLoggerLevel.h>

#include <QString>

namespace QLogger
{

struct QLoggerDestinationConfig
{
   QString fileDestination {};
   LogLevel level = LogLevel::Warning;
   QString fileFolderDestination {};
   LogMode mode = LogMode::OnlyFile;
   LogFileDisplay fileSuffixIfFull = LogFileDisplay::DateTime;
   LogMessageDisplays messageOptions = LogMessageDisplay::Default;

   bool isValid() const { return !fileDestination.isEmpty(); }
};

}
