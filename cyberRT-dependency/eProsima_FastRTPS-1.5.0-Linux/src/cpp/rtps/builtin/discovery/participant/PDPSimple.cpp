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
 * @file PDPSimple.cpp
 *
 */

#include <fastrtps/rtps/builtin/discovery/participant/PDPSimple.h>
#include <fastrtps/rtps/builtin/discovery/participant/PDPSimpleListener.h>

#include <fastrtps/rtps/builtin/BuiltinProtocols.h>
#include <fastrtps/rtps/builtin/liveliness/WLP.h>

#include <fastrtps/rtps/builtin/data/ParticipantProxyData.h>
#include <fastrtps/rtps/builtin/discovery/participant/timedevent/RemoteParticipantLeaseDuration.h>
#include <fastrtps/rtps/builtin/data/ReaderProxyData.h>
#include <fastrtps/rtps/builtin/data/WriterProxyData.h>

#include <fastrtps/rtps/builtin/discovery/endpoint/EDPSimple.h>
#include <fastrtps/rtps/builtin/discovery/endpoint/EDPStatic.h>

#include <fastrtps/rtps/resources/AsyncWriterThread.h>

#include <fastrtps/rtps/builtin/discovery/participant/timedevent/ResendParticipantProxyDataPeriod.h>


#include "../../../participant/RTPSParticipantImpl.h"

#include <fastrtps/rtps/writer/StatelessWriter.h>
#include <fastrtps/rtps/reader/StatelessReader.h>
#include <fastrtps/rtps/reader/StatefulReader.h>
#include <fastrtps/rtps/reader/WriterProxy.h>

#include <fastrtps/rtps/history/WriterHistory.h>
#include <fastrtps/rtps/history/ReaderHistory.h>


#include <fastrtps/utils/TimeConversion.h>

#include <fastrtps/log/Log.h>

#include <mutex>

using namespace eprosima::fastrtps;

