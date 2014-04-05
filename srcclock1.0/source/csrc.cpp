/*
    Class Csrc - Implementation.
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


#include "csrc.h"

Csrc::Csrc()
: Crw(), F0(2000), F1(2500), Fsync(1000), Ts(0.030), src_vector(48, -1), lout(), lerr(true)
{
  /* initialize random seed: */
  srand ( time(NULL) );
  
  sample_frequency = 8000;	// default value for sample frequency
  soundChannels = 1;
  
  verbose_level = 1;		// level of debug messages
  
  change_time = 7;		// no changes
  leap_second = 0;
  decoded = false;
  decision_threshold = pow(10, (-35.0/10.0));
  running = false;
  error = -1;
  adaptive_decision_threshold = true;
  do_sync = true;
  timeout = 600;
  streamON = false;
  window_length = 50;
  snr_level = 16.0;

  sys_clock = std::chrono::high_resolution_clock::now();
  msec = 0;
  set_today();
}

Csrc::~Csrc()
{
  running = false;
  streamON = false;
}

bool Csrc::operator==(const Csrc& other) const
{
  return (other.sec == sec) && (other.min == min) && (other.hour == hour) && (other.day == day) &&
	    (other.month == month) && (other.year == year) && (other.dst == dst) &&
	    (other.wday == wday) && (other.change_time == change_time) &&
	    (other.leap_second == leap_second) && (other.msec == msec);
}

bool Csrc::operator<(const Csrc& other) const
{
  bool s;
  
  if(year == other.year)
    if(month == other.month)
      if(day == other.day)
	if(hour == other.hour)
	  if(min == other.min)
	    if(sec == other.sec)
	      s = msec < other.msec;
	    else s = sec < other.sec;
	  else s = min < other.min;
	else s = hour < other.hour;
      else s = day < other.day;
    else s = month < other.month;
  else s = year < other.year;
  
  return s;
}


Csrc& Csrc::operator=(const Csrc& other)
{
  if(this != &other) {
    src_vector = other.src_vector;
    decoded = other.decoded;
    year = other.year;
    month = other.month;
    day = other.day;
    wday = other.wday;
    hour = other.hour;
    min = other.min;
    sec = other.sec;
    leap_second = other.leap_second;
    change_time = other.change_time;
    dst = other.dst;
    timeout = other.timeout;
    sample_frequency = other.sample_frequency;
    verbose_level = other.verbose_level;
    decision_threshold = other.decision_threshold;
    adaptive_decision_threshold = other.adaptive_decision_threshold;
    do_sync = other.do_sync;

    error = other.error;
    msec = other.msec;
    sys_clock = other.sys_clock;
    window_length = other.window_length;
    snr_level = other.snr_level;
    soundChannels = other.soundChannels;
  }

  return *this;
}


bool Csrc::open_soundStream_input(int fc, int channels, const char* device, pa_sample_format SampFormat, const char* appName)
{
  streamON = Crw::open_soundStream_input(fc, channels, device, SampFormat, appName);
  if(streamON) {
    sample_frequency = fc;
    soundChannels = channels;
  }
  
  return streamON;
}

bool Csrc::open_soundStream_output(int fc, int channels, const char* device, pa_sample_format SampFormat, const char* appName)
{
  streamON = Crw::open_soundStream_output(fc, channels, device, SampFormat, appName);
  if(streamON) {
    sample_frequency = fc;
    soundChannels = channels;
  }
  
  return streamON;
}


bool Csrc::open_file_input(const char* fileNAme)
{
  sample_frequency = 8000;
  soundChannels = 1;
  streamON = Crw::open_file_input(fileNAme);
  
  return streamON;
}


bool Csrc::open_file_output(const char* fileNAme)
{
  sample_frequency = 8000;
  soundChannels = 1;
  streamON = Crw::open_file_output(fileNAme);
  
  return streamON;
}

bool Csrc::open_file_input(const char* fileNAme, int fc, int channels)
{ 
  streamON = Crw::open_file_input(fileNAme);
  if(streamON) {
    sample_frequency = fc;
    soundChannels = channels;
  }
  return streamON;
}

bool Csrc::open_file_output(const char* fileNAme, int fc, int channels)
{
  streamON = Crw::open_file_output(fileNAme);
  if(streamON) {
    sample_frequency = fc;
    soundChannels = channels;
  }
  return streamON;
}

int Csrc::readBuffer(float* buffer, int samples)
{
  int r = 0;

  if(!running) return 0;

  if(samples >= 0) {
    reset_buffer(buffer, samples*soundChannels);	// clear the buffer only for the number of samples to read
    r = Crw::readBuffer(buffer, samples*soundChannels, sizeof(float));
    sys_clock = std::chrono::high_resolution_clock::now();		// start counting the system ticks
  }


  if((soundChannels == 2) && (r > 0)) {
    int i;
    r /= 2;			// r is the number of samples (non stereo)
    for(i = 0; i < r; i++) {
      buffer[i] = (buffer[i*2] + buffer[i*2 + 1])/2.0;
    }

    for(i = r; i < 2*r; i++)	// put the unused samples to zero to prevent errors
      buffer[i] = 0.0;
  }
  
  return r;		// returns the number of samples read
}

int Csrc::writeBuffer(const float* buffer, int samples)
{
  int s = 0;

  if(running) s = Crw::writeBuffer(buffer, samples*soundChannels, sizeof(float));

  return s;
}



int Csrc::parity(int beg, int end) const
{
  int acc = 0;

  for(int i = beg; i <= end; i++) {
    acc += src_vector[i];
  }

  return !(acc %  2);
}



