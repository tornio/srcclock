/**
 *	@mainpage SRCclock
 *	@author Vittorio Tornielli di Crestvolant <vittorio.tornielli@gmail.com>
 *	@date 2009-2014
 *	@copyright (C) 2014  Vittorio Tornielli di Crestvolant
 *	@version 1.0
 *
 *	@section Description
 *
 *	This program decodes/plays the SRC RAI radio signal created by the INRIM.
 *	The SRC (Segnale orario Rai Codificato) is generated every minute by the
 *	laboratories of the INRIM (Istituto Nazionale di Ricerca Metrologica) and
 *	transmitted to RAI for broadcasting. Any AM/FM radio tuner incorporated or attached
 *	to the computer can be used to receive and to decode the SRC through this
 *	program. Alternatively, it is possible to read the raw audio data from file
 *	or to reproduce the signal for test purposes.
 *
 *
 *   @section License
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */



#include <iostream>
#include <string>
#include <cstdlib>
#include <getopt.h>
#include <sys/time.h>
#include "csrc.h"

using std::cout;
using std::cerr;


// struct used to gather command line options
struct SRCoption {
	int SRCaction;
	bool random_samples, random_theta,  do_sync, binary, iso, sys_sync;
	int verb, chdate, leap, fc, channels, timeout, wds, repeat, dst;
	double th, power, noise, snr_level;
	long delay;
	char *soundDev, *fo, *logfile, *setDate;
};
	



void print_help(const char* filename)
{
  cout  <<"Usage:\t" <<filename <<" --decode [OPTIONS]\n"
	<<"or\t" <<filename <<" --play [OPTIONS]\n"
	<<"\nOption list:\n"
	<<"  -d, --decode\t\tdecode SRC signal\n"
	<<"  -y, --system-sync\tSyncronise the system clock to the SRC. Requires\n\t\t\tsuperuser privileges.\n"
	<<"  -N, --snr=SNR_LEVEL\tSNR detection level over the noise in dB.\n\t\t\tDefault is SNR_LEVEL=5 dB abose noise level\n"
	<<"  -W, --window=LENGTH\tWindow Decision System. Set the length of the window\n\t\t\tin time symbols. Default is LENGTH=50 symbols\n"
	<<"  -D, --delay=DELAY\tdelay of syncronisation in microseconds\n"
	<<"  -T, --timeout=TIMEOUT\tset the timeout for decoding in seconds\n"
	<<"  -t, --threshold=TH\tset static decision threshold in dB (default -35 dB)\n"
	<<'\n'
	<<"  -p, --play\t\tplay SRC signal\n"
	<<"  -k, --rand-theta\tgives a random theta while playing\n"
	<<"  -o, --rand-samples\tadd random samples (< 1 sec) before playing the signal\n"
	<<"  -a, --power=POWER\tlevel of power of the wave in dB when playing\n"
	<<"  -n, --noise=SIGMA\tplays the SRC with a given noise expressed as the RMS\n"
	<<"  -S, --set-date=DATE\tSet the SRC date time in the format <hh:mm DD/MM/YYYY>\n"
	<<"  -e, --dst-on\t\tforce the dst flag (OE) to be 1\n"
	<<"  -E, --dst-off\t\tforce the dst flag (OE) to be 0\n"
	<<"  -C, --change-time=SE\tset SRC warning days of change solar/winter time [0-7]\n\t\t\t(7 = no warning, default)\n"
	<<"  -l, --leap-second=SI\tset SRC warning of leap second (+1|-1)\n\t\t\tsecond at the end of the month\n"
	<<'\n'
	<<"  -s, --sync\t\tsyncronisation with RP tones.\n\n"
	<<"  -r, --rate=FREQ\tset the sample frequency (for input/output on files\n\t\t\tdefault is 8 kHz)\n"
	<<"  -m, --mono\t\tsound stream is mono (1 channel). Default is stereo\n\t\t\t(2 channels)\n"
	<<"  -M, --stero\t\tsound stream is stereo (2 channels). This is the\n\t\t\tdefault setting\n"
	<<"  -f, --file=FILE\tselect source/destination file (default is sound server)\n"
	<<"  -c, --card=DEV\tspecifies a different sound device\n"
	<<"  -v, --debug=LEVEL\tverbose level (default 1)\n"
	<<"  -I, --iso\t\tPrint the date/time in the format ISO 8601\n\t\t\t(default: RFC2822 format)\n"
	<<"  -b, --binary\t\tPrint the binary representation of the SRC signal\n"
	<<"  -R, --repeat=TIMES\tNumber of decoding repetition (default = 1.\n\t\t\tSet 0 for unlimited repetitions)\n"
	<<"  -L, --logfile=LOG\tredirect outputs to file log\n"
	<<"  -w, --warranty\twarranty details\n"
	<<"  -V, --version\t\tversion of the program\n"
	<<"  -h, --help\t\tprint this help then exits.\n\n";
}



