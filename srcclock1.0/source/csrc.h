/*
    Class Csrc - Part of the SRCclock program.
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


#ifndef CSRC_H
#define CSRC_H


#include <cmath>
#include <ctime>
#include <vector>
#include <string>
#include <cstdlib>
#include <chrono>
#include "crw.h"
#include "clog.h"



using std::vector;
using std::string;



/**
 * @brief Implement and manage all the main functionalities of the SRC signals. Possibility to decode from both file or sound stream as well playing the signal for test purposes.
 *
 * @class Csrc
 * @author Vittorio Tornielli di Crestvolant   <vittorio.tornielli@gmail.com>
 * @version 1.0
 * @date 2009-2014
 */


class Csrc : public Crw {

  const int F0;		// frequency of tone 0
  const int F1;		// frequency of tone 1
  const int Fsync;
  const double Ts;	// time of each symbol
  
  int year, month, day, wday, hour, min, sec;
  int leap_second, change_time;
  bool dst;
  
  int msec;		// milliseconds. Used to compensate the error on the syncronisation due to post processing of the samples
  
  vector<int> src_vector;
  
  int timeout;			// timeout in sec
  int soundChannels;		// mono, stereo
  
  int sample_frequency;
  double decision_threshold;	// decision threshold in dB
  bool adaptive_decision_threshold;
  int  window_length;
  double snr_level;

  int verbose_level;		// verbose level for debugging messages
  
  bool decoded;		// decode status
  bool running;
  bool do_sync;
  
  int error;
  bool streamON;	// the stream has been established
  
  Clog lout, lerr;	// logs for output and error messages
  
  std::chrono::high_resolution_clock::time_point sys_clock;
  
public:
    Csrc();		/**< Default constructor */
    virtual ~Csrc();	/**< Descructor */
    
    /**
     * @brief Open the output stream on the Sound server. The SRC signal will be played there.
     * 
     * @param fc Sampling frequency
     * @param channels Channels: 1 = mono; 2 = stereo. Default: mono
     * @param device Name of the sound device. Leaving it NULL will choose the default sound card. The sound server is the one used as default.
     * @param SampFormat Format of the samples. The SRC class expects float samples in the continues range [-1;1]. Please, don't change this parameter unless sure. On Intel machines the endianness is 'Little Endian'.
     * @param appName Name of the application using the sound server. By default "SRC".
     * @return bool
     */
    bool open_soundStream_output(int fc, int channels = 1, const char* device = NULL, pa_sample_format SampFormat = PA_SAMPLE_FLOAT32LE, const char* appName = "SRC");
    
    
    /**
     * @brief Open the input stream on the Sound server. The SRC signal will be played there.
     * 
     * @param fc Sampling frequency
     * @param channels Channels: 1 = mono; 2 = stereo. Default: mono
     * @param device Name of the sound device. Leaving it NULL will choose the default sound card. The sound server is the one used as default.
     * @param SampFormat Format of the samples. The SRC class expects float samples in the continues range [-1;1]. Please, don't change this parameter unless sure. On Intel machines the endianness is 'Little Endian'.
     * @param appName Name of the application using the sound server. By default "SRC".
     * @return bool
     */
    bool open_soundStream_input(int fc, int channels = 1, const char* device = NULL, pa_sample_format SampFormat = PA_SAMPLE_FLOAT32LE,const char* appName = "SRC");

    /**
     * @brief Open the output stream on File. The default sampling frequency is 8000 Hz on one (mono) channel
     * 
     * @param fileNAme Name of the file
     * @return bool
     */
    bool open_file_output(const char* fileNAme);
    
    
    /**
     * @brief Open the input stream on File. The default sampling frequency is 8000 Hz on one (mono) channel
     * 
     * @param fileNAme Name of the file
     * @return bool
     */    
    bool open_file_input(const char* fileNAme);
    

    /**
     * @brief Open the output stream on file.
     * 
     * @param fileNAme Name of the file
     * @param fc Sampling frequency
     * @param channels Channels: 1 = mono; 2 = stereo.
     * @return bool
     */
    bool open_file_output(const char* fileNAme, int fc, int channels);
    
    
    
    /**
     * @brief Open the input stream on file.
     * 
     * @param fileNAme Name of the file
     * @param fc Sampling frequency
     * @param channels Channels: 1 = mono; 2 = stereo.
     * @return bool
     */
    bool open_file_input(const char* fileNAme, int fc, int channels);
    
    
    
    /**
     * @brief Write raw data on the output stream if it is open. The number of samplers written are returned.
     * 
     * @param buffer Pointer to sampling data
     * @param samples Number of samples
     * @return int number of samples written
     */
    int writeBuffer(const float* buffer, int samples);
    
    
   /**
     * @brief Read raw data on the input stream if it is open. The number of samplers read are returned.
     * 
     * @param buffer Pointer to sampling data
     * @param samples Number of samples
     * @return int number of samples read
     */
    int readBuffer(float* buffer, int samples);
    
    
    
    
    void close_input_stream();		/**< Close input stream */
    void close_output_stream();		/**< Close output stream */
    void close_all();			/**< Close all active streams */