void Csrc::set_today()
{
  struct tm ttmm;
  time_t tt = time('\0');
  localtime_r(&tt, &ttmm);
//               struct tm {
//                   int tm_sec;         /* seconds */
//                   int tm_min;         /* minutes */
//                   int tm_hour;        /* hours */
//                   int tm_mday;        /* day of the month */
//                   int tm_mon;         /* month */
//                   int tm_year;        /* year */
//                   int tm_wday;        /* day of the week */
//                   int tm_yday;        /* day in the year */
//                   int tm_isdst;       /* daylight saving time */
//               };
  min = ttmm.tm_min;
  hour = ttmm.tm_hour;
  day = ttmm.tm_mday;
  wday = ttmm.tm_wday;
  if(wday == 0) wday = 7;
  month = ttmm.tm_mon + 1;
  year = ttmm.tm_year + 1900;
  sec = 53;
  
  if(ttmm.tm_isdst) dst = true;
  else dst = false;

  change_time = 7;
  leap_second = 0;
  
  encode();
  
  decoded = false;
  msec = 0;
}

tm Csrc::get_date_tm() const
{
  struct tm ttmm;

  ttmm.tm_sec  = sec;
  ttmm.tm_min  = min;
  ttmm.tm_hour = hour;
  ttmm.tm_mday = day;
  if(wday != 7) ttmm.tm_wday = wday;
  else ttmm.tm_wday = 0;
  ttmm.tm_mon  = month - 1;
  ttmm.tm_year = year - 1900;
  ttmm.tm_isdst = int(dst);
  
  return ttmm;
}



double Csrc::randn(double mu, double sigma)
{
  static bool deviateAvailable=false; // flag
  static float storedDeviate;         // deviate from previous calculation
  double polar, rsquared, var1, var2;

  if(sigma == 0.0) {
    deviateAvailable = false;
    return mu;
  }

  if (!deviateAvailable) {
    do {
      var1=2.0*( double(rand())/double(RAND_MAX) ) - 1.0;
      var2=2.0*( double(rand())/double(RAND_MAX) ) - 1.0;
      rsquared=var1*var1+var2*var2;
  } while ( rsquared>=1.0 || rsquared == 0.0);

  // calculate polar tranformation for each deviate
  polar=sqrt(-2.0*log(rsquared)/rsquared);

  // store first deviate and set flag
  storedDeviate=var1*polar;
  deviateAvailable=true;

  // return second deviate
  return var2*polar*sigma + mu;
  }
  
  // If a deviate is available from a previous call to this function, it is
  // returned, and the flag is set to false.
  else {
    deviateAvailable=false;
    return storedDeviate*sigma + mu;
  }
}



bool Csrc::P1() const
{
  return src_vector[16] == parity(0, 15);
}

bool Csrc::P2() const
{
  return src_vector[31] == parity(17, 30); 
}

bool Csrc::PA() const
{
   return src_vector[47] == parity(32, 46);
}


bool Csrc::ID1() const
{
  return (src_vector[0] == 0) && (src_vector[1] == 1);
}

bool Csrc::ID2() const
{
  return (src_vector[32] == 1) && (src_vector[33] == 0);
}


void Csrc::reset()
{
  Crw::close_all();
  reset_src_vector();
  decoded = false;
  decision_threshold = pow(10, (-35.0/10.0));		// power decision threshold
  sample_frequency = 8000;
  verbose_level = 1;
  running = false;
  error = -1;
  adaptive_decision_threshold = true;
  set_today();
  do_sync = true;
  timeout = 600;
  sys_clock = std::chrono::high_resolution_clock::now();
  msec = 0;
  window_length = 50;
  snr_level = 16.0;
}

void Csrc::stop()
{
  running = false;
  //decoded = false;
  //close_all();
}



void Csrc::bin_convert(int val, int offset, int length)
{
  int i = 0;
  int k;

  while(length) {
    switch(length) {
      case 1: k = val;
              break;
      case 2: k = val / 2;
              val -= 2*k;
              break;
      case 3: k = val / 4;
              val -= 4*k;
              break;
      case 4: k = val / 8;
              val -= 8*k;
              break;
      case 5: k = val / 10;
              val -= 10*k;
              break;
      case 6: k = val / 20;
              val -= 20*k;
              break;
      case 7: k = val / 40;
              val -= 40*k;
              break;
      case 8: k = val / 80;
              val -= 80*k;
              break;
      default: if(verbose_level >= 2) lerr <<"EE: bin_convert(): Length error!\n";
               return;
    }
    src_vector[offset + i] = k;
    length--;
    i++;
  }
}



