/*
    Class Crw - Interface with the Pulseaudio sound server.
    Copyright (C) 2014  Vittorio Tornielli di Crestvolant <vittorio.tornieli@gmail.com>

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


#ifndef CRW_H
#define CRW_H





#include <string>
#include <fstream>
#include <pulse/simple.h>
#include <pulse/error.h>
//#include <pulse/pulseaudio.h>








/**
 * @brief Class that manages the input/output for a sound application. By default it is set for mono play/rec. This class permits the user to write data to a file if needed.
 *
 * @class Crw
 * @author Vittorio Tornielli di Crestvolant   <vittorio.tornielli@gmail.com>
 * @version 1.0
 * @date 2009-2014
 **/

class Crw {
    int		INstate;			// State of the INput stream
    int		OUTstate;			// State of the OUTput stream
    
    std::fstream	fd;			// File descriptor for input or output on file
    pa_simple*	sound_stream;			// Descriptor of the PulseAudio stream
    
    int		sound_error;			// variable that identifies the last error in sound stream

public:
  /**
   * @brief Read some data FROM the input stream that has been previously opened
   *
   * @param buffer Buffer of data to read.
   * @param samples Number of samples to read. Please note that no control is made on the dimesions of the buffer array.
   * @param size Dimension in bytes of each element of the buffer. By default it is set to 1
   * @return <int> number of samples read
   **/
  virtual int readBuffer(void* buffer, int samples, int size = 1);		// read a buffer from the open INput stream
  
  
  /**
   * @brief Writes some data TO the input stream that has been previously opened
   *
   * @param buffer Buffer of data to write.
   * @param samples Number of samples to write. Please note that no control is made on the dimesions of the buffer array.
   * @param size Dimension in bytes of each element of the buffer. By default it is set to 1
   * @return <int> number of samples read
   **/
  virtual int writeBuffer(const void* buffer, int samples, int size = 1);	// write a buffer to an open OUTput stream

  
    /**
     * @brief Opens a PulseAudio simple stream in PLAYBACK mode
     *
     * @param fc Sample frequency
     * @param channels 1 = mono; 2 = stereo
     * @param SampFormat Sample format
     * @param device Name of the PA device
     * @param appName Name of the application
     * @return <bool> returns true if everything is OK
     **/
    virtual bool open_soundStream_output(int fc, int channels, const char* device = 0, pa_sample_format SampFormat = PA_SAMPLE_U8, const char* appName = 0);
    
    
    /**
     * @brief Opens a PulseAudio simple stream in RECORDING mode
     *
     * @param fc Sample frequency
     * @param channels 1 = mono; 2 = stereo
     * @param SampFormat Sample format
     * @param device Name of the PA device
     * @param appName Name of the application
     * @return <bool> returns true if everything is OK
     **/
    virtual bool open_soundStream_input(int fc, int channels, const char* device = 0, pa_sample_format SampFormat = PA_SAMPLE_U8, const char* appName = 0);
    
    
    virtual bool open_file_output(const char* fileNAme);	/**< Open the output stream on file */
    virtual bool open_file_input(const char* fileNAme);		/**< Open the output stream on file */
    
    virtual void close_input_stream();				/**< Close the input stream */
    virtual void close_output_stream();				/**< Close the output stream */
    virtual void close_all();					/**< Close all streams */



    std::string get_sound_error() const { return pa_strerror(sound_error); }	/**< Return a string of the previous pulseaudio error */
    
    
    int get_INstate() const { return INstate; }		/**< Return the value of variable INstate for the input status */
    int get_OUTstate() const { return OUTstate; }	/**< Return the value of variable OUTstate for the output status */
    
    Crw();
    virtual ~Crw() { close_all(); } 
};

#endif // CRW_H