    /**
     * @brief Play the SRC signal on the output stream.
     *
     * @param power Power level of the signal in dB (maximum level is 0 dB)
     * @param initial_delay flag that indicates whether to put a random number of samples on the stream before playing the SRC. Duration is less than 1 second.
     * @param random_theta Flag that indicates wheter to add a random \f$\theta\f$ to the signal
     * @param noise_sigma RMS of the Additive White Gaussian Noise added to the main signal
     */
    void play(double power = -3, bool initial_delay = false, bool random_theta = false, double noise_sigma = 0.0);
    
    bool warnings() const { return (change_time != 7) || (leap_second != 0); }	/**< Return true if there is any warning issued. */
    
    bool operator==(const Csrc& other) const;	/**< Equal operator between objects of class Csrc */
    bool operator<(const Csrc& other) const;	/**< Minor operator between objects of class Csrc */
    Csrc& operator=(const Csrc& other);		/**< Operator = for class Csrc */

    
    vector<int> get_src_vector() const { return src_vector; }	/**< Return the binary vector in the for of a vector<int> */
    int* get_src_vector_int() const;	/**< Return the binary vector in the for of a int* */
    
    
    bool OK() const { return decoded; }	/**< If the decoding was successful is returned TRUE */
    
    bool P1() const;	/**< Check on the parity 1 (first block) */
    bool P2() const;	/**< Check on the parity 2 (first block) */
    bool PA() const;	/**< Check on the parity of the second block */
    
    bool ID1() const;	/**< Return true if the first identification ID1 is valid */
    bool ID2() const;	/**< Return true if the second identification ID2 is valid */
    
    
    int OR() const { return hour; }		/**< Return the hour */
    int MI() const { return min; }		/**< Return the minute */
    bool OE() const { return dst; }		/**< Return the flag of summer time */
    int ME() const { return month; }		/**< Return the month */
    int GM() const { return day; }		/**< Return the day */
    int GS() const { return wday; }		/**< Return the day of the week */
    int AN() const { return year; }		/**< Return the year */
    int SE() const { return change_time; }	/**< Return the warning of change time */
    int SI() const { return leap_second; }	/**< Return the warning of leapsecond */
    

    bool valid_date() const;	/**< check if the format of the date is valid */
    bool check(int bits = 48);	/**< check the first bits of the received sequence */



    void set_today();		/**< The internal variables are set to the current date and time */
    void reset();		/**< Reset all the variables the default and close all the stream */
    void stop();		/**< Any action on streams are stopped. */
    bool streamOK() const { return streamON; }	/**< Return the state of the stream. A 0 is returned for invalid input or output streams. */


    
    int	get_timeout() const { return timeout; }	/**< Return the decoding timeout in seconds */
    void set_timeout(int seconds);		/**< Set the decoding timeout in seconds */



    
    bool decode();		/**< decode the SRC signal. Return TRUE is decoding was successful */

    bool sincronized() const { return decoded && (error == 0); }	/**< Return TRUE if the SRC has been decoded and sincronized */


    /**
     * @brief Set the length in term on time symbols for the Window Decision System
     *
     * The Window Decision System is an algorithm used by the class in order to adapt the value of the decision threshold
     * to the noise level of the channel. The average of the power associated to both signal frequencies is calculated over the last
     * Wlength time symbols and the decision threshold is set snr dB over this level. This way the decision threshold is kept updated
     * and adapted to the channel noise level.
     * The WDS algorithm is used by the Csrc class in order to select the right value of any decision thresholds and it
     * is alternative with respect the static decision. It is assumed here that if the window length is positive then a
     * dynamic evaluation of the channel is required. By the contrary, any zero or negative value of the window length
     * implies the use of the static threshold, whose value can be changed with the function set_decision_threshold().
     *
     * @param Wlength dimension of the window in time symbols. If Wlength <= 0 then no WDS adaptation will be made
     * @param snr value indicating the increasing of the decision threshold in dB with respect the noise average
     * @return length of the window in time symbols
     */
    int setWDS(int Wlength, double snr = 12.0);





    double set_decision_threshold(double dB);		/**< Set the value in dB of the decision threshold */
    inline double get_decision_threshold() const;	/**< Return the value of the decision threshold in dB */



    
    void yes_sync() { do_sync = true; }	/**< Syncronisation ON. In case of play mode, the syncronisation tones are played. In decoding mode, the syncronisation tones are used to syncronize with the minute of the SRC. */
    void no_sync() { do_sync = false; }	/**< Skip syncronisation */
    bool get_sync() const { return do_sync; }	/**< Return the syncronisation mode */




    
    int internalError() const { return error; }	/**< Return the internal error level. */
    