void Csrc::play(double power, bool initial_delay, bool random_theta, double noise_sigma)
{
  if(!streamON) {
    error = -2;
    return;
  }

  const int N = sample_frequency*Ts;	// number of samples per tone (bit)
  float theta = 0.0;
  float* tone_buffer = new float[sample_frequency*soundChannels];
  float amplitude;		// amplitude of the sinusoidal wave
  int samples, k;
  int freq;
  
  encode();
  
  if(verbose_level >= 3) {
    lout <<"Play(): src_vector = ";
    for(int i = 0; i < 48; i++) lout <<src_vector[i];
    lout <<'\n';
  }

//////////////////////////////////////////////////////////////////////
// the src_vector has just been filled. Now is the moment to play it!!
//////////////////////////////////////////////////////////////////////

  running = true;
  
  if(random_theta) {	// set a random theta!
    int v = rand() % 360;
    theta = v/180.0*M_PI;
    if(verbose_level >= 1) lout <<"Random Theta: " <<v <<"° (" <<theta <<" rad)\n";
  }

  // delay before starting trasmission
  if(initial_delay) {
    int c;
    c = rand() % sample_frequency;  // delay < 1 second
    if(verbose_level >= 1) lout <<"Initial delay: " <<c <<" samples. (" <<float(c/float(sample_frequency)) 
				<<" secs)\n";
    
    for(int i = 0; i < c; i++) {
      tone_buffer[i*soundChannels] = randn(0, noise_sigma);
      checkSample(tone_buffer[i*soundChannels]);
      stereo_encode(tone_buffer, i*soundChannels);
      if(verbose_level >= 5)
	lout <<"Random value: " <<tone_buffer[i*soundChannels] <<'\n';
    }
    Csrc::writeBuffer(tone_buffer, c);
  }
  
  if(power > 0) power *= -1;
  amplitude = pow(10, (power/20.0));
   
  for(int i = 0; i < 48; i++) {
    if(i == 32) {
      samples = 0.04*sample_frequency;
      for(k = 0; k < samples; k++) {
	tone_buffer[k*soundChannels] = randn(0, noise_sigma);
	checkSample(tone_buffer[k*soundChannels]);
	stereo_encode(tone_buffer, k*soundChannels);
      }
      Csrc::writeBuffer(tone_buffer, samples);
    }


    if(src_vector[i] == 0) freq = F0;		// chooses the right frequency
    else freq = F1;
      
    for(k = 0; k < N; k++) {
      tone_buffer[k*soundChannels] = amplitude*cos(2.0*M_PI*freq/sample_frequency*k + theta) + randn(0, noise_sigma);
      checkSample(tone_buffer[k*soundChannels]);
      stereo_encode(tone_buffer, k*soundChannels);
    }
    Csrc::writeBuffer(tone_buffer, N);

  }	// end of bit transmission
  

// plays the SYNC beeps

  if(do_sync) {
    int ticks = number_of_RP();

    samples = 0.52*sample_frequency;
    for(k = 0; k < samples; k++) {
      tone_buffer[k*soundChannels] = randn(0, noise_sigma);
      checkSample(tone_buffer[k*soundChannels]);
      stereo_encode(tone_buffer, k*soundChannels);
    }
    Csrc::writeBuffer(tone_buffer, samples);
    

    
    for(int i = 0; i < 5; i++) {				// plays 5 ticks (syncronization)
      samples = 0.1*sample_frequency;
      for(k = 0; k < samples; k++) {
        tone_buffer[k*soundChannels] = amplitude*cos(2.0*M_PI*Fsync/sample_frequency*k + theta) + randn(0, noise_sigma);
        checkSample(tone_buffer[k*soundChannels]);
        stereo_encode(tone_buffer, k*soundChannels);
      }
    
    
      while (k < sample_frequency) {
        tone_buffer[k*soundChannels] = randn(0, noise_sigma);
        checkSample(tone_buffer[k*soundChannels]);
        stereo_encode(tone_buffer, k*soundChannels);
        k++;
      }
      Csrc::writeBuffer(tone_buffer, sample_frequency);
    }


    if(ticks >= 6) {
    
      for(k = 0; k < sample_frequency; k++) {		// 1 second of silence before the last tick
        tone_buffer[k*soundChannels] = randn(0, noise_sigma);
        checkSample(tone_buffer[k*soundChannels]);
        stereo_encode(tone_buffer, k*soundChannels);
      }
    
      Csrc::writeBuffer(tone_buffer, sample_frequency);
    
      samples = 0.1*sample_frequency;
      for(k = 0; k < samples; k++) {			// this is the last tick!
        tone_buffer[k*soundChannels] = amplitude*cos(2.0*M_PI*Fsync/sample_frequency*k + theta) + randn(0, noise_sigma);
        checkSample(tone_buffer[k*soundChannels]);
        stereo_encode(tone_buffer, k*soundChannels);
      }
    
      Csrc::writeBuffer(tone_buffer, samples);

      if(ticks == 7) {
        samples = 0.9*sample_frequency;
        for(k = 0; k < samples; k++) {
          tone_buffer[k*soundChannels] = randn(0, noise_sigma);
          checkSample(tone_buffer[k*soundChannels]);
          stereo_encode(tone_buffer, k*soundChannels);
        }

        Csrc::writeBuffer(tone_buffer, samples);

        samples = 0.1*sample_frequency;
        for(k = 0; k < samples; k++) {
          tone_buffer[k*soundChannels] = amplitude*cos(2.0*M_PI*Fsync/sample_frequency*k + theta) + randn(0, noise_sigma);
          checkSample(tone_buffer[k*soundChannels]);
          stereo_encode(tone_buffer, k*soundChannels);
        }
        Csrc::writeBuffer(tone_buffer, samples);
      }
    }
    add_minute();
  }

  error = 0;
  running = false;
  delete[] tone_buffer;
}

inline void Csrc::stereo_encode(float* p, int k)
{
  if(soundChannels == 2)
    p[k + 1] = p[k];
}



inline void Csrc::checkSample(float& s)
{
  if(s < -1) s = -1;
  else if (s > 1) s = 1;
}



