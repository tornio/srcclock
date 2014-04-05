/*
    Class Clog - Used for logging.
    Copyright (C) 2014  Vittorio Tornielli di Crestvolant <vittorio.tornielli@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef CLOG_H
#define CLOG_H

#include <iostream>
#include <fstream>
#include <string>



/**
  * @brief Manage the outputs and logs. It is possible to redirect the output messages (normal and/or errors) to standard output or file.
  *
  * @class Clog
  * @author Vittorio Tornielli di Crestvolant   <vittorio.tornielli@gmail.com>
  * @version 1.0
  * @date 2009-2014
  */

class Clog {
    std::ofstream fout;
    int mode;
    bool errorStream;
    
public:
    Clog();
    Clog(bool ErrorMode);
    Clog(const char* filename, bool ErrorMode = false);

    
    void setErrorStream(bool ErrorMode) { errorStream = ErrorMode; }
    
    bool streamOnFile(const char* filename);
    void streamOnSTDOUT();
    void closeFile();
    
    bool good() const;
    
    Clog& flush();
    
    Clog& operator<<(const std::string& s);
    Clog& operator<<(const char* s);
    Clog& operator<<(char s);
    Clog& operator<<(int s);
    Clog& operator<<(double s);
    Clog& operator<<(long int s);
    Clog& operator<<(long long int s);
    
    virtual ~Clog();
};

#endif // CLOG_H