namespace eprosima {
namespace fastrtps{
namespace rtps {


PDPSimple::PDPSimple(BuiltinProtocols* built):
    mp_builtin(built),
    mp_RTPSParticipant(nullptr),
    mp_SPDPWriter(nullptr),
    mp_SPDPReader(nullptr),
    mp_EDP(nullptr),
    m_hasChangedLocalPDP(true),
    mp_resendParticipantTimer(nullptr),
    mp_listener(nullptr),
    mp_SPDPWriterHistory(nullptr),
    mp_SPDPReaderHistory(nullptr),
    mp_mutex(new std::recursive_mutex())
    {

    }

PDPSimple::~PDPSimple()
{
    if(mp_resendParticipantTimer != nullptr)
        delete(mp_resendParticipantTimer);

    if(mp_EDP!=nullptr)
        delete(mp_EDP);

    delete(mp_SPDPWriter);
    delete(mp_SPDPReader);
    delete(mp_SPDPWriterHistory);
    delete(mp_SPDPReaderHistory);

    delete(mp_listener);
    for(auto it = this->m_participantProxies.begin();
            it!=this->m_participantProxies.end();++it)
    {
        delete(*it);
    }

    delete(mp_mutex);
}

void PDPSimple::initializeParticipantProxyData(ParticipantProxyData* participant_data)
{
    participant_data->m_leaseDuration = mp_RTPSParticipant->getAttributes().builtin.leaseDuration;
    set_VendorId_eProsima(participant_data->m_VendorId);

    participant_data->m_availableBuiltinEndpoints |= DISC_BUILTIN_ENDPOINT_PARTICIPANT_ANNOUNCER;
    participant_data->m_availableBuiltinEndpoints |= DISC_BUILTIN_ENDPOINT_PARTICIPANT_DETECTOR;
    if(mp_RTPSParticipant->getAttributes().builtin.use_WriterLivelinessProtocol)
    {
        participant_data->m_availableBuiltinEndpoints |= BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_WRITER;
        participant_data->m_availableBuiltinEndpoints |= BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_READER;
    }
    if(mp_RTPSParticipant->getAttributes().builtin.use_SIMPLE_EndpointDiscoveryProtocol)
    {
        if(mp_RTPSParticipant->getAttributes().builtin.m_simpleEDP.use_PublicationWriterANDSubscriptionReader)
        {
            participant_data->m_availableBuiltinEndpoints |= DISC_BUILTIN_ENDPOINT_PUBLICATION_ANNOUNCER;
            participant_data->m_availableBuiltinEndpoints |= DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_DETECTOR;
        }
        if(mp_RTPSParticipant->getAttributes().builtin.m_simpleEDP.use_PublicationReaderANDSubscriptionWriter)
        {
            participant_data->m_availableBuiltinEndpoints |= DISC_BUILTIN_ENDPOINT_PUBLICATION_DETECTOR;
            participant_data->m_availableBuiltinEndpoints |= DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_ANNOUNCER;
        }
    }

#if HAVE_SECURITY
    participant_data->m_availableBuiltinEndpoints |= mp_RTPSParticipant->security_manager().builtin_endpoints();
#endif

    participant_data->m_defaultUnicastLocatorList = mp_RTPSParticipant->getAttributes().defaultUnicastLocatorList;
    participant_data->m_defaultMulticastLocatorList = mp_RTPSParticipant->getAttributes().defaultMulticastLocatorList;
    participant_data->m_expectsInlineQos = false;
    participant_data->m_guid = mp_RTPSParticipant->getGuid();
    for(uint8_t i = 0; i<16; ++i)
    {
        if(i<12)
            participant_data->m_key.value[i] = participant_data->m_guid.guidPrefix.value[i];
        else
            participant_data->m_key.value[i] = participant_data->m_guid.entityId.value[i - 12];
    }


    participant_data->m_metatrafficMulticastLocatorList = this->mp_builtin->m_metatrafficMulticastLocatorList;
    participant_data->m_metatrafficUnicastLocatorList = this->mp_builtin->m_metatrafficUnicastLocatorList;

    participant_data->m_participantName = std::string(mp_RTPSParticipant->getAttributes().getName());

    participant_data->m_userData = mp_RTPSParticipant->getAttributes().userData;

#if HAVE_SECURITY
    IdentityToken* identity_token = nullptr;
    if(mp_RTPSParticipant->security_manager().get_identity_token(&identity_token) && identity_token != nullptr)
    {
        participant_data->identity_token_ = std::move(*identity_token);
        mp_RTPSParticipant->security_manager().return_identity_token(identity_token);
    }
#endif
}

bool PDPSimple::initPDP(RTPSParticipantImpl* part)
{
    logInfo(RTPS_PDP,"Beginning");
    mp_RTPSParticipant = part;
    m_discovery = mp_RTPSParticipant->getAttributes().builtin;
    std::lock_guard<std::recursive_mutex> guardPDP(*this->mp_mutex);
    //CREATE ENDPOINTS
    if(!createSPDPEndpoints())
        return false;
    //UPDATE METATRAFFIC.
    mp_builtin->updateMetatrafficLocators(this->mp_SPDPReader->getAttributes()->unicastLocatorList);
    //std::lock_guard<std::recursive_mutex> guardR(*this->mp_SPDPReader->getMutex());
    //std::lock_guard<std::recursive_mutex> guardW(*this->mp_SPDPWriter->getMutex());
    m_participantProxies.push_back(new ParticipantProxyData());
    initializeParticipantProxyData(m_participantProxies.front());

    //INIT EDP
    if(m_discovery.use_STATIC_EndpointDiscoveryProtocol)
    {
        mp_EDP = (EDP*)(new EDPStatic(this,mp_RTPSParticipant));
        if(!mp_EDP->initEDP(m_discovery)){
            logError(RTPS_PDP,"Endpoint discovery configuration failed");
            return false;
        }

    }
    else if(m_discovery.use_SIMPLE_EndpointDiscoveryProtocol)
    {
        mp_EDP = (EDP*)(new EDPSimple(this,mp_RTPSParticipant));
        if(!mp_EDP->initEDP(m_discovery)){
            logError(RTPS_PDP,"Endpoint discovery configuration failed");
            return false;
        }
    }
    else
    {
        logWarning(RTPS_PDP,"No EndpointDiscoveryProtocol defined");
        return false;
    }

    if(!mp_RTPSParticipant->enableReader(mp_SPDPReader))
        return false;

    mp_resendParticipantTimer = new ResendParticipantProxyDataPeriod(this,TimeConv::Time_t2MilliSecondsDouble(m_discovery.leaseDuration_announcementperiod));

    return true;
}

void PDPSimple::stopParticipantAnnouncement()
{
    mp_resendParticipantTimer->cancel_timer();
}

void PDPSimple::resetParticipantAnnouncement()
{
    mp_resendParticipantTimer->restart_timer();
}

void PDPSimple::announceParticipantState(bool new_change, bool dispose)
{
    logInfo(RTPS_PDP,"Announcing RTPSParticipant State (new change: "<< new_change <<")");
    CacheChange_t* change = nullptr;

    std::lock_guard<std::recursive_mutex> guardPDP(*this->mp_mutex);

    if(!dispose)
    {
        if(new_change || m_hasChangedLocalPDP)
        {
            this->getLocalParticipantProxyData()->m_manualLivelinessCount++;
            if(mp_SPDPWriterHistory->getHistorySize() > 0)
                mp_SPDPWriterHistory->remove_min_change();
            // TODO(Ricardo) Change DISCOVERY_PARTICIPANT_DATA_MAX_SIZE with getLocalParticipantProxyData()->size().
            change = mp_SPDPWriter->new_change([]() -> uint32_t {return DISCOVERY_PARTICIPANT_DATA_MAX_SIZE;}, ALIVE,getLocalParticipantProxyData()->m_key);
            if(getLocalParticipantProxyData()->toParameterList())
            {
                CDRMessage_t aux_msg(0);
                aux_msg.wraps = true;
                aux_msg.buffer = change->serializedPayload.data;
                aux_msg.max_size = change->serializedPayload.max_size;

#if EPROSIMA_BIG_ENDIAN
                change->serializedPayload.encapsulation = (uint16_t)PL_CDR_BE;
                aux_msg.msg_endian = BIGEND;
#else
                change->serializedPayload.encapsulation = (uint16_t)PL_CDR_LE;
                aux_msg.msg_endian =  LITTLEEND;
#endif

                ParameterList::writeParameterListToCDRMsg(&aux_msg, &getLocalParticipantProxyData()->m_QosList.allQos, true);
                change->serializedPayload.length = (uint16_t)aux_msg.length;

                mp_SPDPWriterHistory->add_change(change);
            }
            m_hasChangedLocalPDP = false;
        }
        else
        {
            mp_SPDPWriter->unsent_changes_reset();
        }
    }
    else
    {
        if(mp_SPDPWriterHistory->getHistorySize() > 0)
            mp_SPDPWriterHistory->remove_min_change();
        change = mp_SPDPWriter->new_change([]() -> uint32_t {return DISCOVERY_PARTICIPANT_DATA_MAX_SIZE;}, NOT_ALIVE_DISPOSED_UNREGISTERED, getLocalParticipantProxyData()->m_key);
        if(getLocalParticipantProxyData()->toParameterList())
        {
            CDRMessage_t aux_msg(0);
            aux_msg.wraps = true;
            aux_msg.buffer = change->serializedPayload.data;
            aux_msg.max_size = change->serializedPayload.max_size;

#if EPROSIMA_BIG_ENDIAN
            change->serializedPayload.encapsulation = (uint16_t)PL_CDR_BE;
            aux_msg.msg_endian = BIGEND;
#else
            change->serializedPayload.encapsulation = (uint16_t)PL_CDR_LE;
            aux_msg.msg_endian =  LITTLEEND;
#endif

            ParameterList::writeParameterListToCDRMsg(&aux_msg, &getLocalParticipantProxyData()->m_QosList.allQos, true);
            change->serializedPayload.length = (uint16_t)aux_msg.length;

            mp_SPDPWriterHistory->add_change(change);
        }
    }

}

bool PDPSimple::lookupReaderProxyData(const GUID_t& reader, ReaderProxyData** rdata, ParticipantProxyData** pdata)
{
    std::lock_guard<std::recursive_mutex> guardPDP(*this->mp_mutex);
    for (auto pit = m_participantProxies.begin();
            pit != m_participantProxies.end();++pit)
    {
        std::lock_guard<std::recursive_mutex> guard(*(*pit)->mp_mutex);
        for (auto rit = (*pit)->m_readers.begin();
                rit != (*pit)->m_readers.end();++rit)
        {
            if((*rit)->guid() == reader)
            {
                *rdata = *rit;
                *pdata = *pit;
                return true;
            }
        }
    }
    return false;
}

bool PDPSimple::lookupWriterProxyData(const GUID_t& writer, WriterProxyData** wdata, ParticipantProxyData** pdata)
{
    std::lock_guard<std::recursive_mutex> guardPDP(*this->mp_mutex);
    for (auto pit = m_participantProxies.begin();
            pit != m_participantProxies.end(); ++pit)
    {
        std::lock_guard<std::recursive_mutex> guard(*(*pit)->mp_mutex);
        for (auto wit = (*pit)->m_writers.begin();
                wit != (*pit)->m_writers.end(); ++wit)
        {
            if((*wit)->guid() == writer)
            {
                *wdata = *wit;
                *pdata = *pit;
                return true;
            }
        }
    }
    return false;
}

bool PDPSimple::removeReaderProxyData(ParticipantProxyData* pdata, ReaderProxyData* rdata)
{
    logInfo(RTPS_PDP,rdata->guid());
    std::lock_guard<std::recursive_mutex> guardPDP(*this->mp_mutex);
    std::lock_guard<std::recursive_mutex> guard(*pdata->mp_mutex);
    for(std::vector<ReaderProxyData*>::iterator rit = pdata->m_readers.begin();
            rit != pdata->m_readers.end(); ++rit)
    {
        if((*rit)->guid() == rdata->guid())
        {
            pdata->m_readers.erase(rit);
            delete(rdata);
            return true;
        }
    }
    return false;
}

bool PDPSimple::removeWriterProxyData(ParticipantProxyData* pdata, WriterProxyData* wdata)
{
    logInfo(RTPS_PDP, wdata->guid());
    std::lock_guard<std::recursive_mutex> guardPDP(*this->mp_mutex);
    std::lock_guard<std::recursive_mutex> guard(*pdata->mp_mutex);
    for(std::vector<WriterProxyData*>::iterator wit = pdata->m_writers.begin();
            wit != pdata->m_writers.end(); ++wit)
    {
        if((*wit)->guid() == wdata->guid())
        {
            pdata->m_writers.erase(wit);
            delete(wdata);
            return true;
        }
    }
    return false;
}


bool PDPSimple::lookupParticipantProxyData(const GUID_t& pguid,ParticipantProxyData** pdata)
{
    logInfo(RTPS_PDP,pguid);
    std::lock_guard<std::recursive_mutex> guardPDP(*this->mp_mutex);
    for(std::vector<ParticipantProxyData*>::iterator pit = m_participantProxies.begin();
            pit!=m_participantProxies.end();++pit)
    {
        if((*pit)->m_guid == pguid)
        {
            *pdata = *pit;
            return true;
        }
    }
    return false;
}

bool PDPSimple::createSPDPEndpoints()
{
    logInfo(RTPS_PDP,"Beginning");
    //SPDP BUILTIN RTPSParticipant WRITER
    HistoryAttributes hatt;
    hatt.payloadMaxSize = DISCOVERY_PARTICIPANT_DATA_MAX_SIZE;
    hatt.initialReservedCaches = 20;
    hatt.maximumReservedCaches = 100;
    mp_SPDPWriterHistory = new WriterHistory(hatt);
    WriterAttributes watt;
    watt.endpoint.endpointKind = WRITER;
    watt.endpoint.durabilityKind = TRANSIENT_LOCAL;
    watt.endpoint.reliabilityKind = BEST_EFFORT;
    watt.endpoint.topicKind = WITH_KEY;
    if(mp_RTPSParticipant->getRTPSParticipantAttributes().throughputController.bytesPerPeriod != UINT32_MAX &&
            mp_RTPSParticipant->getRTPSParticipantAttributes().throughputController.periodMillisecs != 0)
        watt.mode = ASYNCHRONOUS_WRITER;
    RTPSWriter* wout;
    if(mp_RTPSParticipant->createWriter(&wout,watt,mp_SPDPWriterHistory,nullptr,c_EntityId_SPDPWriter,true))
    {
#if HAVE_SECURITY
        mp_RTPSParticipant->set_endpoint_rtps_protection_supports(wout, false);
#endif
        mp_SPDPWriter = dynamic_cast<StatelessWriter*>(wout);
        for(LocatorListIterator lit = mp_builtin->m_initialPeersList.begin();
                lit != mp_builtin->m_initialPeersList.end(); ++lit)
            mp_SPDPWriter->add_locator(*lit);
    }
    else
    {
        logError(RTPS_PDP,"SimplePDP Writer creation failed");
        delete(mp_SPDPWriterHistory);
        mp_SPDPWriterHistory = nullptr;
        return false;
    }
    hatt.payloadMaxSize = DISCOVERY_PARTICIPANT_DATA_MAX_SIZE;
    hatt.initialReservedCaches = 250;
    hatt.maximumReservedCaches = 5000;
    mp_SPDPReaderHistory = new ReaderHistory(hatt);
    ReaderAttributes ratt;
    ratt.endpoint.multicastLocatorList = mp_builtin->m_metatrafficMulticastLocatorList;
    ratt.endpoint.unicastLocatorList = mp_builtin->m_metatrafficUnicastLocatorList;
    ratt.endpoint.topicKind = WITH_KEY;
    ratt.endpoint.durabilityKind = TRANSIENT_LOCAL;
    ratt.endpoint.reliabilityKind = BEST_EFFORT;
    mp_listener = new PDPSimpleListener(this);
    RTPSReader* rout;
    if(mp_RTPSParticipant->createReader(&rout,ratt,mp_SPDPReaderHistory,mp_listener,c_EntityId_SPDPReader,true, false))
    {
#if HAVE_SECURITY
        mp_RTPSParticipant->set_endpoint_rtps_protection_supports(rout, false);
#endif
        mp_SPDPReader = dynamic_cast<StatelessReader*>(rout);
    }
    else
    {
        logError(RTPS_PDP,"SimplePDP Reader creation failed");
        delete(mp_SPDPReaderHistory);
        mp_SPDPReaderHistory = nullptr;
        delete(mp_listener);
        mp_listener = nullptr;
        return false;
    }

    logInfo(RTPS_PDP,"SPDP Endpoints creation finished");
    return true;
}

bool PDPSimple::addReaderProxyData(ReaderProxyData* rdata,bool copydata,
        ReaderProxyData** returnReaderProxyData,ParticipantProxyData** pdata)
{
    logInfo(RTPS_PDP,rdata->guid());
    std::lock_guard<std::recursive_mutex> guardPDP(*this->mp_mutex);
    for(std::vector<ParticipantProxyData*>::iterator pit = m_participantProxies.begin();
            pit!=m_participantProxies.end();++pit)
    {
        std::lock_guard<std::recursive_mutex> guard(*(*pit)->mp_mutex);
        if((*pit)->m_guid.guidPrefix == rdata->guid().guidPrefix)
        {
            //CHECK THAT IT IS NOT ALREADY THERE:
            for(std::vector<ReaderProxyData*>::iterator rit = (*pit)->m_readers.begin();
                    rit!=(*pit)->m_readers.end();++rit)
            {
                if((*rit)->guid().entityId == rdata->guid().entityId)
                {
                    if(copydata)
                        *returnReaderProxyData = *rit;
                    if(pdata != nullptr)
                        *pdata = *pit;
                    return false;
                }
            }
            if(copydata)
            {
                ReaderProxyData* newRPD = new ReaderProxyData();
                newRPD->copy(rdata);
                (*pit)->m_readers.push_back(newRPD);
                *returnReaderProxyData = newRPD;
                if(pdata != nullptr)
                    *pdata = *pit;
            }
            else
            {
                (*pit)->m_readers.push_back(rdata);
                if(pdata != nullptr)
                    *pdata = *pit;
            }
            return true;
        }
    }
    return false;
}

bool PDPSimple::addWriterProxyData(WriterProxyData* wdata,bool copydata,
        WriterProxyData** returnWriterProxyData,ParticipantProxyData** pdata)
{
    logInfo(RTPS_PDP,wdata->guid());
    std::lock_guard<std::recursive_mutex> guardPDP(*this->mp_mutex);
    for(std::vector<ParticipantProxyData*>::iterator pit = m_participantProxies.begin();
            pit!=m_participantProxies.end();++pit)
    {
        std::lock_guard<std::recursive_mutex> guard(*(*pit)->mp_mutex);
        if((*pit)->m_guid.guidPrefix == wdata->guid().guidPrefix)
        {
            //CHECK THAT IT IS NOT ALREADY THERE:
            for(std::vector<WriterProxyData*>::iterator wit = (*pit)->m_writers.begin();
                    wit!=(*pit)->m_writers.end();++wit)
            {
                if((*wit)->guid().entityId == wdata->guid().entityId)
                {
                    if(copydata)
                        *returnWriterProxyData = *wit;
                    if(pdata != nullptr)
                        *pdata = *pit;
                    return false;
                }
            }
            if(copydata)
            {
                WriterProxyData* newWPD = new WriterProxyData();
                newWPD->copy(wdata);
                (*pit)->m_writers.push_back(newWPD);
                *returnWriterProxyData = newWPD;
                if(pdata != nullptr)
                    *pdata = *pit;
            }
            else
            {
                (*pit)->m_writers.push_back(wdata);
                if(pdata != nullptr)
                    *pdata = *pit;
            }
            return true;
        }
    }
    return false;
}

void PDPSimple::assignRemoteEndpoints(ParticipantProxyData* pdata)
{
    logInfo(RTPS_PDP,"For RTPSParticipant: "<<pdata->m_guid.guidPrefix);
    uint32_t endp = pdata->m_availableBuiltinEndpoints;
    uint32_t auxendp = endp;
    auxendp &=DISC_BUILTIN_ENDPOINT_PARTICIPANT_ANNOUNCER;
    // TODO Review because the mutex is already take in PDPSimpleListener.
    std::lock_guard<std::recursive_mutex> guard(*pdata->mp_mutex);
    if(auxendp!=0)
    {
        RemoteWriterAttributes watt;
        watt.guid.guidPrefix = pdata->m_guid.guidPrefix;
        watt.guid.entityId = c_EntityId_SPDPWriter;
        watt.endpoint.unicastLocatorList = pdata->m_metatrafficUnicastLocatorList;
        watt.endpoint.multicastLocatorList = pdata->m_metatrafficMulticastLocatorList;
        watt.endpoint.reliabilityKind = BEST_EFFORT;
        watt.endpoint.durabilityKind = TRANSIENT_LOCAL;
        pdata->m_builtinWriters.push_back(watt);
        mp_SPDPReader->matched_writer_add(watt);
    }
    auxendp = endp;
    auxendp &=DISC_BUILTIN_ENDPOINT_PARTICIPANT_DETECTOR;
    if(auxendp!=0)
    {
        RemoteReaderAttributes ratt;
        ratt.expectsInlineQos = false;
        ratt.guid.guidPrefix = pdata->m_guid.guidPrefix;
        ratt.guid.entityId = c_EntityId_SPDPReader;
        ratt.endpoint.unicastLocatorList = pdata->m_metatrafficUnicastLocatorList;
        ratt.endpoint.multicastLocatorList = pdata->m_metatrafficMulticastLocatorList;
        ratt.endpoint.reliabilityKind = BEST_EFFORT;
        ratt.endpoint.durabilityKind = TRANSIENT_LOCAL;
        pdata->m_builtinReaders.push_back(ratt);
        mp_SPDPWriter->matched_reader_add(ratt);
    }

#if HAVE_SECURITY
    // Validate remote participant
    mp_RTPSParticipant->security_manager().discovered_participant(pdata);
#else
    //Inform EDP of new RTPSParticipant data:
    notifyAboveRemoteEndpoints(pdata);
#endif
}

void PDPSimple::notifyAboveRemoteEndpoints(ParticipantProxyData* pdata)
{
    std::lock_guard<std::recursive_mutex> guard_pdpsimple(*mp_mutex);
    std::lock_guard<std::recursive_mutex> guard_pdata(*pdata->mp_mutex);

    //Inform EDP of new RTPSParticipant data:
    if(mp_EDP!=nullptr)
        mp_EDP->assignRemoteEndpoints(pdata);
    if(mp_builtin->mp_WLP !=nullptr)
        mp_builtin->mp_WLP->assignRemoteEndpoints(pdata);
}


void PDPSimple::removeRemoteEndpoints(ParticipantProxyData* pdata)
{
    logInfo(RTPS_PDP,"For RTPSParticipant: "<<pdata->m_guid);
    std::lock_guard<std::recursive_mutex> guard(*pdata->mp_mutex);
    for(auto it = pdata->m_builtinReaders.begin();
            it!=pdata->m_builtinReaders.end();++it)
    {
        if((*it).guid.entityId == c_EntityId_SPDPReader && this->mp_SPDPWriter !=nullptr)
            mp_SPDPWriter->matched_reader_remove(*it);
    }
    for(auto it = pdata->m_builtinWriters.begin();
            it!=pdata->m_builtinWriters.end();++it)
    {
        if((*it).guid.entityId == c_EntityId_SPDPWriter && this->mp_SPDPReader !=nullptr)
            mp_SPDPReader->matched_writer_remove(*it);
    }
}

bool PDPSimple::removeRemoteParticipant(GUID_t& partGUID)
{
    logInfo(RTPS_PDP,partGUID );
    std::unique_lock<std::recursive_mutex> guardW(*this->mp_SPDPWriter->getMutex());
    std::unique_lock<std::recursive_mutex> guardR(*this->mp_SPDPReader->getMutex());
    ParticipantProxyData* pdata = nullptr;
    //Remove it from our vector or RTPSParticipantProxies:
    std::unique_lock<std::recursive_mutex> guardPDP(*this->mp_mutex);
    for(std::vector<ParticipantProxyData*>::iterator pit = m_participantProxies.begin();
            pit!=m_participantProxies.end();++pit)
    {
        std::lock_guard<std::recursive_mutex> guard(*(*pit)->mp_mutex);
        if((*pit)->m_guid == partGUID)
        {
            pdata = *pit;
            m_participantProxies.erase(pit);
            break;
        }
    }

    if(pdata !=nullptr)
    {
        pdata->mp_mutex->lock();
        if(mp_EDP!=nullptr)
        {
            for(std::vector<ReaderProxyData*>::iterator rit = pdata->m_readers.begin();
                    rit!= pdata->m_readers.end();++rit)
            {
                mp_EDP->unpairReaderProxy(pdata, *rit);
            }
            for(std::vector<WriterProxyData*>::iterator wit = pdata->m_writers.begin();
                    wit!=pdata->m_writers.end();++wit)
            {
                mp_EDP->unpairWriterProxy(pdata, *wit);
            }
        }
#if HAVE_SECURITY
        mp_builtin->mp_participantImpl->security_manager().remove_participant(pdata);
#endif
        if(mp_builtin->mp_WLP != nullptr)
            this->mp_builtin->mp_WLP->removeRemoteEndpoints(pdata);
        this->mp_EDP->removeRemoteEndpoints(pdata);
        this->removeRemoteEndpoints(pdata);
        for(std::vector<CacheChange_t*>::iterator it=this->mp_SPDPReaderHistory->changesBegin();
                it!=this->mp_SPDPReaderHistory->changesEnd();++it)
        {
            if((*it)->instanceHandle == pdata->m_key)
            {
                this->mp_SPDPReaderHistory->remove_change(*it);
                break;
            }
        }
        pdata->mp_mutex->unlock();

        guardPDP.unlock();
        guardW.unlock();
        guardR.unlock();

        delete(pdata);
        return true;
    }

    return false;
}


void PDPSimple::assertRemoteParticipantLiveliness(const GuidPrefix_t& guidP)
{
    std::lock_guard<std::recursive_mutex> guardPDP(*this->mp_mutex);
    for(std::vector<ParticipantProxyData*>::iterator it = this->m_participantProxies.begin();
            it!=this->m_participantProxies.end();++it)
    {
        std::lock_guard<std::recursive_mutex> guard(*(*it)->mp_mutex);
        if((*it)->m_guid.guidPrefix == guidP)
        {
            logInfo(RTPS_LIVELINESS,"RTPSParticipant "<< (*it)->m_guid << " is Alive");
            // TODO Ricardo: Study if isAlive attribute is necessary.
            (*it)->isAlive = true;
            if((*it)->mp_leaseDurationTimer != nullptr)
            {
                (*it)->mp_leaseDurationTimer->cancel_timer();
                (*it)->mp_leaseDurationTimer->restart_timer();
            }
            break;
        }
    }
}

void PDPSimple::assertLocalWritersLiveliness(LivelinessQosPolicyKind kind)
{
    logInfo(RTPS_LIVELINESS,"of type " << (kind==AUTOMATIC_LIVELINESS_QOS?"AUTOMATIC":"")
            <<(kind==MANUAL_BY_PARTICIPANT_LIVELINESS_QOS?"MANUAL_BY_PARTICIPANT":""));
    std::lock_guard<std::recursive_mutex> guard(*this->mp_mutex);
    std::lock_guard<std::recursive_mutex> guard2(*this->m_participantProxies.front()->mp_mutex);
    for(std::vector<WriterProxyData*>::iterator wit = this->m_participantProxies.front()->m_writers.begin();
            wit!=this->m_participantProxies.front()->m_writers.end();++wit)
    {
        if((*wit)->m_qos.m_liveliness.kind == kind)
        {
            logInfo(RTPS_LIVELINESS,"Local Writer "<< (*wit)->guid().entityId << " marked as ALIVE");
            (*wit)->isAlive(true);
        }
    }
}

void PDPSimple::assertRemoteWritersLiveliness(GuidPrefix_t& guidP,LivelinessQosPolicyKind kind)
{
    std::lock_guard<std::recursive_mutex> pguard(*this->mp_mutex);
    logInfo(RTPS_LIVELINESS,"of type " << (kind==AUTOMATIC_LIVELINESS_QOS?"AUTOMATIC":"")
            <<(kind==MANUAL_BY_PARTICIPANT_LIVELINESS_QOS?"MANUAL_BY_PARTICIPANT":""));
    for(std::vector<ParticipantProxyData*>::iterator pit=this->m_participantProxies.begin();
            pit!=this->m_participantProxies.end();++pit)
    {
        std::lock_guard<std::recursive_mutex> guard(*(*pit)->mp_mutex);
        if((*pit)->m_guid.guidPrefix == guidP)
        {
            for(std::vector<WriterProxyData*>::iterator wit = (*pit)->m_writers.begin();
                    wit != (*pit)->m_writers.end();++wit)
            {
                if((*wit)->m_qos.m_liveliness.kind == kind)
                {
                    (*wit)->isAlive(true);
                    std::lock_guard<std::recursive_mutex> guardP(*mp_RTPSParticipant->getParticipantMutex());
                    for(std::vector<RTPSReader*>::iterator rit = mp_RTPSParticipant->userReadersListBegin();
                            rit!=mp_RTPSParticipant->userReadersListEnd();++rit)
                    {
                        if((*rit)->getAttributes()->reliabilityKind == RELIABLE)
                        {
                            StatefulReader* sfr = (StatefulReader*)(*rit);
                            WriterProxy* WP;
                            if(sfr->matched_writer_lookup((*wit)->guid(), &WP))
                            {
                                WP->assertLiveliness();
                                continue;
                            }
                        }
                    }
                }
            }
            break;
        }
    }
}

bool PDPSimple::newRemoteEndpointStaticallyDiscovered(const GUID_t& pguid, int16_t userDefinedId,EndpointKind_t kind)
{
    ParticipantProxyData* pdata;
    if(lookupParticipantProxyData(pguid, &pdata))
    {
        if(kind == WRITER)
            dynamic_cast<EDPStatic*>(mp_EDP)->newRemoteWriter(pdata,userDefinedId);
        else
            dynamic_cast<EDPStatic*>(mp_EDP)->newRemoteReader(pdata,userDefinedId);
    }
    return false;
}

CDRMessage_t PDPSimple::get_participant_proxy_data_serialized(Endianness_t endian)
{
    std::lock_guard<std::recursive_mutex> guardPDP(*this->mp_mutex);
    CDRMessage_t cdr_msg;
    cdr_msg.msg_endian = endian;

    ParameterList::writeParameterListToCDRMsg(&cdr_msg, &getLocalParticipantProxyData()->m_QosList.allQos, true);

    return cdr_msg;
}

} /* namespace rtps */
} /* namespace fastrtps */
} /* namespace eprosima */