void Csrc::encode()
{
  src_vector[0] = 0;		// first set the ID1
  src_vector[1] = 1;
  bin_convert(hour, 2, 6);
  bin_convert(min, 8, 7);
  if(dst) src_vector[15] = 1;
  else src_vector[15] = 0;
  
  src_vector[16] = parity(0, 15);	// P1
  bin_convert(month, 17, 5);
  bin_convert(day, 22, 6);
  bin_convert(wday, 28, 3);
  src_vector[31] = parity(17, 30);	// P2
  
  // set second block
  src_vector[32] = 1;		// ID2
  src_vector[33] = 0;
  bin_convert(year%100, 34, 8);
  if((change_time < 0) || (change_time > 6)) {	// set warning for change time
    src_vector[42] = 1;
    src_vector[43] = 1;
    src_vector[44] = 1;
  }
  else {
    bin_convert(change_time, 42, 3);
    if(verbose_level >= 5) lout <<"encode() Warning of change time: " <<change_time <<'\n';
  }
  switch(leap_second) {	// set the warning for leap second
    case +1: src_vector[45] = 1;
             src_vector[46] = 0;
             if(verbose_level >= 5) lout <<"encode() Leap second at the end of the month: +1s\n";
             break;
    case -1: src_vector[45] = 1;
             src_vector[46] = 1;
             if(verbose_level >= 5) lout <<"encode() Leap second at the end of the month: -1s\n";
             break;
    case 0:
    default: src_vector[45] = 0;
	     src_vector[46] = 0;
  }
  
  src_vector[47] = parity(32, 46);	// PA
}







int Csrc::setWDS(int Wlength, double snr)
{
  if(Wlength < 1) {
    window_length = 0;
    adaptive_decision_threshold = false;
  }
  else {
    window_length = Wlength;
    adaptive_decision_threshold = true;
  }

  snr_level = pow10(snr/10);

  if(verbose_level >= 2) lout <<"WDS. New window length: " <<window_length <<" symbols; SNR: " <<10*log10(snr_level) <<" dB\n";

  return window_length;
}









///////////////////////////////////////////////////////////////////////////////////
//         DECODING
///////////////////////////////////////////////////////////////////////////////////





