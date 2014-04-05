/*
    Class Clog - Implementation.
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


#include "clog.h"

Clog::Clog()
: fout()
{
  mode = 1;	// normal stream
  errorStream = false;
}

Clog::Clog(bool ErrorMode)
: fout()
{
  mode = 1;
  errorStream = ErrorMode;
}

Clog::Clog(const char* filename, bool ErrorMode)
: fout(filename, std::ios_base::out)
{
  if(fout) mode = 2;
  else mode = 1;
  
  errorStream = ErrorMode;
}

bool Clog::streamOnFile(const char* filename)
{
  if(mode == 2) fout.close();
  
  fout.open(filename, std::ios_base::out);
  if(fout) mode = 2;
  else mode = 1;
  
  return mode == 2;
}

void Clog::streamOnSTDOUT()
{
  if(mode == 2) {
    fout.close();
    mode = 1;
  }
}


bool Clog::good() const
{
  bool stat;
  
  switch(mode) {
    case 1: if(errorStream) stat = std::cerr.good();
	    else stat = std::cout.good();
	    break;
    case 2: stat = fout.good();
	    break;
    default: stat = false;
  }
  
  return stat;
}


Clog& Clog::operator<<(const std::string& s)
{
  switch(mode) {
    case 1: if(errorStream) std::cerr <<s;
	    else std::cout <<s;
	    break;
    case 2: fout <<s;
	    break;
  }
  
  return *this;
}

Clog& Clog::operator<<(const char* s)
{
  switch(mode) {
    case 1: if(errorStream) std::cerr <<s;
	    else std::cout <<s;
	    break;
    case 2: fout <<s;
	    break;
  }
  
  return *this;
}

Clog& Clog::operator<<(char s)
{
  switch(mode) {
    case 1: if(errorStream) std::cerr <<s;
	    else std::cout <<s;
	    break;
    case 2: fout <<s;
	    break;
  }
  
  return *this;
}

Clog& Clog::operator<<(int s)
{
  switch(mode) {
    case 1: if(errorStream) std::cerr <<s;
	    else std::cout <<s;
	    break;
    case 2: fout <<s;
	    break;
  }
  
  return *this;
}

Clog& Clog::operator<<(long int s)
{
  switch(mode) {
    case 1: if(errorStream) std::cerr <<s;
	    else std::cout <<s;
	    break;
    case 2: fout <<s;
	    break;
  }
  
  return *this;
}


Clog& Clog::operator<<(long long int s)
{
  switch(mode) {
    case 1: if(errorStream) std::cerr <<s;
	    else std::cout <<s;
	    break;
    case 2: fout <<s;
	    break;
  }
  
  return *this;
}

Clog& Clog::operator<<(double s)
{
  switch(mode) {
    case 1: if(errorStream) std::cerr <<s;
	    else std::cout <<s;
	    break;
    case 2: fout <<s;
	    break;
  }
  
  return *this;
}

void Clog::closeFile()
{
  if(mode == 2)
    fout.close();
  mode = 1;
}


Clog::~Clog()
{
  if(mode == 2)
    fout.close();
}

Clog& Clog::flush()
{
  if(mode == 1) {
    if(errorStream) std::cerr.flush();
    else  std::cout.flush();
  }
  else  fout.flush();

  
  return *this;
}

