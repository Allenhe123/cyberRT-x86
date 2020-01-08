﻿// Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "LatencyTestPublisher.h"
#include "LatencyTestSubscriber.h"

#include "optionparser.h"

#include <stdio.h>
#include <string>
#include <iostream>
#include <iomanip>
#include <bitset>
#include <cstdint>

#include <fastrtps/log/Log.h>
#include <fastrtps/Domain.h>

#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable:4512)
#endif

using namespace eprosima;
using namespace fastrtps;


#if defined(__LITTLE_ENDIAN__)
const Endianness_t DEFAULT_ENDIAN = LITTLEEND;
#elif defined (__BIG_ENDIAN__)
const Endianness_t DEFAULT_ENDIAN = BIGEND;
#endif

#if defined(_WIN32)
#define COPYSTR strcpy_s
#else
#define COPYSTR strcpy
#endif


struct Arg: public option::Arg{

    static void printError(const char* msg1, const option::Option& opt, const char* msg2){
        fprintf(stderr, "%s", msg1);
        fwrite(opt.name, opt.namelen, 1, stderr);
        fprintf(stderr, "%s", msg2);
    }

    static option::ArgStatus Unknown(const option::Option& option, bool msg){
        if (msg) printError("Unknown option '", option, "'\n");
        return option::ARG_ILLEGAL;
    }

    static option::ArgStatus Required(const option::Option& option, bool msg){
        if (option.arg != 0 && option.arg[0] != 0)
        return option::ARG_OK;

        if (msg) printError("Option '", option, "' requires an argument\n");
        return option::ARG_ILLEGAL;
    }

    static option::ArgStatus Numeric(const option::Option& option, bool msg){
        char* endptr = 0;
        if (option.arg != 0 && strtol(option.arg, &endptr, 10)){};
        if (endptr != option.arg && *endptr == 0)
        return option::ARG_OK;

        if (msg) printError("Option '", option, "' requires a numeric argument\n");
        return option::ARG_ILLEGAL;
    }
};

enum  optionIndex {
    UNKNOWN_OPT,
    HELP,
    RELIABILITY,
    SAMPLES,
    SEED,
    SUBSCRIBERS,
    ECHO_OPT,
    HOSTNAME,
    EXPORT_CSV
};

const option::Descriptor usage[] = {
    { UNKNOWN_OPT, 0,"", "",            Arg::None,      "Usage: ThroughputTest <publisher|subscriber>\n\nGeneral options:" },
    { HELP,    0,"h", "help",           Arg::None,      "  -h \t--help  \tProduce help message." },
    { RELIABILITY,0,"r","reliability",  Arg::Required,  "  -r <arg>, \t--reliability=<arg>  \tSet reliability (\"reliable\"/\"besteffort\")."},
    { SAMPLES,0,"s","samples",          Arg::Numeric,  "  -s <num>, \t--samples=<num>  \tNumber of samples." },
    { SEED,0,"","seed",                 Arg::Numeric,  "  \t--seed=<num>  \tNumber of subscribers." },
    { UNKNOWN_OPT, 0,"", "",            Arg::None,      "\nPublisher options:"},
    { SUBSCRIBERS,0,"n","subscribers",  Arg::Numeric,  "  -n <num>,   \t--subscribers=<arg>  \tSeed to calculate domain and topic, to isolate test." },
    { UNKNOWN_OPT, 0,"", "",            Arg::None,      "\nSubscriber options:"},
    { ECHO_OPT, 0,"e","echo",           Arg::Required,  "  -e <arg>, \t--echo=<arg>  \tEcho mode (\"true\"/\"false\")." },
    { HOSTNAME,0,"","hostname",         Arg::None,      "" },
    { EXPORT_CSV,0,"","export_csv",     Arg::None,      "" },
    { 0, 0, 0, 0, 0, 0 }
};

const int c_n_samples = 10000;

int main(int argc, char** argv){

    int columns;

#if defined(_WIN32)
    char* buf = nullptr;
    size_t sz = 0;
    if (_dupenv_s(&buf, &sz, "COLUMNS") == 0 && buf != nullptr){
        columns = strtol(buf, nullptr, 10);
        free(buf);
    }
    else{
        columns = 80;
    }
#else
    columns = getenv("COLUMNS")? atoi(getenv("COLUMNS")) : 80;
#endif

    bool pub_sub = false;
    int sub_number = 1;
    int n_samples = c_n_samples;
    bool echo = true;
    bool reliable = false;
    uint32_t seed = 80;
    bool hostname = false;
    bool export_csv = false;

    argc-=(argc>0); argv+=(argc>0); // skip program name argv[0] if present
    if(argc){
		if(strcmp(argv[0],"publisher") == 0){
			pub_sub = true;
        }
		else if(strcmp(argv[0],"subscriber") == 0){
			pub_sub = false;
        }
        else{
            option::printUsage(fwrite, stdout, usage, columns);
            return 0;
        }
    }
	else{
        option::printUsage(fwrite, stdout, usage, columns);
        return 0;
	}

    argc-=(argc>0); argv+=(argc>0); // skip pub/sub argument
    option::Stats stats(usage, argc, argv);
    std::vector<option::Option> options(stats.options_max);
    std::vector<option::Option> buffer(stats.buffer_max);
    option::Parser parse(usage, argc, argv, &options[0], &buffer[0]);

    if (parse.error())
    return 1;

    if (options[HELP]){
        option::printUsage(fwrite, stdout, usage, columns);
        return 0;
    }

    for (int i = 0; i < parse.optionsCount(); ++i){
        option::Option& opt = buffer[i];
        switch (opt.index()){
            case HELP:
                // not possible, because handled further above and exits the program
                break;
            case RELIABILITY:
                if(strcmp(opt.arg, "reliable") == 0){
        			reliable = true;
                }
        		else if(strcmp(opt.arg, "besteffort") == 0){
        			reliable = false;
                }
                else{
                    option::printUsage(fwrite, stdout, usage, columns);
                    return 0;
                }
                break;

            case SEED:
                seed = strtol(opt.arg, nullptr, 10);
                break;
            case SAMPLES:
                n_samples = strtol(opt.arg, nullptr, 10);
                break;

            case SUBSCRIBERS:
                sub_number = strtol(opt.arg, nullptr, 10);
                break;

            case ECHO_OPT:
                if(strcmp(opt.arg, "true") == 0){
                    echo = true;
                }
                else if(strcmp(opt.arg, "false") == 0){
                    echo = false;
                }
                else{
                    option::printUsage(fwrite, stdout, usage, columns);
                    return 0;
                }
                break;

            case HOSTNAME:
                hostname = true;
                break;

            case EXPORT_CSV:
                export_csv = true;
                break;

            case UNKNOWN_OPT:
                option::printUsage(fwrite, stdout, usage, columns);
                return 0;
                break;
        }
    }

    if (pub_sub){
        cout << "Performing test with "<< sub_number << " subscribers and "<<n_samples << " samples" <<endl;
        LatencyTestPublisher latencyPub;
        latencyPub.init(sub_number,n_samples, reliable, seed, hostname, export_csv);
        latencyPub.run();
    }
    else {
        LatencyTestSubscriber latencySub;
		latencySub.init(echo, n_samples, reliable, seed, hostname);
        latencySub.run();
    }

	eClock::my_sleep(1000);

	cout << "EVERYTHING STOPPED FINE"<<endl;

	return 0;
}

#if defined(_MSC_VER)
#pragma warning (pop)
#endif