bool Csrc::decode()
{
  if(!streamON) {
    decoded = false;
    error = -2;
    return false;
  }
  else reset_src_vector();

  const int N = int(sample_frequency*Ts);
  const int DELTA = N;
  const int STEP = N/30;			// tuning function with 1 microsec uncertainty
  long int total = 0;
  float* buffer = new float[(2*N + DELTA)*soundChannels];
  double power0, power1;
  int r, c, bytes2read, extra;
  vector<double> window(window_length, 0.0);	// vector of avarage of level of noise in the last symbols
  double avg;


  decoded = false;
  
  if(verbose_level >= 2) lout <<"Threshold: " <<get_decision_threshold() <<" dB\n";
  
  reset_buffer(buffer, (2*N + DELTA)*soundChannels);
  c = 0;		// number of bits decoded
  bytes2read = N;
  extra = 0;
  error = -1;
  
  power0 = power1 = 0.0;

// The cycle that acquires the SRC data starts here
  running = true;

  while(running && ((total*N/sample_frequency) < timeout) && (!decoded)) {	// iterates till: running is true, the timeout is not expired and the SRC has not been decoded
    r = read_buffer(buffer, 2*N, bytes2read, extra);

    if(r < 0) {	// error in the input stream
      running = false;
      decoded = false;
      error = -3;
      if(verbose_level >= 1) lerr <<"EE: Reading error!!\n";
      break;
    }

    power0 = goertzel(F0, &buffer[N], N);	// power associated to frequency F0
    power1 = goertzel(F1, &buffer[N], N);	// power associated to frequency F1


    //---------------------------------------------------------------- Window Calibration System
    if(window_length > 0) {	// WDS is on
      window[total % window_length] = (power0 + power1)/2.0;
      if(((total / window_length) != 0) && (c == 0)) {
        avg = 0.0;
        for(int i = 0; i < window_length; i++) avg += window[i];
        avg /= window_length;	// average noise level in the last symbols time
        decision_threshold = avg*snr_level;
        if(decision_threshold > 1)
          decision_threshold = max(avg, 1.0/snr_level);
        if(verbose_level >= 4)
          lout <<"Threshold now is " <<get_decision_threshold() <<" dB. Noise average of the last " <<window_length
               <<" time symbols is " <<10*log10(avg) <<" dB; Pass: " <<total <<'\n';
      }
    }	//------------------------------------------------------------ End of Window Calibration System



    if(verbose_level >= 5) {
      lout <<"[DEBUG] Pass " <<total <<" Power : " <<10*log10(power0) <<" dB; Power2 : " <<10*log10(power1) <<" dB\n";
    }


	//  || ((c > 0) && (c != 32))
    if((power0 > decision_threshold) || (power1 > decision_threshold)) {	// checks wheter one of the two frequencies has reached the threshold
      if(((c == 0) || (c == 32)) && (verbose_level >= 2)) {
	lout <<"Supposed detection at pass " <<total <<". Power level: " <<10*log10(max(power0, power1)) <<" dB for frequency ";
	if(power0 > power1) lout <<"F0\n";
	else lout <<"F1\n";
        lout <<"Decision Threshold: " <<get_decision_threshold() <<" dB\n";
      }
      
      if(power0 > power1) {  // 0
	if(c == 0) {  // syncronization of first block
	  bytes2read = tuning(F0, buffer, extra, N, DELTA, STEP, power0);
	  if(verbose_level >= 2) lout <<"Tuned @ pass: " <<total <<'\n';
          avg = 0.0;	// reset avg in order to calculate the average of the power of the tones F0 and F1
	}
	src_vector[c] = 0;
      }
      else {  // 1
	if(c == 32) {  // syncronization of second block
	  bytes2read = tuning(F1, buffer, extra, N, DELTA, STEP, power1);
	  if(verbose_level >= 2) lout <<"Tuned @ pass: " <<total <<'\n';
	}
	src_vector[c] = 1;
      }
      if(verbose_level >= 3) lout <<'[' <<itos(c,2) <<"] " <<src_vector[c] <<" Power: " <<10*log10(max(power0, power1)) <<" dB\n";
      c++;
      avg += max(power0, power1);
    }
    else if(c != 0)
           src_vector[c++] = -1;		// if the symbol is under threshold during the transmission forces a reset.
    
    
    if(!check(c)) {
      if(verbose_level >= 2) lout <<"EE: Detection error. RESET\nPass " <<total <<"; Error code: " <<error <<"\n-----\n";

      c = 0;
      avg = 0.0;
      reset_buffer(buffer, (2*N + DELTA)*soundChannels);
      reset_src_vector();
    }
    
    

    if(c == 32) {
    /* At this point the first block has been decoded.
       Time to clear the noise samples between the first and second block
       and to reset the buffer in order to avoid wrong calculation of the power of the next tone.
    */
      r = readBuffer(buffer, 0.04*sample_frequency);
      reset_buffer(buffer, (2*N + DELTA)*soundChannels);
    }

    total++;	// another sample has been processed!


// ***
// Check whether all the 48 symbols have been received
// ***
    if(c == 48) {
      check(48);		// checks the entire binary sequence
      if(error == 0) {
        int lp;
        hour = deconvert(2, 6);
        min = deconvert(8, 7);
        dst = bool(src_vector[15]);
      
        month = deconvert(17, 5);
        day = deconvert(22, 6);
        wday = deconvert(28, 3);
        year = deconvert(34, 8) + 2000;
      
        change_time = deconvert(42, 3);
        lp = deconvert(45, 2);
        switch(lp) {
	  case 0: leap_second = 0;
		  break;
	  case 2: leap_second = +1;
		  break;
	  case 3: leap_second = -1;
		  break;
	  default: error = 5;
        }
        sec = 53;
        msec = 480;

	decoded = (c == 48) && ID1() && P1() && P2() && ID2() && PA() && (error == 0) && valid_date();	// decoded!!
      }
      
      if(!decoded) {
        if(verbose_level >= 2) lout <<"EE: decoding error after 48 symbols. Error code: " <<error <<"; Valid date: "
					<<valid_date() <<"\nResetting...\n";
        c = 0;
        avg = 0.0;
        reset_src_vector();
      }
    }
  }
//--------------------------------------------------------------------------------
// end of the SRC data

  delete[] buffer;

  if(!running) {
    decoded = false;
    return decoded;
  }
  else if(c < 48) {
    if(verbose_level >= 1) lerr <<"*** TIMEOUT! ***\n";
    set_today();
    error = 6;
    decoded = false;
    return decoded;
  }

  
//--------------------------------------------------------------------------------
// Syncronization
//--------------------------------------------------------------------------------
  if((do_sync == true) && (error == 0) && running && decoded) {
    const int Nsync = int(0.1*sample_frequency);			// samples for syncronization signal
    const int DELTAsync = Nsync;
    const int STEPsync = Nsync/100;
    float* syncBuffer = new float[(2*Nsync + DELTAsync)*soundChannels];
    double power = 0.0;
    int ticks;
    int noise_symbols;
    long int syncTimeout, syncTimeout_sec;
    
    if(verbose_level >= 1) lout <<"---- SRC received! Synchronization! ----\n";
    
    reset_buffer(syncBuffer, (2*Nsync + DELTAsync)*soundChannels);
    
    c = 0;			// ticks decoded
    bytes2read = Nsync;
    extra = 0;
    syncTimeout = 0;
    noise_symbols = 5;		// 5 noise symbols to process before the sequence of RP
    ticks = number_of_RP();	// determines the number of RP to expect

    error = 7;			// SRC decoded but not syncronised
    msec = 0;
    syncTimeout_sec = ticks + 1;	// the total duration of the sync timeout in seconds is the number of ticks plus
					// 1 second between the last two RP. In this calculation also the time elapsed between
					// the end of segment S2 and the first RP is considered.

    if(ticks != 6) {
      if(verbose_level >= 1) lout <<"This minute has " <<(60+leap_second) <<" seconds! The sync ticks are " <<ticks <<'\n';
      leap_second = 0;
    }

    if(adaptive_decision_threshold)
      decision_threshold = max(decision_threshold, avg/(snr_level*48));	// updated the decision TH from the previous F0 and F1 average power

    while((c < ticks) && (((syncTimeout*Nsync)/sample_frequency) < syncTimeout_sec) && running) {

      r = read_buffer(syncBuffer, 2*Nsync, bytes2read, extra);
      power = goertzel(Fsync, &syncBuffer[Nsync], Nsync);

      if(noise_symbols > 0) {
        decision_threshold = max(decision_threshold, power*snr_level);
        if(decision_threshold > 1)
          decision_threshold = max(power, 1.0/snr_level);
        noise_symbols--;
      }

      if(power > decision_threshold) {
        if(c == 0) {
          if(verbose_level >= 3) lout <<"Supposed syncronisation RP at pass " <<syncTimeout <<". TH: " <<get_decision_threshold() <<" dB\n";
	  bytes2read = tuning(Fsync, syncBuffer, extra, Nsync, DELTAsync, STEPsync, power);

	  if(adaptive_decision_threshold) {
	    decision_threshold = power/2.0;	// Sync threshold is -3 dB below the signal power
	    if(verbose_level >= 2) lout <<"Synchronization threshold: " <<get_decision_threshold() <<" dB\n";
	  }
	}

        c++;
	if(verbose_level >= 1) {
	  if(c < ticks) {
            lout <<"===== ";
          }
	  else lout <<"|||||";
	  lout.flush();       // flush output buffer
	  if(verbose_level >= 3) lout <<"Sync tick number: " <<c <<" Power: " <<10*log10(power) <<" dB; (TH: "
					<<get_decision_threshold() <<" dB)\n";
	}
	sec++;
      }
      else if(verbose_level >= 4) lout <<"Sync under threshold: " <<10*log10(power) <<" dB\n";
      syncTimeout++;
    }
    
    if(c == ticks) {
      long long nanosec;
      add_minute();
      error = 0;
      if(verbose_level >= 1) lout <<" =====> [Synchronized!]\n";
      msec = 100;
      std::chrono::duration<double,std::ratio<1,1000000000>> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - sys_clock);
      nanosec = time_span.count();	// syncronisation offset in nanoseconds since the reading of last RP

      if(verbose_level >= 2) lout <<"Syncronisation offset in ns: " <<nanosec <<" ns\n";
    }
    else {
      sec = 53;			// if the system is not able to syncronize, it chooses the end of the encoded block
      if(verbose_level >= 1) lerr <<"\nEE: Sync timeout expired. Timeout: " <<((syncTimeout*Nsync)/sample_frequency) << " seconds.\n"
				  <<"\tTicks received: " <<c <<" out of " <<ticks <<'\n';
      error = 7;
    }
    
    delete[] syncBuffer;
  }
