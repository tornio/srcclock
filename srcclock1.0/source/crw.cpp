/*
    Class Crw - Implementation
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


#include "crw.h"



Crw::Crw()
: fd()
{
  INstate = 0;
  OUTstate = 0;
//  fd = -1;
  sound_stream = NULL;
  sound_error = 0;
  

}


bool Crw::open_file_input(const char* fileNAme)
{
  bool correct;
  
  if (INstate == 0) {
    if(OUTstate == 1) close_output_stream();
    //fd = open(fileNAme, O_RDONLY);
    fd.open(fileNAme, std::ios_base::binary | std::ios_base::in);
    if (fd.good()) {
      correct = true;
      INstate = 1;		// 1 => file stream
    }
    else {
      correct = false;
      INstate = 0;
    }
  }
  else correct = false;
  
  return correct;
}


bool Crw::open_file_output(const char* fileNAme)
{
  bool correct;
  
  if (OUTstate == 0) {
    if(INstate == 1) close_input_stream();
    //fd = open(fileNAme, O_CREAT | O_WRONLY | O_TRUNC, 00644);
    fd.open(fileNAme, std::ios_base::binary | std::ios_base::trunc | std::ios_base::out);
    if (fd.good()) {
      correct = true;
      OUTstate = 1;		// 1 => file stream
    }
    else {
      correct = false;
      OUTstate = 0;
    }
  }
  else correct = false;
  
  return correct;
}


bool Crw::open_soundStream_output(int fc, int channels, const char* device, pa_sample_format SampFormat, const char* appName)
{
  pa_sample_spec ss;		// stream properties
  
  ss.format = SampFormat;
  ss.channels = channels;
  ss.rate = fc;			// sampling frequency

  bool correct;
  
  if(OUTstate == 0) {
    if(INstate == 2) close_input_stream();
    sound_stream = pa_simple_new(NULL,               // Use the default server.
                   appName,            // Our application's name.
                   PA_STREAM_PLAYBACK,
                   device,             // device.
                   "Clock",            // Description of our stream.
                   &ss,                // Our sample format.
                   NULL,               // Use default channel map
                   NULL,               // Use default buffering attributes.
                   &sound_error        // error code.
                   );

    if (sound_stream) {
      correct = true;
      OUTstate = 2;		// 2 => sound stream
    }
    else {
      correct = false;
      OUTstate = 0;
    }
  }
  else correct = false;

  return correct;
}



bool Crw::open_soundStream_input(int fc, int channels, const char* device, pa_sample_format SampFormat, const char* appName)
{
  pa_sample_spec ss;		// stream properties
  
  ss.format = SampFormat;
  ss.channels = channels;
  ss.rate = fc;			// sampling frequency

  bool correct;
  
  if(INstate == 0) {
    if(OUTstate == 2) close_output_stream();
    sound_stream = pa_simple_new(NULL,               // Use the default server.
                   appName,            // Our application's name.
                   PA_STREAM_RECORD,
                   device,             // device.
                   "Clock",            // Description of our stream.
                   &ss,                // Our sample format.
                   NULL,               // Use default channel map
                   NULL,               // Use default buffering attributes.
                   &sound_error        // Ignore error code.
                   );

    if (sound_stream) {
      correct = true;
      INstate = 2;		// 2 => sound stream
    }
    else {
      correct = false;
      INstate = 0;
    }
  }
  else correct = false;

  return correct;
}





int Crw::readBuffer(void* buffer, int samples, int size)
{
  int samples_read = 0;
  char *b;

  if(samples > 0) {
    switch(INstate) {
      case 1:	b = static_cast<char*>(buffer);
		fd.read(b, samples*size);
		samples_read = fd.gcount()/size;
		if(samples_read == 0) samples_read = -1;
		break;
      case 2:	if(pa_simple_read(sound_stream, buffer, samples*size, &sound_error) >= 0)
		  samples_read = samples;
		else
		  samples_read = -1;
		break;
      default:	samples_read = -1;
    }
  }
  
  return samples_read;
}

int Crw::writeBuffer(const void* buffer, int samples, int size)
{
  int samples_written = 0;
  const char *b;
  
  if(samples > 0) {
    switch(OUTstate) {
      case 1:	b = static_cast<const char*>(buffer);
		fd.write(b, samples*size);
		samples_written = fd.gcount()/size;
		break;
      case 2:	if(pa_simple_write(sound_stream, buffer, samples*size, &sound_error) >= 0)
		  samples_written = samples;
		else
		  samples_written = 0;
		break;
      default:	samples_written = 0;
    }
  }
  
  return samples_written;
}

void Crw::close_input_stream()
{
  switch(INstate) {
    case 1:	fd.close();
		break;
    case 2:	pa_simple_flush(sound_stream, &sound_error);
		pa_simple_free(sound_stream);
		break;
  }
  
  INstate = 0;
}

void Crw::close_output_stream()
{
  switch(OUTstate) {
    case 1:	fd.close();
		break;
    case 2:	pa_simple_drain(sound_stream, &sound_error);
		pa_simple_free(sound_stream);
		break;
  }
  
  OUTstate = 0;
}


void Crw::close_all()
{
  close_output_stream();
  close_input_stream();
}