void print_warranty()
{
  cout <<"srcclock  Copyright (C) 2014  Vittorio Tornielli di Crestvolant\n"
       <<"                              Email: vittorio.tornielli@gmail.com\n\n"
       <<"This program comes with ABSOLUTELY NO WARRANTY.\n"
       <<"This is free software, and you are welcome to redistribute it\n"
       <<"under certain conditions; the details of the license can be found in the file\n"
       <<"LICENSE.txt along with this program. If not, see <http://www.gnu.org/licenses/>.\n\n";
}



int main(int argc, char **argv) {

  Csrc SRC;
  SRCoption options;
  const char* VERSION = "1.0";
  int error = 0;
  int choice;
  
  bool openState;	// used for error control
  
  
  // default values:
  options.random_samples = false;
  options.random_theta = false;
  options.do_sync = false;
  options.binary = false;
  options.iso = false;
  options.sys_sync = false;
  options.dst = 0;		// default, set by system date
  options.verb = 1;		// normal verbose level
  options.chdate = 7;		// no change date
  options.leap = 0;
  options.fc = 8000;		// 8 kHz
  options.channels = 2;		// stereo, 2 channels
  options.th = -35;
  options.power = -6.0;
  options.noise = 0.0;
  options.timeout = 0;		// uses default
  options.wds = 50;
  options.snr_level = 5.0;	// 5 dB SNR default
  
  options.fo = '\0';
  options.soundDev = '\0';	// default sound device
  options.logfile = '\0';
  options.setDate = '\0';
  options.repeat = 1;
  options.SRCaction = 0;
  options.delay = 1;



  if(argc == 1) {
    cerr <<"EE: At least one option required.\n\n";
    print_warranty();
    print_help(argv[0]);
    return 1;
  }

  int longindex = 0;	// variable used for the getopt_long function
  static struct option long_options[] = {
		{"decode",       no_argument,       NULL, 'd'},
		{"play",         no_argument,       NULL, 'p'},
		{"system-sync",  no_argument,       NULL, 'y'},
		{"delay",        required_argument, NULL, 'D'},
		{"rand-theta",   no_argument,       NULL, 'k'},
		{"rand-samples", no_argument,       NULL, 'o'},
		{"set-date",     required_argument, NULL, 'S'},
		{"dst-on",       no_argument,       NULL, 'e'},
		{"dst-off",      no_argument,       NULL, 'E'},
		{"power",        required_argument, NULL, 'a'},
		{"sync",         no_argument,       NULL, 's'},
		{"file",         required_argument, NULL, 'f'},
		{"debug",        required_argument, NULL, 'v'},
		{"threshold",    required_argument, NULL, 't'},
		{"noise",        required_argument, NULL, 'n'},
		{"snr",          required_argument, NULL, 'N'},
		{"window",       required_argument, NULL, 'W'},
		{"change-time",  required_argument, NULL, 'C'},
		{"leap-second",  required_argument, NULL, 'l'},
		{"rate",         required_argument, NULL, 'r'},
		{"mono",         no_argument,       NULL, 'm'},
		{"stereo",       no_argument,       NULL, 'M'},
		{"card",         required_argument, NULL, 'c'},
		{"iso",          no_argument,       NULL, 'I'},
		{"binary",       no_argument,       NULL, 'b'},
		{"timeout",      required_argument, NULL, 'T'},
		{"logfile",      required_argument, NULL, 'L'},
		{"repeat",       required_argument, NULL, 'R'},
		{"warranty",     no_argument,       NULL, 'w'},
		{"version",      no_argument,       NULL, 'V'},
		{"help",         no_argument,       NULL, 'h'},
		{0, 0, 0, 0}};


  while((choice = getopt_long(argc, argv, "dypkoeEa:sf:v:t:N:W:n:C:l:c:mMr:D:R:T:L:S:bIhVw", long_options, &longindex)) != -1) {
    switch (choice) {
      case 'd': options.SRCaction |= 1;		// SRCaction=1	=>	DECODE!
		break;
      case 'y': options.sys_sync = true;
		options.SRCaction |= 1;
		break;
      case 'p': options.SRCaction |= 2;		// SRCaction=2	=>	PLAY!
		break;
      case 'k': options.random_theta = true;
		options.SRCaction |= 2;
		break;
      case 'o': options.random_samples = true;
		options.SRCaction |= 2;
		break;
      case 'S': options.setDate = optarg;
		options.SRCaction |= 2;
		break;
      case 'e': options.dst = 1;
		options.SRCaction |= 2;
		break;
      case 'E': options.dst = 2;
		options.SRCaction |= 2;
		break;
      case 'a': options.power = atof(optarg);
		options.SRCaction |= 2;
		break;
      case 's': options.do_sync = true;
                break;
      case 'f': options.fo = optarg;
		break;
      case 'v': options.verb = atoi(optarg);
		break;
      case 't':	options.th = atof(optarg);
		options.SRCaction |= 1;
		break;
      case 'N': options.snr_level = atof(optarg);
		options.SRCaction |= 1;
		break;
      case 'W': options.wds = atoi(optarg);
		options.SRCaction |= 1;
		break;
      case 'n': options.noise = atof(optarg);
		options.SRCaction |= 2;
		break;
      case 'C': options.chdate = atoi(optarg);
		if((options.chdate < 0) || (options.chdate > 7)) {
		  cerr <<"EE: Change date input error! Parameter must be in the range [0-7]\n";
		  options.chdate = 7;	// no changes
		}
		options.SRCaction |= 2;
		break;
      case 'l': options.leap = atoi(optarg);
		if((options.leap != 0) && (options.leap != -1) && (options.leap != 1)) {
		  cerr <<"EE: Leap second must be 1 or -1 (no leap second => 0)\n";
		  options.leap = 0;
		}
		options.SRCaction |= 2;
		break;
      case 'r': options.fc = atoi(optarg);
		if((options.fc < 8000) || (options.fc > 48000)) {
		  cerr <<"EE: Sampling frequency must be in between 8000 and 48000 Hz\nDefault: 8000 Hz\n";
		  options.fc = 8000;
		}
		break;
      case 'm': options.channels = 1;
		break;
      case 'M': options.channels = 2;
		break;
      case 'c': options.soundDev = optarg;
		break;
      case 'D': options.delay = atol(optarg);
		options.SRCaction |= 1;
		break;
      case 'I': options.iso = true;
		break;
      case 'b': options.binary = true;
		break;
      case 'T': options.timeout = atoi(optarg);
		if(options.timeout < 2) {
		  cerr <<"EE: Timeout cannot be less than 2 seconds. Setting default -> 300 seconds\n";
		  options.timeout = 300;
		}
		options.SRCaction |= 1;
		break;
      case 'L': options.logfile = optarg;
		break;
      case 'R': options.repeat = atoi(optarg);
		break;
      case 'w': print_warranty();
		break;
      case 'V': cout <<"Version: " <<VERSION <<'\n';
		break;
      case '?': // unrecognized argument
		break;
      case 'h': print_help(argv[0]);
		return 0;
      default:	cerr <<"EE: Wrong argument!\n";
		print_help(argv[0]);
		return 1;
    }
  }

  
  if(options.verb >= 4) {
    cout <<"Option passed:\n\n"
	 <<"decodeSRC: " <<options.SRCaction <<'\n'
	 <<"Window Decision System length: " <<options.wds <<'\n'
	 <<"random_theta: " <<options.random_theta <<'\n'
	 <<"random_samples: " <<options.random_samples <<'\n'
	 <<"System sincronisation: " <<options.sys_sync <<'\n'
	 <<"Power level: " <<options.power <<" dB\n"
	 <<"Do sync: " <<options.do_sync <<'\n'
	 <<"Verbose level: " <<options.verb <<'\n'
	 <<"WDS: " <<options.wds <<'\n'
	 <<"SNR level: " <<options.snr_level <<'\n'
	 <<"Noise RMS: " <<options.noise <<'\n'
	 <<"Change date: " <<options.chdate <<'\n'
	 <<"Leap second: " <<options.leap <<'\n'
	 <<"Sampling frequency: " <<options.fc <<" Hz\n"
	 <<"Channels: " <<options.channels <<'\n'
	 <<"Repeat: " <<options.repeat <<'\n'
	 <<"Timeout: " <<options.timeout <<'\n'
	 <<"Sync delay: " <<options.delay <<'\n';

    if(options.fo) cout <<"FileStream: " <<options.fo <<'\n';
    if(options.soundDev) cout <<"Sound device: " <<options.soundDev <<'\n';
    if(options.logfile) cout <<"Logfile: " <<options.logfile <<'\n';
    if(options.setDate) cout <<"Set date: " <<options.setDate <<'\n';
    cout <<'\n';
  }
  
  SRC.set_verbose(options.verb);
  
  if(options.logfile) SRC.logOnFile(options.logfile);
  else SRC.logOnSTDOUT();

  if(options.do_sync) SRC.yes_sync();
  else SRC.no_sync();

  if((options.SRCaction != 1) && (options.SRCaction != 2)) {
    cerr <<"EE: You should specify either option -d or -p\n\n";
    print_help(argv[0]);
    return 1;
  }


  do {	// repetition loop
    if(options.SRCaction == 1) {
      if(options.fo) {
        openState = SRC.open_file_input(options.fo, options.fc, options.channels);
        if(!openState) { 
	  cerr <<"EE: Unable to open input stream file " <<options.fo <<'\n';
	  return 1;
        }
      }
      else {
        openState = SRC.open_soundStream_input(options.fc, options.channels, options.soundDev);
        if(!openState) {
	  cerr <<"EE: Unable to open sound input stream. " <<SRC.get_sound_error() <<'\n';
	  return 1;
        }
      }


      SRC.set_decision_threshold(options.th);
      SRC.setWDS(options.wds, options.snr_level);
      SRC.set_timeout(options.timeout);
      SRC.decode();

      error = SRC.internalError();
      if(options.sys_sync && SRC.sincronized()) {		// syncronisation of the system clock
        struct timeval t;
        struct timezone tz;	// timezone
        tm ttmm;

        tz.tz_minuteswest = 60*(1 + int(SRC.OE()));
//      tz.tz_dsttime = DST_MET;		// central europe dst time constant. For old systems....

        ttmm = SRC.get_date_tm();	// 1) return the tm structure of the date/time
        t.tv_sec  = mktime(&ttmm);	// 2) convert the current time to the number of seconds since the "epoc"
// 3) set the number of microseonds. These are calculated taking into account the delay since last reading plus an uncertainty value
        t.tv_usec = SRC.getMilliseconds()*1000l + SRC.microsecDelay() + options.delay;

        error = settimeofday(&t, &tz);
        if(error == 0) cout <<"System clock updated!!\n" <<SRC.dateSTD() <<'\n';
        else cerr <<"EE: Unable to syncronise the system clock.\n";

      }
    }
    else if(options.SRCaction == 2) {
      if(options.fo)
        SRC.open_file_output(options.fo, options.fc, options.channels);
      else
        SRC.open_soundStream_output(options.fc, options.channels, options.soundDev);
    
      if(SRC.get_OUTstate() == 0) {
        cerr <<"Error in opening output stream!\n";
        return 1;
      }

      if(options.setDate) SRC.set(options.setDate, "%H:%M %d/%m/%Y");
      switch (options.dst) {
        case 1:	SRC.setOE(true);
		break;
        case 2: SRC.setOE(false);
		break;
        default: break;
      }

      if((options.chdate != 7) || (options.leap != 0))
        SRC.setWarnings(options.chdate, options.leap);
    
      SRC.play(options.power, options.random_samples, options.random_theta, options.noise);
      if(options.verb >= 1) {
        cout <<SRC.dateSTR(options.iso) <<'\n';
        if(options.binary) cout <<SRC <<'\n';
      }
    }
  
    if(options.verb >= 0) {
      if(options.SRCaction == 1) {
        if(SRC.OK()) {
	  cout <<SRC.dateSTR(options.iso) <<'\n';
	  if(SRC.warnings() && (options.verb >= 1)) {
	    cout <<"---------------\nSRC WARNINGS:\n";
	    if(SRC.SE() != 7) cout <<"Change time in " <<SRC.SE() <<" days\n";
	    if(SRC.SI()) cout <<"Leap second at the end of the month: " <<SRC.SI() <<" second\n";
	    cout <<"---------------\n";
	  }
          if(options.binary) cout <<SRC <<'\n';
        }
        else {
  	  cerr <<"Decoding error!\n";
//	  error = 1;
        }
      }
    }

    SRC.close_all();
    options.repeat--;
  } while(options.repeat != 0);


  return error;
}