//--------------------------------------------------------------------------------
// END Syncronization
//--------------------------------------------------------------------------------
  running = false;	// finished! stop running!
  
  return decoded;
}			// end of decode() function!


long Csrc::microsecDelay() const
{
  long int micro = 0;

      std::chrono::duration<long int,std::ratio<1,1000000l>> time_span = std::chrono::duration_cast<std::chrono::duration<long int>>(std::chrono::high_resolution_clock::now() - sys_clock);
  micro = time_span.count();

  return micro;
}


/** The leapsecond is applied at the enld of the month UTC time. This means that it applies at time 00:59 +0100
  *  or 01:59 +0200 of the 1st of the new month for the Italian time.
  * 
  */
int Csrc::number_of_RP() const
{
  int t = 6;		// in normal cases there are just 6 ticks

  if((day == 1) && ((hour - int(dst)) == 0) && (min == 59) && (leap_second != 0))
    t += leap_second;

  return t;
}


void Csrc::set_timeout(int seconds)
{
  if(seconds > 2)
    timeout = seconds;
}


double Csrc::set_decision_threshold(double dB)
{
  if(dB > 0.0) dB *= -1;	// change the sign if not correct

  decision_threshold = pow(10, (dB/10.0));
  return decision_threshold;
}

inline double Csrc::get_decision_threshold() const
{
  return 10*log10(decision_threshold);
}



bool Csrc::valid_date() const
{
  bool isvalid;

  
  isvalid = ((sec >= 0) && (sec <= 60) && (min >= 0) && (min < 60) && (hour >= 0) && (hour < 24));

  if(isvalid) {
    int numberOfDays = 0;		// number of days in a month
    switch(month) {
      case 1: case 5:
      case 3: case 7:
      case 8: case 10:
      case 12:	numberOfDays = 31;
		break;
      case 4:
      case 6:
      case 9:
      case 11:	numberOfDays = 30;
		break;
      case 2:	if(Csrc::leapyear(year)) numberOfDays = 29;
		else numberOfDays = 28;
		break;
      default:	isvalid = false;
    }
    
    if(isvalid == true) {
      isvalid = ((day > 0) && (day <= numberOfDays));

      if(isvalid) {		// check the day of the week
        int month_table[12] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
        int y, day_of_week;

        y = year - int(month < 3);
        day_of_week = (y + y/4 - y/100 + y/400 + month_table[month-1] + day) % 7;
        if(day_of_week == 0) day_of_week = 7;
        isvalid = (day_of_week == wday);
      }
    }
  }
  
  return isvalid;
}




bool Csrc::leapyear(int y)
{
  return (!(y % 400) || (!(y % 4) && (y % 100)));
}



int Csrc::deconvert(int offset, int length)
{
  const int SIZE = 8;
  const int cv[SIZE] = {80,40,20,10,8,4,2,1};	// sizeof cv[] => 8
  int val = 0;

  for(int i = 0; i < length; i++) val += src_vector[i + offset]*cv[(SIZE - length) + i];

  return val;
}


inline void Csrc::reset_src_vector()
{
  for(int i = 0; i < 48; i++) src_vector[i] = -1;	// put all elements with an error value
}

inline void Csrc::reset_buffer(float* b, int size, float value)
{
  for(int i = 0; i < size; i++) b[i] = value;
}


int Csrc::read_buffer(float* b, int total_size, int& bytes2read, int& extra)
{
  int r;
  int m = total_size/2;
  float* pp;


  if(bytes2read == m) {
    for(int i = 0; i < (m+extra); i++) b[i] = b[m + i];
    pp = &b[m + extra];
    bytes2read -= extra;
    extra = 0;
  }
  else pp = &b[2*m - bytes2read];
  r = Csrc::readBuffer(pp, bytes2read);
  if(r != bytes2read) bytes2read = m - r; //lerr <<"Read " <<r <<" samples instead " <<bytes2read <<"!\n";
  else bytes2read = m;		// next time, m samples will be read

  return r;
}