    /**
      * @brief Set the verbose level. Higher is the value and more detailed is the debugging output.
      *
      * @param level Level of the verbose. 1 = normal verbose; 2 = debugging level 1; 3 = debugging level 2; less than 1: no verbose.
      */
    void set_verbose(int level) { verbose_level = level; }
    int get_verbose() const { return verbose_level; }	/**< Return the verbose level */

    
    static bool leapyear(int y);	/**< Return true if the year has 366 days */
    
    
    friend std::ostream& operator<<(std::ostream& os, const Csrc& src);	/**< Output of the binary string of the SRC */
    friend std::istream& operator>>(std::istream& is, Csrc& src);	/**< Input of the binary string of the SRC */
    
    string dateSTD() const;	/**< Return a string of the date in the standard format defined in RFC 2822 */
    string dateISO() const;	/**< Return a string of the date in the standard format ISO 8601 */
    struct tm get_date_tm() const;	/**< Return a tm struct of the date/time information */

    /**
      * @brief Return the string of the SRC date/time with format RFC2822 ora ISO8601
      *
      * @param iso8601 This variable should be TRUE to have the date/time in the format ISO8601, FALSE for the format defined in RFC2822
      */
    string dateSTR(bool iso8601 = false) const;



    void logOnFile(const char* fileName);	/**< The log are redirect on external file */
    void errorLogOnFile(const char* fileName);	/**< Errors log are redirect on external file */
    void logOnSTDOUT();				/**< Set the default STDOUT stream for logs */
    


    /**
      * @brief Set internal information of date and time plus the warning flags.
      *
      * @param MI Minutes [0, 60]
      * @param OR Hours [0, 23]
      * @param GM Day of the month [1, 31]
      * @param GS Day of the week [1, 7] (1 = Monday)
      * @param ME Month [1, 12]
      * @param AN Year  (0 = 2000, 1 = 2001, etc...)
      * @param OE flag of summer time
      * @param SE Warning of change time [0, 7] (7 = no warnings, 6 = change time in 6 days, 5 = change time in 5 days, ...)
      * @param SI Warning of leap second at the end of the month (-1, 0, +1)
      * @return True if the date is correct
      */
    bool set(int MI, int OR, int GM, int GS, int ME, int AN, bool OE = false, int SE = 7, int SI = 0);


    /**
      * @brief Set internal information of date and time through the struct tm. No warning flags for change time or leapsecond are set
      *
      * @param ttmm Is a struct tm with the information related to the date and time to be set.
      * @return True if the date is correct
      */
    bool set(const struct tm& ttmm);


    /**
      * @brief Set internal information of date and time. The date/time are passed through a string and processed by function strptime().
      *
      * @param strDate Pointer to a string of char containing the date
      * @param format Pointer to a string of char containing the format of the date passed in the parameter strDate. Please, refer to the documentation of standard function strptime() to set the correct format string.
      * @return True if the date is correct. If any kind of error is encountered during the process of the date string, the internal variables are set to the current date and time.
      */
    bool set(const char* strDate, const char* format);




    /**
      * @brief Set the warning flags.
      *
      * @param SE Warning of change time [0, 7] (7 = no warnings, 6 = change time in 6 days, 5 = change time in 5 days, ...)
      * @param SI Warning of leap second at the end of the month (-1, 0, +1)
      * @return True if the warnings are correct
      */
    bool setWarnings(int SE, int SI);


    bool setOE(bool newDST);	/**< Function to set the flag of the OE (dst) of the SRC bit-string */


    /**
      * @brief Return the microseconds elapsed since last reading from the input stream.
      *
      * This function is useful to perform fine sincronisation of the system clock. Each time some samples are read
      * from the input stream the internal timing variable is reset and the computational delay can be calculated
      * through this function. Essentially, once the SRC signal has been received the system clock is set to the
      * current date/time decoded plus the milliseconds and microseconds elapsed since last reading. The precision
      * can be of the orther of few microseconds depending on the machine's hardware.
      *
      */
    long microsecDelay() const;

    int getMilliseconds() const { return msec;}	/**< Return the number of milliseconds after last reference second */


    int number_of_RP() const;	/**< Return the expected number of syncronisation tones RP for the current date/time */

    
// private functions:
private:

  double randn(double mu, double sigma);
  int parity(int beg, int end) const;
  void bin_convert(int val, int offset, int length);	// converts from an integer value to bit
  int deconvert(int offset, int length);
  inline void checkSample(float& s);
  inline void stereo_encode(float* p, int k);
  void encode();	// build the src_vector
  
  
  double goertzel(int frequency, const float* data, int samples) const;		// calculates the power associated to a given frequency with the Goertzel algorithm
  int read_buffer(float* b, int total_size, int& bytes2read, int& extra);	// modified function for reading the buffer
  int tuning(int freq, float* buffer, int& extra, int N, int DELTA, int STEP, double& p);
  static void reset_buffer(float* b, int size, float value = 0.0);
  static string itos(int value, int length = 1, int base = 10, char fill = '0', bool force_sign = false);
  void add_minute();
  inline double max(double a, double b) const;
  inline void reset_src_vector();
};

#endif // CSRC_H
