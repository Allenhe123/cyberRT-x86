// Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

/**
 * @file ApacheClient.h
 *
 */

#ifndef APACHECLIENT_H_
#define APACHECLIENT_H_

#include <string>

using namespace std;

class ApacheClientTest
{
public:
	ApacheClientTest():m_overhead(0){};
	~ApacheClientTest(){};
	double run(string ip, int samples, int bytes);
	double m_overhead;
};



#endif /* APACHECLIENT_H_ */