// implementation of the goertzel algorithm
double Csrc::goertzel(int frequency, const float* data, int samples) const
{
  double coeff;
  double V0, V1, V2;
  double power;

  // Initialize Goertzel Algorithm
  V0 = V1 = V2 = 0.0;
  coeff = 2.0*cos(2.0 * M_PI * frequency / sample_frequency);

  for (int i = 0; i < samples; i++) {
      V0 = data[i] + (coeff * V1) - V2;
      V2 = V1;
      V1 = V0;
  }
  power = V2*V2 + V1*V1 - coeff*V2*V1;		// not normalized power
  power = power/(samples*samples)*4;		// normalization: it returns the power of the single side sinusoid

  return power;
}



// Tuning function
int Csrc::tuning(int freq, float* buffer, int& extra, int N, int DELTA, int STEP, double& p)
{
  int tuned, bytes2read;//, size;
  double power, maxpower;

  maxpower = 0.0;
  const int start = N - DELTA;
  const int end = N + DELTA;


  Csrc::readBuffer(&buffer[2*N + extra], DELTA - extra);	// read DELTA more samples

  for(int i = start; i <= end; i += STEP) {
    power = goertzel(freq, &buffer[i], N);
    if(verbose_level >= 6) lout <<"Power of frequency " <<freq <<" Hz, starting from sample " <<i <<" = " <<10*log10(power) <<" dB\n";
    if(maxpower < power) {
      tuned = i;
      maxpower = power;
      if(verbose_level >= 6) lout <<"Tuned!\tMax found in " <<i <<" = " <<10*log10(power) <<" dB\n";
    }
  }

  // prepare the buffer for next reading
  for(int i = 0; i < (2*N - tuned + DELTA); i++) buffer[i] = buffer[tuned + i];

  if(tuned <= N) {
    extra = DELTA - tuned;		// extra are the samples that exceed the next symbol.
    bytes2read = 0;
  }
  else {
    bytes2read = tuned - DELTA;		// in the buffer there are already N + DELTA - tuned samples
    extra = 0;
  }

  if(verbose_level >= 3) lout <<"Tuning function. Tuned: " <<tuned <<" (start = " <<start <<"; end = " <<end <<"; step = " <<STEP <<")\n"
			 <<"bytes2read : " <<bytes2read <<";  extra = " <<extra <<"; MaxPower: " <<10*log10(maxpower)
			<<" dB\n";

  p = maxpower;

  return bytes2read;	// are the samples to read from the stream for the next symbol
}


string Csrc::itos(int value, int length, int base, char fill, bool force_sign)
{
  string rev, s;
  int c, q, r;

  c = r = 0;
  q = value;
  if(q < 0) {
    s = "-";
    q = - value;
  }
  if(force_sign && (value > 0)) s = "+";

  while(q > 0) {
    r = q % base;
    q /= base;
    rev += char('0' + r);
    c++;
  }

  while(c < length) {
    s += fill;
    c++;
  }

  s.append(rev.rbegin(), rev.rend());
  return s;
}


bool Csrc::check(int bits)
{
  error = -1;

  for(int i = 0; i < bits; i++) {
    if((src_vector[i] != 0) && (src_vector[i] != 1)) {
      error = 6;
      return false;
    }
  }

  // check of the IDs depending on the number of bits decoded. Also the last parity bit is checked
  switch(bits) {
    case 48: if(!PA()) error = 5;
	     else error = 0;
    case 34: if(src_vector[33] != 0) error = 4;
    case 33: if(src_vector[32] != 1) error = 4;
    case 2:  if(src_vector[1] != 1) error = 1;
    case 1:  if(src_vector[0] != 0) error = 1;
	     break;
  }
  
  if(bits > 16) {
    if(!P1())
      error = 2;
    if(bits > 31) {
      if(!P2())
	error = 3;
    }
  }

   return (error <= 0);
}


void Csrc::add_minute()
{
  sec = 0;
  if(min != 59) min++;
  else {
    min = 0;
    if(hour != 23) {
      if((change_time == 0) && (((hour == 1) && (dst == false)) || ((hour == 2) && (dst == true)))) {
        hour = 3 - int(dst);
        dst ^= true;		// change the value of the dst variable
        change_time = 7;
      }
      else hour++;
    }
    else {
      hour = 0;
      if(wday != 7) wday++;
      else wday = 1;
      
      int numberOfDays;		// number of days in a month
      switch(month) {
	case 1: case 5:
	case 3: case 7:
	case 8: case 10:
	case 12: numberOfDays = 31;
		 break;
	case 4:
	case 6:
	case 9:
	case 11: numberOfDays = 30;
		 break;
	case 2:	 if(Csrc::leapyear(year)) numberOfDays = 29;
		 else numberOfDays = 28;
		 break;
      }
      if(day < numberOfDays) day++;
      else {
	day = 1;
	if(month < 12) month++;
	else {
	  month = 1;
	  year++;
	}
      }
    }
  }
}


inline double Csrc::max(double a, double b) const
{
  if(a > b) return a;
  else return b;
}




std::ostream& operator<<(std::ostream& os, const Csrc& src)
{
  for(int i = 0; i < 48; i++) {
    os <<src.src_vector[i];
    if(i == 31) os <<' ';
  }
  
  return os;
}


std::istream& operator>>(std::istream& is, Csrc& src)
{
  char c;
  
  for(int i = 0; i < 48; i++) {
    is >> c;
    
    switch(c) {
      case '0': src.src_vector[i] = 0;
		break;
      case '1': src.src_vector[i] = 1;
		break;
      default:  src.src_vector[1] = 0;		// error
    }
    
    if(!src.check(i+1))
      break;
  }
  
    src.check(48);		// checks the entire binary sequence
    if(src.error == 0) {
      int lp;
      src.hour = src.deconvert(2, 6);
      src.min = src.deconvert(8, 7);
      src.dst = bool(src.src_vector[15]);
      
      src.month = src.deconvert(17, 5);
      src.day = src.deconvert(22, 6);
      src.wday = src.deconvert(28, 3);
      src.year = src.deconvert(34, 8) + 2000;
      
      src.change_time = src.deconvert(42, 3);
      lp = src.deconvert(45, 2);
      switch(lp) {
	case 0: src.leap_second = 0;
		break;
	case 2: src.leap_second = +1;
		break;
	case 3: src.leap_second = -1;
		break;
	default: src.set_today();
		 src.decoded = false;
		 src.error = 5;
      }
      src.sec = 53;
      if(!src.valid_date() || (src.error != 0))
	src.set_today();
    }
    else src.set_today();
    
    
    src.decoded = false;
  
  return is;
}


string Csrc::dateSTD() const
{
  const char* wday_str[]  = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
  const char* month_str[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Set", "Oct", "Nov", "Dec"};
  string str;

  str = wday_str[wday-1];
  str += ", " + itos(day, 2) + " " + month_str[month-1] + " " + itos(year, 4) + " ";
  str += itos(hour, 2) + ":" + itos(min, 2) + ":" + itos(sec, 2) + " " + "+" + itos(1 + int(dst), 2) + "00"; 

  return str;
}


string Csrc::dateISO() const
{
  string str;
  
  str = itos(year, 4) + "-" + itos(month, 2) + "-"  + itos(day, 2);
  str += "T" + itos(hour, 2) + ":" + itos(min, 2) + ":"  + itos(sec, 2) + "+" + itos(1 + int(dst), 2) + "00";
  
  return str;
}


string Csrc::dateSTR(bool iso8601) const
{
  if(iso8601) return dateISO();
  else return dateSTD();
}

void Csrc::logOnFile(const char* fileName)
{
  lout.streamOnFile(fileName);
//  lerr.streamOnFile(fileName);
}

void Csrc::logOnSTDOUT()
{
  lout.streamOnSTDOUT();
  lerr.streamOnSTDOUT();
}

void Csrc::errorLogOnFile(const char* fileName)
{
  lerr.streamOnFile(fileName);
}



bool Csrc::set(int MI, int OR, int GM, int GS, int ME, int AN, bool OE, int SE, int SI)
{
  bool correct_date;
  decoded = false;
  
  sec = 53;
  min = MI;
  hour = OR;
  day = GM;
  wday = GS;
  month = ME;
  year = AN;
  dst = OE;
  change_time = SE;
  leap_second = SI;
  
  if((!valid_date()) || (change_time < 0) || (change_time > 7) || (leap_second < -1) || (leap_second > 1)) {
    set_today();
    correct_date = false;
  }
  else correct_date = true;
  
  encode();
  
  if(verbose_level >= 2)
    lout <<"SRC set to:\nSeconds: " <<sec <<"\nMinutes: " <<min <<"\nHours: " <<hour
	 <<"\nDay: " <<day <<"\nMonth: " <<month <<"\nYear: " <<year
	 <<"\nDST: " <<dst <<";\tChange time: " <<change_time <<";\tLeap second: " <<leap_second <<'\n';

  msec = 0;
  return correct_date;
}


bool Csrc::set(const struct tm& ttmm)
{
  bool localdst;
  int weekday;

  weekday = ttmm.tm_wday;
  if(weekday == 0) weekday = 7;
  if(ttmm.tm_isdst) localdst = true;
  else localdst = false;

  return set(ttmm.tm_min, ttmm.tm_hour, ttmm.tm_mday, weekday, ttmm.tm_mon+1, ttmm.tm_year + 1900, localdst);
}


bool Csrc::set(const char* strDate, const char* format)
{
  tm ttmm;
  time_t tt = time('\0');
  localtime_r(&tt, &ttmm);		// initialize the ttmm to the current date/time

  strptime(strDate, format, &ttmm);	// convert a string to a struct tm
  return set(ttmm);
}



bool Csrc::setWarnings(int SE, int SI)
{
  bool correct;
  if((SE >= 0) && (SE <= 7) && (SI >= -1) && (SI <= 1)) {
    change_time = SE;
    leap_second = SI;
    encode();
    correct = true;
  }
  else correct = false;
  
  if(verbose_level >= 2)
    lout <<"SRC set to:\nSeconds: " <<sec <<"\nMinutes: " <<min <<"\nHours: " <<hour
	 <<"\nDay: " <<day <<"\nMonth: " <<month <<"\nYear: " <<year
	 <<"\nDST: " <<dst <<";\tChange time: " <<change_time <<";\tLeap second: " <<leap_second <<'\n';

  return correct;
}

bool Csrc::setOE(bool newDST)
{
  dst = newDST;
  encode();

  return dst;
}



int* Csrc::get_src_vector_int() const
{
  int* v = new int[48];
  
  for(int i = 0; i < 48; i++)
    v[i] = src_vector[i];
  
  return v;
}




void Csrc::close_all()
{
  streamON = false;
  Crw::close_all();
}

void Csrc::close_input_stream()
{
  streamON = false;
  Crw::close_input_stream();
}

void Csrc::close_output_stream()
{
  streamON = false;
  Crw::close_output_stream();
}


